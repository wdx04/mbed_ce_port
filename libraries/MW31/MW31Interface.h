/*  MW31Interface Example
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

#ifndef MW31_INTERFACE_H
#define MW31_INTERFACE_H

#include "MW31.h"
#include "mbed.h"

#define MW31_SOCKET_COUNT 5

/** MW31Interface class
 *  Implementation of the NetworkStack for the MW31
 */
class MW31Interface : public NetworkStack, public WiFiInterface
{
public:
    /** MW31Interface lifetime
     * @param tx        TX pin
     * @param rx        RX pin
     * @param debug     Enable debugging
     */
    MW31Interface(PinName tx, PinName rx, bool debug = false);

    /** Start the interface
     *
     *  Attempts to connect to a WiFi network. If passphrase is invalid,
     *  NSAPI_ERROR_AUTH_ERROR is returned.
     *
     *  @param ssid      Name of the network to connect to
     *  @param pass      Security passphrase to connect to the network
     *  @param security  Type of encryption for connection
     *  @return          0 on success, negative error code on failure
     */
    virtual int connect(const char *ssid, const char *pass, nsapi_security_t security = NSAPI_SECURITY_NONE,
                                 uint8_t channel = 0);

    /** Start the interface
     *
     *  Attempts to connect to a WiFi network. Requires ssid and passphrase to be set.
     *  If passphrase is invalid, NSAPI_ERROR_AUTH_ERROR is returned.
     *
     *  @return         0 on success, negative error code on failure
     */
    virtual int connect();

    /** Set the WiFi network credentials
     *
     *  @param ssid      Name of the network to connect to
     *  @param pass      Security passphrase to connect to the network
     *  @param security  Type of encryption for connection
     *                   (defaults to NSAPI_SECURITY_NONE)
     *  @return          0 on success, or error code on failure
     */
    virtual int set_credentials(const char *ssid, const char *pass,
    									nsapi_security_t security = NSAPI_SECURITY_NONE);

    /** Set the WiFi network channel
     *
     * This function is not supported and will return NSAPI_ERROR_UNSUPPORTED
     *
     *	@param channel	 Channel on which the connection is to be made, or 0 for any (Default: 0)
     *	@return 		 true only if set channel succesfully.
     */
    virtual int set_channel(uint8_t channel);

    /** Stop the interface
     *  @return             0 on success, negative on failure
     */
    virtual int disconnect();

    /** Get the internally stored IP address
     *  @return             IP address of the interface or null if not yet connected
     */
    virtual const char *get_ip_address();

    /** Get the internally stored MAC address
     *  @return             MAC address of the interface
     */
    virtual const char *get_mac_address();

    /** Get the local gateway
    *
    *  @return         Null-terminated representation of the local gateway
    *                  or null if no network mask has been recieved
    */
   virtual const char *get_gateway();

   /** Get the local network mask
    *
    *  @return         Null-terminated representation of the local network mask
    *                  or null if no network mask has been recieved
    */
   virtual const char *get_netmask();

    /** Gets the current radio signal strength for active connection
     *
     * @return          Connection strength in dBm (negative value)
     */
    virtual int8_t get_rssi();

    /** Scan for available networks
     *
     * This function is not supported and will return NSAPI_ERROR_UNSUPPORTED
     *
     * @param  ap       Pointer to allocated array to store discovered AP
     * @param  count    Size of allocated @a res array, or 0 to only count available AP
     * @return          Number of entries in @a, or if @a count was 0 number of available networks, negative on error
     *                  see @a nsapi_error
     */
   virtual int scan(WiFiAccessPoint *ap, unsigned count);

    /**
    * Enable network automatic reconnect mode
    *
    *@param ENABLE network automatic reconnect mode enabled when true
    *@param id id to reconnect the socket, valid 0-4    
    * @return true when success
    */
    bool network_reconnect(bool ENABLE, uint8_t id);

protected:
    /** Open a socket
     *  @param handle       Handle in which to store new socket
     *  @param proto        Type of socket to open, NSAPI_TCP or NSAPI_UDP
     *  @return             0 on success, negative on failure
     */
    virtual int socket_open(void **handle, nsapi_protocol_t proto);

    /** Close the socket
     *  @param handle       Socket handle
     *  @return             0 on success, negative on failure
     *  @note On failure, any memory associated with the socket must still
     *        be cleaned up
     */
    virtual int socket_close(void *handle);

    /** Bind a server socket to a specific port
     *  @param handle       Socket handle
     *  @param address      Local address to listen for incoming connections on
     *  @return             0 on success, negative on failure.
     */
    virtual int socket_bind(void *handle, const SocketAddress &address);

    /** Start listening for incoming connections
     *  @param handle       Socket handle
     *  @param backlog      Number of pending connections that can be queued up at any
     *                      one time [Default: 1]
     *  @return             0 on success, negative on failure
     */
    virtual int socket_listen(void *handle, int backlog);

    /** Connects this TCP socket to the server
     *  @param handle       Socket handle
     *  @param address      SocketAddress to connect to
     *  @return             0 on success, negative on failure
     */
    virtual int socket_connect(void *handle, const SocketAddress &address);

    /** Accept a new connection.
     *  @param handle       Handle in which to store new socket
     *  @param server       Socket handle to server to accept from
     *  @return             0 on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_accept(void *server, void **handle, SocketAddress *addr );

    /** Send data to the remote host
     *  @param handle       Socket handle
     *  @param data         The buffer to send to the host
     *  @param size         The length of the buffer to send
     *  @return             Number of written bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_send(void *handle, const void *data, unsigned size);

    /** Receive data from the remote host
     *  @param handle       Socket handle
     *  @param data         The buffer in which to store the data received from the host
     *  @param size         The maximum length of the buffer
     *  @return             Number of received bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_recv(void *handle, void *data, unsigned size);

    /** Send a packet to a remote endpoint
     *  @param handle       Socket handle
     *  @param address      The remote SocketAddress
     *  @param data         The packet to be sent
     *  @param size         The length of the packet to be sent
     *  @return             The number of written bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size);

    /** Receive a packet from a remote endpoint
     *  @param handle       Socket handle
     *  @param address      Destination for the remote SocketAddress or null
     *  @param buffer       The buffer for storing the incoming packet data
     *                      If a packet is too long to fit in the supplied buffer,
     *                      excess bytes are discarded
     *  @param size         The length of the buffer
     *  @return             The number of received bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size);

    /** Register a callback on state change of the socket
     *  @param handle       Socket handle
     *  @param callback     Function to call on state change
     *  @param data         Argument to pass to callback
     *  @note Callback may be called in an interrupt context.
     */
    virtual void socket_attach(void *handle, void (*callback)(void *), void *data);

    /** Provide access to the NetworkStack object
     *
     *  @return The underlying NetworkStack object
     */
    virtual NetworkStack *get_stack()
    {
        return this;
    }

private:
    MW31 _mxchip;
    bool _ids[MW31_SOCKET_COUNT];
    uint8_t _channel;

    char ap_ssid[33]; /* 32 is what 802.11 defines as longest possible name; +1 for the \0 */
    nsapi_security_t ap_sec;
    uint8_t ap_ch;
    char ap_pass[64]; /* The longest allowed passphrase */

    void event();
    struct {
        void (*callback)(void *);
        void *data;
    } _cbs[MW31_SOCKET_COUNT];
};


#endif
