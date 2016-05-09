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

#ifndef SRS_APP_SERVER_HPP
#define SRS_APP_SERVER_HPP

/*
#include <srs_app_server.hpp>
*/

//#include <srs_core.hpp>

#include <vector>
#include <string>

#include <app_reload.hpp>
//#include <srs_app_source.hpp>
#include <app_listener.hpp>
#include <app_conn.hpp>
#include <app_st.hpp>

class CServer;
class CConnection;
//class SrsHttpServer;
//class CHttpHeartbeat;
//class SrsKbps;
class ConfDirective;
class ITcpHandler;
class IUdpHandler;
class CUdpHandler;
class CTcpHandler;


// listener type for server to identify the connection,
// that is, use different type to process the connection.
enum ListenerType
{
//    // RTMP client,
//    SrsListenerRtmpStream       = 0,
//    // HTTP api,
//    SrsListenerHttpApi          = 1,
//    // HTTP stream, HDS/HLS/DASH
//    SrsListenerHttpStream       = 2,
//    // UDP stream, MPEG-TS over udp.
//    SrsListenerMpegTsOverUdp    = 3,
//    // TCP stream, RTSP stream.
//    SrsListenerRtsp             = 4,
//    // TCP stream, FLV stream over HTTP.
//    SrsListenerFlv              = 5,

    ListenerIM = 0
    ,ListenerHttp
};

/**
* the common tcp listener, for RTMP/HTTP server.
*/
class CListener
{
protected:
    ListenerType type;
protected:
    std::string ip;
    int port;
    CServer* server;
public:
    CListener(CServer* svr, ListenerType t);
    virtual ~CListener();
public:
    virtual ListenerType listen_type();
    virtual int listen(std::string i, int p) = 0;
};

/**
* tcp listener.
*/
class CTcpListener : virtual public CListener, virtual public ITcpHandler
{
private:
	CTcpHandler* tcpHandler;
public:
	CTcpListener(CServer* server, ListenerType type);
    virtual ~CTcpListener();
public:
    virtual int listen(std::string ip, int port);
// ITcpHandler
public:
    virtual int on_tcp_client(st_netfd_t stfd);
};

/**
* convert signal to io,
* @see: st-1.9/docs/notes.html
*/
class CSignalManager : public IEndlessThreadHandler
{
private:
    /* Per-process pipe which is used as a signal queue. */
    /* Up to PIPE_BUF/sizeof(int) signals can be queued up. */
    int sig_pipe[2];
    st_netfd_t signal_read_stfd;
private:
    CServer* _server;
    CEndlessThread* pthread;
public:
    CSignalManager(CServer* server);
    virtual ~CSignalManager();
public:
    virtual int initialize();
    virtual int start();
// interface ISrsEndlessThreadHandler.
public:
    virtual int cycle();
private:
    // global singleton instance
    static CSignalManager* instance;
    /* Signal catching function. */
    /* Converts signal event to I/O event. */
    static void sig_catcher(int signo);
};

/**
* the handler to the handle cycle in SRS RTMP server.
*/
class IServerCycle
{
public:
	IServerCycle();
    virtual ~IServerCycle();
public: 
    /**
    * initialize the cycle handler.
    */
    virtual int initialize() = 0;
    /**
    * do on_cycle while server doing cycle.
    */
    virtual int on_cycle(int connections) = 0;
};

/**
* SRS RTMP server, initialize and listen, 
* start connection service thread, destroy client.
*/
class CServer : virtual public IReloadHandler
    , virtual public IConnectionManager
{
private:
#ifdef SRS_AUTO_HTTP_CORE
//    CHttpHeartbeat* http_heartbeat;
#endif

private:
    /**
    * the pid file fd, lock the file write when server is running.
    * @remark the init.d script should cleanup the pid file, when stop service,
    *       for the server never delete the file; when system startup, the pid in pid file
    *       maybe valid but the process is not SRS, the init.d script will never start server.
    */
    int pid_fd;
    /**
    * all connections, connection manager
    */
    std::vector<CConnection*> conns;
    /**
    * all listners, listener manager.
    */
    std::vector<CListener*> listeners;
    /**
    * signal manager which convert gignal to io message.
    */
    CSignalManager* signal_manager;
    /**
    * handle in server cycle.
    */
    IServerCycle* handler;
    /**
    * user send the signal, convert to variable.
    */
    bool signal_reload;
    bool signal_gmc_stop;
    bool signal_gracefully_quit;
public:
    CServer();
    virtual ~CServer();
private:
    /**
    * the destroy is for gmc to analysis the memory leak,
    * if not destroy global/static data, the gmc will warning memory leak.
    * in service, server never destroy, directly exit when restart.
    */
    virtual void destroy();
    /**
     * when SIGTERM, SRS should do cleanup, for example, 
     * to stop all ingesters, cleanup HLS and dvr.
     */
    virtual void dispose();
// server startup workflow, @see run_master()
public:
    /**
     * initialize server with callback handler.
     * @remark user must free the cycle handler.
     */
    virtual int initialize(IServerCycle* cycle_handler);
    virtual int initialize_st();
    virtual int initialize_signal();
    virtual int acquire_pid_file();
    virtual int listen();
    virtual int register_signal();
    virtual int http_handle();
    //virtual int ingest();
    virtual int cycle();
// IConnectionManager
public:
    /**
    * callback for connection to remove itself.
    * when connection thread cycle terminated, callback this to delete connection.
    * @see CConnection.on_thread_stop().
    */
    virtual void remove(CConnection* conn);
// server utilities.
public:
    /**
    * callback for signal manager got a signal.
    * the signal manager convert signal to io message,
    * whatever, we will got the signo like the orignal signal(int signo) handler.
    * @remark, direclty exit for SIGTERM.
    * @remark, do reload for SIGNAL_RELOAD.
    * @remark, for SIGINT and SIGUSR2:
    *       no gmc, directly exit.
    *       for gmc, set the variable signal_gmc_stop, the cycle will return and cleanup for gmc.
    */
    virtual void on_signal(int signo);
private:
    /**
    * the server thread main cycle,
    * update the global static data, for instance, the current time,
    * the cpu/mem/network statistic.
    */
    virtual int do_cycle();
    /**
    * listen at specified protocol.
    */
    virtual int listen_im();
    virtual int listen_http_server();
    /**
    * close the listeners for specified type, 
    * remove the listen object from manager.
    */
    virtual void close_listeners(ListenerType type);
    /**
    * resample the server kbs.
    */
    virtual void resample_kbps();
// internal only
public:
    /**
    * when listener got a fd, notice server to accept it.
    * @param type, the client type, used to create concrete connection, 
    *       for instance RTMP connection to serve client.
    * @param client_stfd, the client fd in st boxed, the underlayer fd.
    */
    virtual int accept_client(ListenerType type, st_netfd_t client_stfd);
// interface IReloadHandler.
public:
    virtual int on_reload_listen();
    virtual int on_reload_pid();
};

#endif

