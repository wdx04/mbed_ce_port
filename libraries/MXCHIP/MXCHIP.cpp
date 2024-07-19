/* MXCHIP Example
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

#include "MXCHIP.h"

MXCHIP::MXCHIP(PinName tx, PinName rx, bool debug)
    : _serial(tx, rx, 115200), _parser(_serial, "\r\n", "\r")
    , _packets(0), _packets_end(&_packets)
{
    _parser.debugOn(debug);
}

bool MXCHIP::startup(void)
{
    int i;
    _parser.setTimeout(1000);
    for( i=0; i<3; i++){
        if((_parser.send("AT+FACTORY")
            &&_parser.recv("OK")))
        break;
    }
    while(1){
        if((_parser.send("AT") &&_parser.recv("OK")))
            break;
        ThisThread::sleep_for(100ms);
    }
    _parser.setTimeout(8000);

    if(!(_parser.send("AT+CIPRECVCFG=1")  //recv network data by send AT+CIPRECV=id\r
        &&_parser.recv("OK")))
        return false;
    _parser.oob("+CIPEVENT:SOCKET", this, &MXCHIP::_packet_handler);
    return true;
}

bool MXCHIP::dhcp(bool enabled)
{
    return _parser.send("AT+WDHCP=%s", enabled ? "ON":"OFF")
        && _parser.recv("OK");
}

bool MXCHIP::connect(const char *ap, const char *passPhrase)
{
    bool success = _parser.send("AT+WJAP=%s,%s", ap, passPhrase)
                    && _parser.recv("OK");
                  //  && _parser.recv("+WEVENT:STATION_UP");
    // _parser.setTimeout(8000);
    if(!success)
    	return false;
    if(_parser.recv("+WEVENT:STATION_UP"))
        return true;
    else
        return false;
}

bool MXCHIP::disconnect(void)
{
    return _parser.send("AT+WJAPQ")
        && _parser.recv("OK");
}

const char *MXCHIP::getIPAddress(void)
{
    if (!(_parser.send("AT+WJAPIP?")
        && _parser.recv("+WJAPIP:%[^,],%*[^,],%*[^,],%*[^\r]%*[\r]%*[\n]", _ip_buffer))) {
        return NULL;
    }
    return _ip_buffer;
}

const char *MXCHIP::getMACAddress(void)
{
    if (!(_parser.send("AT+WMAC?")
        && _parser.recv("+WMAC:%[^\r]%*[\r]%*[\n]", _mac_buffer))) {
        return 0;
    }
    return _mac_buffer;
}

//get current signal strength
int8_t MXCHIP::getRSSI()
{
    int8_t rssi = 0;
    if (!(_parser.send("AT+WJAP?")
        && _parser.recv("+WJAP:%*[^,],%*[^,],%*[^,],%[^\r]%*[\r]%*[\n]", &rssi))) {
        return false;
    }
    return rssi;
}

bool MXCHIP::isConnected(void)
{
    return getIPAddress() != 0;
}

bool MXCHIP::NetworkReconnect(bool ENABLE, uint8_t id)
{
    if(ENABLE == 0){
        if(!(_parser.send("AT+CIPAUTOCONN=%d,0", id)
            &&_parser.recv("OK")))
        return false;
    }else{
        if(!(_parser.send("AT+CIPAUTOCONN=%d,1", id)
            &&_parser.recv("OK")))
        return false;
    }
    return true;
}

int MXCHIP::open(const char *type, int id, const char* addr, int port)
{
    if(id>4)
        return false;

    int socketId = -1;
    /*There are some problem. when start CIPSTART.*/
    if (!(_parser.send("AT+CIPSTART=%d,%s,%s,%d,%d", id, type, addr, port,id+20001)
            && _parser.recv("OK")))
        return false;
    ThisThread::sleep_for(1s);
    if(!_parser.recv("+CIPEVENT:%d,%*[^,],CONNECTED",&socketId)) {
        return false;
    }
    return true;
}

bool MXCHIP::send(int id, const void *data, uint32_t amount)
{
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (_parser.send("AT+CIPSEND=%d,%d", id, amount)
            && _parser.write((char*)data, (int)amount) >= 0
            && _parser.recv("OK")) {
            //wait(3);
            return true;
        }
    }
    return false;
}

void MXCHIP::_packet_handler()
{
    int id;
    uint32_t amount;
    // parse out the packet
    if (!_parser.recv(",%d,%d,", &id, &amount)) {
        return;
    }

    struct packet *packet = (struct packet*)malloc(
            sizeof(struct packet) + amount);
    if (!packet) {
        return;
    }

    packet->id = id;
    packet->len = amount;
    packet->next = 0;

    if (!(_parser.read((char*)(packet + 1), amount))) {
        free(packet);
        return;
    }

    // append to packet list
    *_packets_end = packet;
    _packets_end = &packet->next;
}

