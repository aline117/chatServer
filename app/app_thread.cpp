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

#include <kernel_error.hpp>
#include <kernel_log.hpp>
#include "app_thread.hpp"

namespace internal {
    IThreadHandler::IThreadHandler()
    {
    }
    
    IThreadHandler::~IThreadHandler()
    {
    }
    
    void IThreadHandler::on_thread_start()
    {
    }
    
    int IThreadHandler::on_before_cycle()
    {
        int ret = ERROR_SUCCESS;
        return ret;
    }
    
    int IThreadHandler::on_end_cycle()
    {
        int ret = ERROR_SUCCESS;
        return ret;
    }
    
    void IThreadHandler::on_thread_stop()
    {
    }
    
    CThread::CThread(const char* name, IThreadHandler* thread_handler, int64_t interval_us, bool joinable)
    {
        _name = name;
        handler = thread_handler;
        cycle_interval_us = interval_us;
        
        tid = NULL;
        loop = false;
        really_terminated = true;
        _cid = -1;
        _joinable = joinable;
        disposed = false;
        
        // in start(), the thread cycle method maybe stop and remove the thread itself,
        // and the thread start() is waiting for the _cid, and segment fault then.
        // @see https://github.com/ossrs/srs/issues/110
        // thread will set _cid, callback on_thread_start(), then wait for the can_run signal.
        can_run = false;
    }
    
    CThread::~CThread()
    {
        stop();
    }
    
    int CThread::cid()
    {
        return _cid;
    }
    
    int CThread::start()
    {
        int ret = ERROR_SUCCESS;
        
        if(tid) {
            srs_info("thread %s already running.", _name);
            return ret;
        }
        
        if((tid = st_thread_create(thread_fun, this, (_joinable? 1:0), 0)) == NULL){
            ret = ERROR_ST_CREATE_CYCLE_THREAD;
            srs_error("st_thread_create failed. ret=%d", ret);
            return ret;
        }
        
        disposed = false;
        // we set to loop to true for thread to run.
        loop = true;
        
        // wait for cid to ready, for parent thread to get the cid.
        while (_cid < 0) {
            st_usleep(10 * 1000);
        }
        
        // now, cycle thread can run.
        can_run = true;
        
        return ret;
    }
    
    void CThread::stop()
    {
        if (!tid) {
            return;
        }
        
        loop = false;
        
        dispose();
        
        _cid = -1;
        can_run = false;
        tid = NULL;        
    }
    
    bool CThread::can_loop()
    {
        return loop;
    }
    
    void CThread::stop_loop()
    {
        loop = false;
    }
    
    void CThread::dispose()
    {
        if (disposed) {
            return;
        }
        
        // the interrupt will cause the socket to read/write error,
        // which will terminate the cycle thread.
        st_thread_interrupt(tid);
        
        // when joinable, wait util quit.
        if (_joinable) {
            // wait the thread to exit.
            int ret = st_thread_join(tid, NULL);
            if (ret) {
                srs_warn("core: ignore join thread failed.");
            }
        }
        
        // wait the thread actually terminated.
        // sometimes the thread join return -1, for example,
        // when thread use st_recvfrom, the thread join return -1.
        // so here, we use a variable to ensure the thread stopped.
        // @remark even the thread not joinable, we must ensure the thread stopped when stop.
        while (!really_terminated) {
            st_usleep(10 * 1000);
            
            if (really_terminated) {
                break;
            }
            srs_warn("core: wait thread to actually terminated");
        }
        
        disposed = true;
    }
    
    void CThread::thread_cycle()
    {
        int ret = ERROR_SUCCESS;
        
        _thContext->generate_id();
        srs_info("thread %s cycle start", _name);
        
        _cid = _thContext->get_id();
        
        srs_assert(handler);
        handler->on_thread_start();
        
        // thread is running now.
        really_terminated = false;
        
        // wait for cid to ready, for parent thread to get the cid.
        while (!can_run && loop) {
            st_usleep(10 * 1000);
        }
        
        while (loop) {
            if ((ret = handler->on_before_cycle()) != ERROR_SUCCESS) {
                srs_warn("thread %s on before cycle failed, ignored and retry, ret=%d", _name, ret);
                goto failed;
            }
            srs_info("thread %s on before cycle success", _name);
            
            if ((ret = handler->cycle()) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret) && !srs_is_system_control_error(ret)) {
                    srs_warn("thread %s cycle failed, ignored and retry, ret=%d", _name, ret);
                }
                goto failed;
            }
            srs_info("thread %s cycle success", _name);
            
            if ((ret = handler->on_end_cycle()) != ERROR_SUCCESS) {
                srs_warn("thread %s on end cycle failed, ignored and retry, ret=%d", _name, ret);
                goto failed;
            }
            srs_info("thread %s on end cycle success", _name);
            
        failed:
            if (!loop) {
                break;
            }
            
            // to improve performance, donot sleep when interval is zero.
            // @see: https://github.com/ossrs/srs/issues/237
            if (cycle_interval_us != 0) {
                st_usleep(cycle_interval_us);
            }
        }
        
        // readly terminated now.
        really_terminated = true;
        
        handler->on_thread_stop();
        srs_info("thread %s cycle finished", _name);
    }
    
    void* CThread::thread_fun(void* arg)
    {
        CThread* obj = (CThread*)arg;
        srs_assert(obj);
        
        obj->thread_cycle();
        
        st_thread_exit(NULL);
        
        return NULL;
    }
}

