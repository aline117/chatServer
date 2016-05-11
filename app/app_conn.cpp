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

#include <kernel_log.hpp>
#include <kernel_error.hpp>
#include <app_utility.hpp>
#include <app_server.hpp>
#include "app_conn.hpp"
#include "io.hpp"

IConnectionManager::IConnectionManager()
{
}

IConnectionManager::~IConnectionManager()
{
}

CConnection::CConnection(IConnectionManager* cm, st_netfd_t c)
{
    id = 0;
    manager = cm;
    stfd = c;
    disposed = false;
    expired = false;

    // the client thread should reap itself,
    // so we never use joinable.
    // TODO: FIXME: maybe other thread need to stop it.
    // @see: https://github.com/ossrs/srs/issues/78
    pthread = new COneCycleThread("conn", this);
}

CConnection::~CConnection()
{
    dispose();

    srs_freep(pthread);
}

void CConnection::dispose()
{
    if (disposed) {
        return;
    }

    disposed = true;

    /**
     * when delete the connection, stop the connection,
     * close the underlayer socket, delete the thread.
     */
    srs_close_stfd(stfd);
}

int CConnection::start()
{
    return pthread->start();
}

int CConnection::cycle()
{
    int ret = ERROR_SUCCESS;

    _thContext->generate_id();
    id = _thContext->get_id();

    ip = srs_get_peer_ip(st_netfd_fileno(stfd));

    ret = do_cycle();

    // if socket io error, set to closed.
    if (srs_is_client_gracefully_close(ret)) {
        ret = ERROR_SOCKET_CLOSED;
    }

    // success.
    if (ret == ERROR_SUCCESS) {
        srs_trace("client finished.");
    }

    // client close peer.
    if (ret == ERROR_SOCKET_CLOSED) {
        srs_warn("client disconnect peer. ret=%d", ret);
    }

    return ERROR_SUCCESS;
}

void CConnection::on_thread_stop()
{
    // TODO: FIXME: never remove itself, use isolate thread to do cleanup.
    manager->remove(this);
}

int CConnection::srs_id()
{
    return id;
}

void CConnection::expire()
{
    expired = true;
}


IMConn::IMConn(CServer* svr, st_netfd_t c)
    : CConnection(svr, c)
{
}

IMConn::~IMConn()
{
}

int IMConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    StSocket skt(stfd);

    while (!disposed) {
        //ret = stream_service_cycle();

        //skt.write("",12,ewr);

        // stream service must terminated with error, never success.
        // when terminated with success, it's user required to stop.
        if (ret == ERROR_SUCCESS) {
            continue;
        }

        // for other system control message, fatal error.
        srs_error("control message(%d) reject as error. ret=%d", ret, ret);
        return ret;
    }

    return ret;
}

