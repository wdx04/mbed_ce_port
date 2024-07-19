/*  MW31 Example
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

#ifndef MW31_H
#define MW31_H

#include "ATParser.h"

/** MW31Interface class.
    This is an interface to a MW31 radio.
 */
class MW31
{
public:
    MW31(PinName tx, PinName rx, bool debug=false);

    /**
    * Startup the MW31
    *
    * @param mode mode of WIFI 1-client, 2-host, 3-both
    * @return true only if MW31 was setup correctly
    */
    bool startup();

    /**
    * Enable/Disable DHCP
    *
    * @param enabled DHCP enabled when true
    *
    * @return true only if MW31 enables/disables DHCP successfully
    */
    bool dhcp(bool enabled);

    /**
    * Connect MW31 to AP
    *
    * @param ap the name of the AP
    * @param passPhrase the password of AP
    * @return true only if MW31 is connected successfully
    */
    bool connect(const char *ap, const char *passPhrase);

    /**
    * Disconnect MW31 from AP
    *
    * @return true only if MW31 is disconnected successfully
    */
    bool disconnect(void);

    /**
    * Get the IP address of MW31
    *
    * @return null-teriminated IP address or null if no IP address is assigned
    */
    const char *getIPAddress(void);

    /**
    * Get the MAC address of MW31
    *
    * @return null-terminated MAC address or null if no MAC address is assigned
    */
    const char *getMACAddress(void);

    /*Get current signal strength.
     *
     * @return the network's signal strength
     */
    int8_t getRSSI();

    /**
    * Check if MW31 is conenected
    *
    * @return true only if the chip has an IP address
    */
    bool isConnected(void);
    
    /**
    * Enable network automatic reconnect mode
    *
    *@param ENABLE network automatic reconnect mode enabled when true
    *@param id id to reconnect the socket, valid 0-4    
    * @return true when success
    */
    bool NetworkReconnect(bool ENABLE, uint8_t id);

    /**
    * Open a socketed connection
    *
    * @param type the type of socket to open "UDP" or "TCP"
    * @param id id to give the new socket, valid 0-4
    * @param port port to open connection with
    * @param addr the IP address of the destination
    * @return true only if socket opened successfully
    */
    int open(const char *type, int id, const char* addr, int port);

    /**
    * Sends data to an open socket
    *
    * @param id id of socket to send to
    * @param data data to be sent
    * @param amount amount of data to be sent - max 1024
    * @return true only if data sent successfully
    */
    bool send(int id, const void *data, uint32_t amount);

    /**
    * Receives data from an open socket
    *
    * @param id id to receive from
    * @param data placeholder for returned information
    * @param amount number of bytes to be received
    * @return the number of bytes received
    */
    int32_t recv(int id, void *data, uint32_t amount);

    /**
    * Closes a socket
    *
    * @param id id of socket to close, valid only 0-4
    * @return true only if socket is closed successfully
    */
    bool close(int id);

    /**
    * Allows timeout to be changed between commands
    *
    * @param timeout_ms timeout of the connection
    */
    void setTimeout(uint32_t timeout_ms);

    /**
    * Checks if data is available
    */
    bool readable();

    /**
    * Checks if data can be written
    */
    bool writeable();

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param func A pointer to a void function, or 0 to set as none
    */
    void attach(Callback<void()> func);

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param obj pointer to the object to call the member function on
    * @param method pointer to the member function to call
    */
    template <typename T, typename M>
    void attach(T *obj, M method) {
        attach(Callback<void()>(obj, method));
    }

private:
    BufferedSerial _serial;
    ATParser _parser;

    struct packet {
        struct packet *next;
        int id;
        uint32_t len;
        // data follows
    } *_packets, **_packets_end;
    void _packet_handler();

    char _ip_buffer[16];
    char _mac_buffer[18];
};

#endif