IEndlessThreadHandler::IEndlessThreadHandler()
{
}

IEndlessThreadHandler::~IEndlessThreadHandler()
{
}

void IEndlessThreadHandler::on_thread_start()
{
}

int IEndlessThreadHandler::on_before_cycle()
{
    return ERROR_SUCCESS;
}

int IEndlessThreadHandler::on_end_cycle()
{
    return ERROR_SUCCESS;
}

void IEndlessThreadHandler::on_thread_stop()
{
}

CEndlessThread::CEndlessThread(const char* n, IEndlessThreadHandler* h)
{
    handler = h;
    pthread = new internal::CThread(n, this, 0, false);
}

CEndlessThread::~CEndlessThread()
{
    pthread->stop();
    srs_freep(pthread);
}

int CEndlessThread::start()
{
    return pthread->start();
}

int CEndlessThread::cycle()
{
    return handler->cycle();
}

void CEndlessThread::on_thread_start()
{
    handler->on_thread_start();
}

int CEndlessThread::on_before_cycle()
{
    return handler->on_before_cycle();
}

int CEndlessThread::on_end_cycle()
{
    return handler->on_end_cycle();
}

void CEndlessThread::on_thread_stop()
{
    handler->on_thread_stop();
}

IOneCycleThreadHandler::IOneCycleThreadHandler()
{
}

IOneCycleThreadHandler::~IOneCycleThreadHandler()
{
}

void IOneCycleThreadHandler::on_thread_start()
{
}

int IOneCycleThreadHandler::on_before_cycle()
{
    return ERROR_SUCCESS;
}

int IOneCycleThreadHandler::on_end_cycle()
{
    return ERROR_SUCCESS;
}

void IOneCycleThreadHandler::on_thread_stop()
{
}

COneCycleThread::COneCycleThread(const char* n, IOneCycleThreadHandler* h)
{
    handler = h;
    pthread = new internal::CThread(n, this, 0, false);
}

COneCycleThread::~COneCycleThread()
{
    pthread->stop();
    srs_freep(pthread);
}

int COneCycleThread::start()
{
    return pthread->start();
}

int COneCycleThread::cycle()
{
    int ret = handler->cycle();
    pthread->stop_loop();
    return ret;
}

void COneCycleThread::on_thread_start()
{
    handler->on_thread_start();
}

int COneCycleThread::on_before_cycle()
{
    return handler->on_before_cycle();
}

int COneCycleThread::on_end_cycle()
{
    return handler->on_end_cycle();
}

void COneCycleThread::on_thread_stop()
{
    handler->on_thread_stop();
}

IReusableThreadHandler::IReusableThreadHandler()
{
}

IReusableThreadHandler::~IReusableThreadHandler()
{
}

void IReusableThreadHandler::on_thread_start()
{
}

int IReusableThreadHandler::on_before_cycle()
{
    return ERROR_SUCCESS;
}

int IReusableThreadHandler::on_end_cycle()
{
    return ERROR_SUCCESS;
}

void IReusableThreadHandler::on_thread_stop()
{
}

CReusableThread::CReusableThread(const char* n, IReusableThreadHandler* h, int64_t interval_us)
{
    handler = h;
    pthread = new internal::CThread(n, this, interval_us, true);
}

CReusableThread::~CReusableThread()
{
    pthread->stop();
    srs_freep(pthread);
}

int CReusableThread::start()
{
    return pthread->start();
}

void CReusableThread::stop()
{
    pthread->stop();
}

int CReusableThread::cid()
{
    return pthread->cid();
}

int CReusableThread::cycle()
{
    return handler->cycle();
}

void CReusableThread::on_thread_start()
{
    handler->on_thread_start();
}

int CReusableThread::on_before_cycle()
{
    return handler->on_before_cycle();
}

int CReusableThread::on_end_cycle()
{
    return handler->on_end_cycle();
}

void CReusableThread::on_thread_stop()
{
    handler->on_thread_stop();
}

IReusableThread2Handler::IReusableThread2Handler()
{
}

IReusableThread2Handler::~IReusableThread2Handler()
{
}

void IReusableThread2Handler::on_thread_start()
{
}

int IReusableThread2Handler::on_before_cycle()
{
    return ERROR_SUCCESS;
}

int IReusableThread2Handler::on_end_cycle()
{
    return ERROR_SUCCESS;
}

void IReusableThread2Handler::on_thread_stop()
{
}

CReusableThread2::CReusableThread2(const char* n, IReusableThread2Handler* h, int64_t interval_us)
{
    handler = h;
    pthread = new internal::CThread(n, this, interval_us, true);
}

CReusableThread2::~CReusableThread2()
{
    pthread->stop();
    srs_freep(pthread);
}

int CReusableThread2::start()
{
    return pthread->start();
}

void CReusableThread2::stop()
{
    pthread->stop();
}

int CReusableThread2::cid()
{
    return pthread->cid();
}

void CReusableThread2::interrupt()
{
    pthread->stop_loop();
}

bool CReusableThread2::interrupted()
{
    return !pthread->can_loop();
}

int CReusableThread2::cycle()
{
    return handler->cycle();
}

void CReusableThread2::on_thread_start()
{
    handler->on_thread_start();
}

int CReusableThread2::on_before_cycle()
{
    return handler->on_before_cycle();
}

int CReusableThread2::on_end_cycle()
{
    return handler->on_end_cycle();
}

void CReusableThread2::on_thread_stop()
{
    handler->on_thread_stop();
}

