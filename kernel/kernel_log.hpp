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

#ifndef SRS_KERNEL_LOG_HPP
#define SRS_KERNEL_LOG_HPP

/*
#include <kernel_log.hpp>
*/

#include <core.hpp>

#include <stdio.h>

#include <errno.h>
#include <string.h>

#include <kernel_consts.hpp>

/**
* the log level, for example:
* if specified Debug level, all level messages will be logged.
* if specified Warn level, only Warn/Error/Fatal level messages will be logged.
*/
namespace _Log {
enum LogLevel {
    // only used for very verbose debug, generally, 
    // we compile without this level for high performance.
     Verbose = 0x01
    ,Info = 0x02
    ,Trace = 0x03
    ,Warn = 0x04
    ,Error = 0x05
    // specified the disabled level, no log, for utest.
    ,Disabled = 0x06
};
};

/**
* the log interface provides method to write log.
* but we provides some macro, which enable us to disable the log when compile.
* @see also SmtDebug/SmtTrace/SmtWarn/SmtError which is corresponding to Debug/Trace/Warn/Fatal.
*/ 
class ILog
{
public:
    ILog();
    virtual ~ILog();
public:
    /**
    * initialize log utilities.
    */
    virtual int initialize();
public:
    /**
    * log for verbose, very verbose information.
    */
    virtual void verbose(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for debug, detail information.
    */
    virtual void info(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for trace, important information.
    */
    virtual void trace(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for warn, warn is something should take attention, but not a error.
    */
    virtual void warn(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for error, something error occur, do something about the error, ie. close the connection,
    * but we will donot abort the program.
    */
    virtual void error(const char* tag, int context_id, const char* fmt, ...);
};

/**
 * the context id manager to identify context, for instance, the green-thread.
 * usage:
 *      _thContext->generate_id(); // when thread start.
 *      _thContext->get_id(); // get current generated id.
 *      int old_id = _thContext->set_id(1000); // set context id if need to merge thread context.
 */
// the context for multiple clients.
class IThreadContext
{
public:
	IThreadContext();
    virtual ~IThreadContext();
public:
    /**
     * generate the id for current context.
     */
    virtual int generate_id();
    /**
     * get the generated id of current context.
     */
    virtual int get_id();
    /**
     * set the id of current context.
     * @return the previous id value; 0 if no context.
     */
    virtual int set_id(int v);
};

// user must provides a log object
extern ILog* _log;

// user must implements the LogContext and define a global instance.
extern IThreadContext* _thContext;

// donot print method
#if 0
    #define srs_verbose(msg, ...) _log->verbose(NULL, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _log->info(NULL, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _log->trace(NULL, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _log->warn(NULL, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _log->error(NULL, _thContext->get_id(), msg, ##__VA_ARGS__)
// use __FUNCTION__ to print c method
#elif 1
    #define srs_verbose(msg, ...) _log->verbose(__FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _log->info(__FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _log->trace(__FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _log->warn(__FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _log->error(__FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
// use __PRETTY_FUNCTION__ to print c++ class:method
#elif 0
    #define srs_verbose(msg, ...) _log->verbose(__LINE__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _log->info(__LINE__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _log->trace(__LINE__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _log->warn(__LINE__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _log->error(__LINE__, _thContext->get_id(), msg, ##__VA_ARGS__)
// use __PRETTY_FUNCTION__ to print c++ class:method
#elif 0
    #define srs_verbose(msg, ...)  (void)0
    #define srs_info(msg, ...)     (void)0
    #define srs_trace(msg, ...)    (void)0
    #define srs_warn(msg, ...)     (void)0
    #define srs_error(msg, ...)    (void)0
#else
    #define srs_verbose(msg, ...) _log->verbose(__PRETTY_FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _log->info(__PRETTY_FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _log->trace(__PRETTY_FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _log->warn(__PRETTY_FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _log->error(__PRETTY_FUNCTION__, _thContext->get_id(), msg, ##__VA_ARGS__)
#endif

#if 0
// TODO: FIXME: add more verbose and info logs.
#ifndef SRS_AUTO_VERBOSE
    #undef srs_verbose
    #define srs_verbose(msg, ...) (void)0
#endif
#ifndef SRS_AUTO_INFO
    #undef srs_info
    #define srs_info(msg, ...) (void)0
#endif
#ifndef SRS_AUTO_TRACE
    #undef srs_trace
    #define srs_trace(msg, ...) (void)0
#endif
#endif


#endif

