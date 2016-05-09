/*
The MIT License (MIT)

Copyright (c) 2013-2015 SRS(ossrs)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SRS_APP_LISTENER_HPP
#define SRS_APP_LISTENER_HPP

/*
#include <app_listener.hpp>
*/

#include <core.hpp>

#include <string>

#include <app_st.hpp>
#include <app_thread.hpp>

struct sockaddr_in;

/**
* the udp packet handler.
*/
class IUdpHandler
{
public:
    IUdpHandler();
    virtual ~IUdpHandler();
public:
    /**
     * when fd changed, for instance, reload the listen port,
     * notify the handler and user can do something.
     */
    virtual int on_stfd_change(st_netfd_t fd);
public:
    /**
    * when udp listener got a udp packet, notice server to process it.
    * @param type, the client type, used to create concrete connection,
    *       for instance RTMP connection to serve client.
    * @param from, the udp packet from address.
    * @param buf, the udp packet bytes, user should copy if need to use.
    * @param nb_buf, the size of udp packet bytes.
    * @remark user should never use the buf, for it's a shared memory bytes.
    */
    virtual int on_udp_packet(sockaddr_in* from, char* buf, int nb_buf) = 0;
};

/**
* the tcp connection handler.
*/
class ITcpHandler
{
public:
    ITcpHandler();
    virtual ~ITcpHandler();
public:
    /**
    * when got tcp client.
    */
    virtual int on_tcp_client(st_netfd_t stfd) = 0;
};

/**
* bind udp port, start thread to recv packet and handler it.
*/
class CUdpHandler : public IReusableThreadHandler
{
private:
    int _fd;
    st_netfd_t _stfd;
    CReusableThread* pthread;
private:
    char* buf;
    int nb_buf;
private:
    IUdpHandler* handler;
    std::string ip;
    int port;
public:
    CUdpHandler(IUdpHandler* h, std::string i, int p);
    virtual ~CUdpHandler();
public:
    virtual int fd();
    virtual st_netfd_t stfd();
public:
    virtual int listen();
// interface IReusableThreadHandler.
public:
    virtual int cycle();
};

/**
* bind and listen tcp port, use handler to process the client.
*/
class CTcpHandler : public IReusableThreadHandler
{
private:
    int _fd;
    st_netfd_t _stfd;
    CReusableThread* pthread;
private:
    ITcpHandler* handler;
    std::string ip;
    int port;
public:
    CTcpHandler(ITcpHandler* h, std::string i, int p);
    virtual ~CTcpHandler();
public:
    virtual int fd();
public:
    virtual int listen();
// interface IReusableThreadHandler.
public:
    virtual int cycle();
};

#endif