int32_t MXCHIP::recv(int id, void *data, uint32_t amount)
{
    while (true) {
        // check if any packets are ready for us
        for (struct packet **p = &_packets; *p; p = &(*p)->next) {
            if ((*p)->id == id) {
                struct packet *q = *p;

                if (q->len <= amount) { // Return and remove full packet
                    memcpy(data, q+1, q->len);
                    if (_packets_end == &(*p)->next) {
                        _packets_end = p;
                    }
                    *p = (*p)->next;

                    uint32_t len = q->len;
                    free(q);
                    return len;
                } else { // return only partial packet
                    memcpy(data, q+1, amount);

                    q->len -= amount;
                    memmove(q+1, (uint8_t*)(q+1) + amount, q->len);

                    return amount;
                }
            }
        }

        // Wait for inbound packet
        if (!_parser.recv("\r\n")) {
            return -1;
        }
        else {
            _parser.recv("\r\n");
        }
    }
}

bool MXCHIP::close(int id)
{
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (_parser.send("AT+CIPSTOP=%d",id)
            && _parser.recv("OK")) {
            if (_parser.recv("+CIPEVENT:%*[^,],%*[^,],CLOSED"))
                return true;
        }
    }
    return false;
}


void MXCHIP::setTimeout(uint32_t timeout_ms)
{
    _parser.setTimeout(timeout_ms);
}

bool MXCHIP::readable()
{
    return _serial.readable();
}

bool MXCHIP::writeable()
{
    return _serial.writable();
}

void MXCHIP::attach(Callback<void()> func)
{
    _serial.sigio(func);
}

 bool MXCHIP::mqtt_enable_event(bool enabled)
 {
    return _parser.send("AT+MQTTEVENT=%s\r", enabled ? "ON":"OFF")
        && _parser.recv("OK");
 }

bool MXCHIP::mqtt_set_auth(const char *username, const char *password)
{
    return _parser.send("AT+MQTTAUTH=%s,%s\r", username, password)
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_set_socket(const char *host, int port)
{
    return _parser.send("AT+MQTTSOCK=%s,%d\r", host, port)
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_enable_cert(bool root_enabled, bool client_enabled)
{
    return _parser.send("AT+MQTTCAVERIFY=%s,%s\r", root_enabled ? "ON": "OFF", client_enabled ? "ON": "OFF")
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_enable_ssl(bool enabled)
{
    return _parser.send("AT+MQTTSSL=%s\r", enabled ? "ON":"OFF")
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_set_client_id(const char *client_id)
{
    return _parser.send("AT+MQTTCID=%s\r", client_id)
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_set_keepalive_interval(int seconds)
{
    return _parser.send("AT+MQTTKEEPALIVE=%d\r", seconds)
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_enable_reconnect(bool enabled)
{
    return _parser.send("AT+MQTTRECONN=%s\r", enabled ? "ON":"OFF")
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_start()
{
    if (!(_parser.send("AT+MQTTSTART\r") && _parser.recv("OK")))
        return false;
    ThisThread::sleep_for(1s);
    return _parser.recv("+MQTTEVENT:CONNECT,SUCCESS");
}

bool MXCHIP::mqtt_poll(MQTTMessage &msg)
{
    if(_parser.recv("+MQTTRECV:%d,%u,", &msg.id, &msg.length) && msg.length > 0)
    {
        return _parser.read(msg.message, msg.length) > 0;
    }
    return false;
}

bool MXCHIP::mqtt_subscribe(int id, const char *topic, int qos)
{
    if(id < 0 || id > 5) return false;
    if(qos < 0 || qos > 2) return false;
    if(!_parser.send("AT+MQTTSUB=%d,%s,%d\r", id, topic, qos))
        return false;
    ThisThread::sleep_for(1s);
    return _parser.recv("+MQTTEVENT:0,SUBSCRIBE,SUCCESS");
}

bool MXCHIP::mqtt_publish_setup(const char *topic, int qos)
{
    if(qos < 0 || qos > 2) return false;
    if(!_parser.send("AT+MQTTPUB=%s,%d\r", topic, qos))
        return false;
    ThisThread::sleep_for(1s);
    return _parser.recv("OK");
}

bool MXCHIP::mqtt_publish_send(const char *data, size_t len)
{
    if (!(_parser.send("\rAT+MQTTSEND=%u", len) && _parser.write(data, len) > 0))
        return false;
    return _parser.recv("+MQTTEVENT:PUBLISH,SUCCESS");
}

bool MXCHIP::mqtt_unsubscribe(int id)
{
    return _parser.send("AT+MQTTUNSUB=%d\r", id)
        && _parser.recv("OK");
}

bool MXCHIP::mqtt_close()
{
    return _parser.send("AT+MQTTCLOSE\r")
        && _parser.recv("OK");
}
