/*  MXCHIP Example
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

#ifndef MXCHIP_H
#define MXCHIP_H

#include "ATParser.h"

constexpr size_t MQTT_MAX_MESSAGE_LEN = 248;

struct MQTTMessage
{
    int id = 0;
    size_t length = 0;
    char message[MQTT_MAX_MESSAGE_LEN];
};

/** MXCHIPInterface class.
    This is an interface to a MXCHIP radio.
 */
class MXCHIP
{
public:
    MXCHIP(PinName tx, PinName rx, bool debug=false);

    /**
    * Startup the MXCHIP
    *
    * @param mode mode of WIFI 1-client, 2-host, 3-both
    * @return true only if MXCHIP was setup correctly
    */
    bool startup();

    /**
    * Enable/Disable DHCP
    *
    * @param enabled DHCP enabled when true
    *
    * @return true only if MXCHIP enables/disables DHCP successfully
    */
    bool dhcp(bool enabled);

    /**
    * Connect MXCHIP to AP
    *
    * @param ap the name of the AP
    * @param passPhrase the password of AP
    * @return true only if MXCHIP is connected successfully
    */
    bool connect(const char *ap, const char *passPhrase);

    /**
    * Disconnect MXCHIP from AP
    *
    * @return true only if MXCHIP is disconnected successfully
    */
    bool disconnect(void);

    /**
    * Get the IP address of MXCHIP
    *
    * @return null-teriminated IP address or null if no IP address is assigned
    */
    const char *getIPAddress(void);

    /**
    * Get the MAC address of MXCHIP
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
    * Check if MXCHIP is conenected
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

    /**
    * Enable/Disable MQTT events
    *
    * @param enabled MQTT events enabled when true
    *
    * @return true only if MXCHIP enables/disables MQTT events successfully
    */
    bool mqtt_enable_event(bool enabled);

    /**
    * Set MQTT broker login params
    *
    * @param username MQTT broker user name
    * @param password MQTT broker password
    *
    * @return true only if MXCHIP sets MQTT broker login params successfully
    */
    bool mqtt_set_auth(const char *username, const char *password);

    /**
    * Set MQTT broker address
    *
    * @param host MQTT broker host name
    * @param port MQTT broker listen port
    *
    * @return true only if MXCHIP sets MQTT broker address successfully
    */
    bool mqtt_set_socket(const char *host, int port);

    /**
    * Enable/Disable MQTT Root or Client certs
    *
    * @param root_enabled Enable Root certification when true
    * @param client_enabled Enable Client certification when true
    *
    * @return true only if MXCHIP enable/disables MQTT root or client certs successfully
    */
    bool mqtt_enable_cert(bool root_enabled, bool client_enabled);

    /**
    * Enable/Disable MQTT SSL connection
    *
    * @param enabled Enable Root certification 
    *
    * @return true only if MXCHIP enable/disables MQTT SSL connection successfully
    */
    bool mqtt_enable_ssl(bool enabled);

    /**
    * Set MQTT Client ID
    *
    * @param client_id MQTT Client ID string 
    *
    * @return true only if MXCHIP sets MQTT Client ID successfully
    */
    bool mqtt_set_client_id(const char *client_id);

    /**
    * Set MQTT Keep-Alive interval
    *
    * @param seconds MQTT Keep-Alive interval in seconds
    *
    * @return true only if MXCHIP sets MQTT Keep-Alive interval successfully
    */
    bool mqtt_set_keepalive_interval(int seconds);

    /**
    * Enable/Disable MQTT auto reconnect
    *
    * @param enabled Enable MQTT auto reconnect when true
    *
    * @return true only if MXCHIP enable/disables MQTT auto reconnect successfully
    */
    bool mqtt_enable_reconnect(bool enabled);

    /**
    * Start MQTT service
    * Can only subscribe or publish messages after MQTT service is started.
    *
    * @return true only if MXCHIP starts MQTT service successfully
    */
    bool mqtt_start();

    /**
    * Subscribe MQTT topic
    *
    * @param id Internal MQTT topic ID, can be 0~5
    * @param topic MQTT topic name to subscribe
    * @param qos QOS level of MQTT subscription, can be 0~2
    *
    * @return true only if MXCHIP subscribes MQTT topic successfully
    */
    bool mqtt_subscribe(int id, const char *topic, int qos);

    /**
    * Setup MQTT publish params
    *
    * @param topic MQTT topic name to publish
    * @param qos QOS level of MQTT topic, can be 0~2
    *
    * @return true only if MXCHIP setups MQTT publish params successfully
    */
    bool mqtt_publish_setup(const char *topic, int qos);

    /**
    * Publish MQTT messages to the topic set by mqtt_publish_setup
    *
    * @param data pointer to message data
    * @param len length of message data
    *
    * @return true only if MXCHIP publishes MQTT message successfully
    */
    bool mqtt_publish_send(const char *data, size_t len);

    /**
    * Query the subscribed MQTT message
    *
    * @param msg received message data
    *
    * @return true only if MXCHIP receives subscribed MQTT message successfully
    */
    bool mqtt_poll(MQTTMessage &msg);

    /**
    * Unsubscribe MQTT topic
    *
    * @param id Internal MQTT topic ID to unsubscribe
    *
    * @return true only if MXCHIP unsubscribes MQTT topic successfully
    */
    bool mqtt_unsubscribe(int id);

    /**
    * Stop MQTT service
    *
    * @return true only if MXCHIP stops MQTT service successfully
    */
    bool mqtt_close();

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
