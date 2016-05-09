//============================================================================
// Name        : chatSvr.cpp
// Author      : slw
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;


#include <core.hpp>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef SRS_AUTO_GPERF_MP
    #include <gperftools/heap-profiler.h>
#endif
#ifdef SRS_AUTO_GPERF_CP
    #include <gperftools/profiler.h>
#endif

#include <kernel_error.hpp>
#include <app_server.hpp>
#include <app_config.hpp>
#include <app_log.hpp>

ILog* _log = new LogFast();
IThreadContext* _thContext = new ThreadContext();
// app module.
Config* _config = new Config();

CServer* _server = new CServer();

int run_master()
{
    int ret = ERROR_SUCCESS;

    if ((ret = _server->initialize_st()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _server->initialize_signal()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _server->acquire_pid_file()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _server->listen()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _server->register_signal()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _server->http_handle()) != ERROR_SUCCESS) {
        return ret;
    }

//    if ((ret = _server->ingest()) != ERROR_SUCCESS) {
//        return ret;
//    }

    if ((ret = _server->cycle()) != ERROR_SUCCESS) {
        return ret;
    }

    return 0;
}

int run()
{
    // if not deamon, directly run master.
    if (!_config->get_deamon()) {
        return run_master();
    }

    srs_trace("start deamon mode...");

    int pid = fork();

    if(pid < 0) {
        srs_error("create process error. ret=-1"); //ret=0
        return -1;
    }

    // grandpa
    if(pid > 0) {
        int status = 0;
        if(waitpid(pid, &status, 0) == -1) {
            srs_error("wait child process error! ret=-1"); //ret=0
        }
        srs_trace("grandpa process exit.");
        exit(0);
    }

    // father
    pid = fork();

    if(pid < 0) {
        srs_error("create process error. ret=0");
        return -1;
    }

    if(pid > 0) {
        srs_trace("father process exit. ret=0");
        exit(0);
    }

    // son
    srs_trace("son(deamon) process running.");

    return run_master();
}

/**
* main entrance.
*/
int main(int argc, char** argv)
{
    int ret = ERROR_SUCCESS;

//    // TODO: support both little and big endian.
//    srs_assert(srs_is_little_endian());

    // for gperf gmp or gcp,
    // should never enable it when not enabled for performance issue.
#ifdef SRS_AUTO_GPERF_MP
    HeapProfilerStart("gperf.srs.gmp");
#endif
#ifdef SRS_AUTO_GPERF_CP
    ProfilerStart("gperf.srs.gcp");
#endif

    // directly compile error when these two macro defines.
#if defined(SRS_AUTO_GPERF_MC) && defined(SRS_AUTO_GPERF_MP)
    #error ("option --with-gmc confict with --with-gmp, "
        "@see: http://google-perftools.googlecode.com/svn/trunk/doc/heap_checker.html\n"
        "Note that since the heap-checker uses the heap-profiling framework internally, "
        "it is not possible to run both the heap-checker and heap profiler at the same time");
#endif

    // never use gmp to check memory leak.
#ifdef SRS_AUTO_GPERF_MP
    #warning "gmp is not used for memory leak, please use gmc instead."
#endif

    // never use srs log(srs_trace, srs_error, etc) before config parse the option,
    // which will load the log config and apply it.
    if ((ret = _config->parse_options(argc, argv)) != ERROR_SUCCESS) {
        return ret;
    }

    // config parsed, initialize log.
    if ((ret = _log->initialize()) != ERROR_SUCCESS) {
        return ret;
    }

    // we check the config when the log initialized.
    if ((ret = _config->check_config()) != ERROR_SUCCESS) {
        return ret;
    }

//    srs_trace(RTMP_SIG_SRS_SERVER", stable is "RTMP_SIG_SRS_PRIMARY);
//    srs_trace("license: "RTMP_SIG_SRS_LICENSE", "RTMP_SIG_SRS_COPYRIGHT);
//    srs_trace("primary/master: "RTMP_SIG_SRS_PRIMARY);
//    srs_trace("authors: "RTMP_SIG_SRS_AUTHROS);
//    srs_trace("contributors: "SRS_AUTO_CONSTRIBUTORS);
//    srs_trace("uname: "SRS_AUTO_UNAME);
//    srs_trace("build: %s, %s", SRS_AUTO_BUILD_DATE, srs_is_little_endian()? "little-endian":"big-endian");
//    srs_trace("configure: "SRS_AUTO_USER_CONFIGURE);
//    srs_trace("features: "SRS_AUTO_CONFIGURE);
//#ifdef SRS_AUTO_ARM_UBUNTU12
//    srs_trace("arm tool chain: "SRS_AUTO_EMBEDED_TOOL_CHAIN);
//#endif
//    srs_trace("conf: %s, limit: %d", _config->config().c_str(), _config->get_max_connections());

    // features
//    check_macro_features();
//    show_macro_features();

    /**
    * we do nothing in the constructor of server,
    * and use initialize to create members, set hooks for instance the reload handler,
    * all initialize will done in this stage.
    */
    if ((ret = _server->initialize(NULL)) != ERROR_SUCCESS) {
        return ret;
    }

    return run();
}

