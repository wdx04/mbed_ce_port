/* MW31 Example
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

#include "MW31.h"

MW31::MW31(PinName tx, PinName rx, bool debug)
    : _serial(tx, rx, 115200), _parser(_serial, "\r\n", "\r")
    , _packets(0), _packets_end(&_packets)
{
    _parser.debugOn(debug);
}

bool MW31::startup(void)
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
    _parser.oob("+CIPEVENT:SOCKET", this, &MW31::_packet_handler);
    return true;
}

bool MW31::dhcp(bool enabled)
{
    return _parser.send("AT+WDHCP=%s", enabled ? "ON":"OFF")
        && _parser.recv("OK");
}

bool MW31::connect(const char *ap, const char *passPhrase)
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

bool MW31::disconnect(void)
{
    return _parser.send("AT+WJAPQ")
        && _parser.recv("OK");
}

const char *MW31::getIPAddress(void)
{
    if (!(_parser.send("AT+WJAPIP?")
        && _parser.recv("+WJAPIP:%[^,],%*[^,],%*[^,],%*[^\r]%*[\r]%*[\n]", _ip_buffer))) {
        return NULL;
    }
    return _ip_buffer;
}

const char *MW31::getMACAddress(void)
{
    if (!(_parser.send("AT+WMAC?")
        && _parser.recv("+WMAC:%[^\r]%*[\r]%*[\n]", _mac_buffer))) {
        return 0;
    }
    return _mac_buffer;
}

//get current signal strength
int8_t MW31::getRSSI()
{
    int8_t rssi = 0;
    if (!(_parser.send("AT+WJAP?")
        && _parser.recv("+WJAP:%*[^,],%*[^,],%*[^,],%[^\r]%*[\r]%*[\n]", &rssi))) {
        return false;
    }
    return rssi;
}

bool MW31::isConnected(void)
{
    return getIPAddress() != 0;
}

bool MW31::NetworkReconnect(bool ENABLE, uint8_t id)
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

int MW31::open(const char *type, int id, const char* addr, int port)
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

bool MW31::send(int id, const void *data, uint32_t amount)
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

void MW31::_packet_handler()
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

int32_t MW31::recv(int id, void *data, uint32_t amount)
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

bool MW31::close(int id)
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


void MW31::setTimeout(uint32_t timeout_ms)
{
    _parser.setTimeout(timeout_ms);
}

bool MW31::readable()
{
    return _serial.readable();
}

bool MW31::writeable()
{
    return _serial.writable();
}

void MW31::attach(Callback<void()> func)
{
    _serial.sigio(func);
}
