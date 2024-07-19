/* MW31 implementation of NetworkInterfaceAPI
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MW31Interface.h"

// Various timeouts for different MW31 operations
#define MW31_CONNECT_TIMEOUT 15000
#define MW31_SEND_TIMEOUT    500
#define MW31_RECV_TIMEOUT    0
#define MW31_MISC_TIMEOUT    500

// MW31Interface implementation
MW31Interface::MW31Interface(PinName tx, PinName rx, bool debug)
    : _mxchip(tx, rx, debug)
{
    _channel=0;
    memset(_ids, 0, sizeof(_ids));
    memset(_cbs, 0, sizeof(_cbs));

    _mxchip.attach(this, &MW31Interface::event);
}

int MW31Interface::connect(const char *ssid, const char *pass, nsapi_security_t security,uint8_t channel)
{
    set_credentials(ssid, pass, security);
    return connect();
}

int MW31Interface::connect()
{
    _mxchip.setTimeout(MW31_CONNECT_TIMEOUT);
    if (!_mxchip.startup()) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    if (!_mxchip.connect(ap_ssid, ap_pass)) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    for(int id=0; id<MW31_SOCKET_COUNT; id++){
        if(!network_reconnect(false, id))
            return false;
    }
    if (!_mxchip.getIPAddress()) {
        return NSAPI_ERROR_DHCP_FAILURE;
    }

    return NSAPI_ERROR_OK;
}

int MW31Interface::set_credentials(const char *ssid, const char *pass, nsapi_security_t security)
{
    memset(ap_ssid, 0, sizeof(ap_ssid));
    strncpy(ap_ssid, ssid, sizeof(ap_ssid));

    memset(ap_pass, 0, sizeof(ap_pass));
    strncpy(ap_pass, pass, sizeof(ap_pass));

    ap_sec = security;

    return 0;
}

int MW31Interface::set_channel(uint8_t channel)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int MW31Interface::disconnect()
{
    _mxchip.setTimeout(MW31_MISC_TIMEOUT);

    if (!_mxchip.disconnect()) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }

    return 0;
}

const char* MW31Interface::get_ip_address()
{
    return _mxchip.getIPAddress();
}

const char* MW31Interface::get_mac_address()
{
    return _mxchip.getMACAddress();
}

const char* MW31Interface::get_gateway()
{
    return NULL;
}

const char* MW31Interface::get_netmask()
{
    return NULL;
}


int8_t MW31Interface::get_rssi()
{
    return _mxchip.getRSSI();
}


int MW31Interface::scan(WiFiAccessPoint *ap, unsigned count)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

bool MW31Interface::network_reconnect(bool ENABLE, uint8_t id)
{
    return _mxchip.NetworkReconnect(ENABLE, id);
}

struct MW31_socket {
    int id;
    nsapi_protocol_t proto;
    bool connected;
    SocketAddress addr;
};

int MW31Interface::socket_open(void **handle, nsapi_protocol_t proto)
{
    // Look for an unused socket
    int id = -1;

    for (int i = 0; i < MW31_SOCKET_COUNT; i++) {
        if (!_ids[i]) {
            id = i;
            _ids[i] = true;
            break;
        }
    }

    if (id == -1) {
        return NSAPI_ERROR_NO_SOCKET;
    }

    struct MW31_socket *socket = new struct MW31_socket;
    if (!socket) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    socket->id = id;
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return 0;
}

int MW31Interface::socket_close(void *handle)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    int err = 0;
    _mxchip.setTimeout(MW31_MISC_TIMEOUT);

    if (socket->connected && !_mxchip.close(socket->id)) {
        err = NSAPI_ERROR_DEVICE_ERROR;
    }

    socket->connected = false;
    _ids[socket->id] = false;
    delete socket;
    return err;
}

int MW31Interface::socket_bind(void *handle, const SocketAddress &address)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int MW31Interface::socket_listen(void *handle, int backlog)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int MW31Interface::socket_connect(void *handle, const SocketAddress &addr)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    _mxchip.setTimeout(MW31_MISC_TIMEOUT);
    const char *proto = ((socket->proto == NSAPI_TCP) ?  "tcp_client":"udp_unicast");
    if(!_mxchip.open(proto, socket->id, addr.get_ip_address(), addr.get_port()))
        return NSAPI_ERROR_DEVICE_ERROR;
    socket->connected = true;
    return 0;
}

int MW31Interface::socket_accept(void *server, void **handle, SocketAddress *addr )
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int MW31Interface::socket_send(void *handle, const void *data, unsigned size)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    _mxchip.setTimeout(MW31_SEND_TIMEOUT);
    if (!_mxchip.send(socket->id, data, size)) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return size;
}

int MW31Interface::socket_recv(void *handle, void *data, unsigned size)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    _mxchip.setTimeout(MW31_RECV_TIMEOUT);
    int32_t recv = _mxchip.recv(socket->id, data, size);
    if (recv < 0) {
        return NSAPI_ERROR_WOULD_BLOCK;
    }
    return recv;
}

int MW31Interface::socket_sendto(void *handle, const SocketAddress &addr, const void *data, unsigned size)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    if (!socket->connected ) {
        int err = socket_connect( socket, addr );
        if(err < 0)
            return err;
    }
    return socket_send(socket, data, size);
}

int MW31Interface::socket_recvfrom(void *handle, SocketAddress *addr, void *data, unsigned size)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    return socket_recv(socket, data, size);
}

void MW31Interface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
    struct MW31_socket *socket = (struct MW31_socket *)handle;
    _cbs[socket->id].callback = callback;
    _cbs[socket->id].data = data;
}

void MW31Interface::event() 
{
    for (int i = 0; i < MW31_SOCKET_COUNT; i++) {
        if (_cbs[i].callback) {
            _cbs[i].callback(_cbs[i].data);
        }
    }
}
