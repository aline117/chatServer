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

#ifndef _APP_CONFIG_HPP
#define _APP_CONFIG_HPP

/*
#include <app_config.hpp>
*/
#include <core.hpp>

#include <vector>
#include <string>

#include <app_reload.hpp>

namespace __internal
{
    class ConfigBuffer;
}

/**
* the config directive.
* the config file is a group of directives,
* all directive has name, args and child-directives.
* for example, the following config text:
        vhost vhost.ossrs.net {
            enabled         on;
            ingest livestream {
                enabled      on;
                ffmpeg       /bin/ffmpeg;
            }
        }
* will be parsed to:
*       ConfDirective: name="vhost", arg0="vhost.ossrs.net", child-directives=[
*           ConfDirective: name="enabled", arg0="on", child-directives=[]
*           ConfDirective: name="ingest", arg0="livestream", child-directives=[
*               ConfDirective: name="enabled", arg0="on", child-directives=[]
*               ConfDirective: name="ffmpeg", arg0="/bin/ffmpeg", child-directives=[]
*           ]
*       ]
* @remark, allow empty directive, for example: "dir0 {}"
* @remark, don't allow empty name, for example: ";" or "{dir0 arg0;}
*/
class ConfDirective
{
public:
    /**
    * the line of config file in which the directive from
    */
    int conf_line;
    /**
    * the name of directive, for example, the following config text:
    *       enabled     on;
    * will be parsed to a directive, its name is "enalbed"
    */
    std::string name;
    /**
    * the args of directive, for example, the following config text:
    *       listen      1935 1936;
    * will be parsed to a directive, its args is ["1935", "1936"].
    */
    std::vector<std::string> args;
    /**
    * the child directives, for example, the following config text:
    *       vhost vhost.ossrs.net {
    *           enabled         on;
    *       }
    * will be parsed to a directive, its directives is a vector contains 
    * a directive, which is:
    *       name:"enalbed", args:["on"], directives:[]
    * 
    * @remark, the directives can contains directives.
    */
    std::vector<ConfDirective*> directives;
public:
    ConfDirective();
    virtual ~ConfDirective();
// args
public:
    /**
    * get the args0,1,2, if user want to get more args,
    * directly use the args.at(index).
    */
    virtual std::string arg0();
    virtual std::string arg1();
    virtual std::string arg2();
// directives
public:
    /**
    * get the directive by index.
    * @remark, assert the index<directives.size().
    */
    virtual ConfDirective* at(int index);
    /**
    * get the directive by name, return the first match.
    */
    virtual ConfDirective* get(std::string _name);
    /**
    * get the directive by name and its arg0, return the first match.
    */
    virtual ConfDirective* get(std::string _name, std::string _arg0);
// help utilities
public:
    /**
    * whether current directive is vhost.
    */
    virtual bool is_vhost();
    /**
    * whether current directive is stream_caster.
    */
    virtual bool is_stream_caster();
// parse utilities
public:
    /**
    * parse config directive from file buffer.
    */
    virtual int parse(__internal::ConfigBuffer* buffer);
// private parse.
private:
    /**
    * the directive parsing type.
    */
    enum SrsDirectiveType {
        /**
        * the root directives, parsing file.
        */
        parse_file, 
        /**
        * for each direcitve, parsing text block.
        */
        parse_block
    };
    /**
    * parse the conf from buffer. the work flow:
    * 1. read a token(directive args and a ret flag), 
    * 2. initialize the directive by args, args[0] is name, args[1-N] is args of directive,
    * 3. if ret flag indicates there are child-directives, read_conf(directive, block) recursively.
    */
    virtual int parse_conf(__internal::ConfigBuffer* buffer, SrsDirectiveType type);
    /**
    * read a token from buffer.
    * a token, is the directive args and a flag indicates whether has child-directives.
    * @param args, the output directive args, the first is the directive name, left is the args.
    * @param line_start, the actual start line of directive.
    * @return, an error code indicates error or has child-directives.
    */
    virtual int read_token(__internal::ConfigBuffer* buffer, std::vector<std::string>& args, int& line_start);
};

/**
* the config service provider.
* for the config supports reload, so never keep the reference cross st-thread,
* that is, never save the ConfDirective* get by any api of config,
* for it maybe free in the reload st-thread cycle.
* you can keep it before st-thread switch, or simply never keep it.
*/
class Config
{
// user command
private:
    /**
     * whether srs is run in dolphin mode.
     * @see https://github.com/ossrs/srs-dolphin
     */
    bool dolphin;
    std::string dolphin_rtmp_port;
    std::string dolphin_http_port;
    /**
    * whether show help and exit.
    */
    bool show_help;
    /**
    * whether test config file and exit.
    */
    bool test_conf;
    /**
    * whether show SRS version and exit.
    */
    bool show_version;
// global env variables.
private:
    /**
    * the user parameters, the argc and argv.
    * the argv is " ".join(argv), where argv is from main(argc, argv).
    */
    std::string _argv;
    /**
    * current working directory.
    */
    std::string _cwd;
// config section
private:
    /**
    * the last parsed config file.
    * if reload, reload the config file.
    */
    std::string config_file;
    /**
    * the directive root.
    */
    ConfDirective* root;
// reload section
private:
    /**
    * the reload subscribers, when reload, callback all handlers.
    */
    std::vector<IReloadHandler*> subscribes;
public:
    Config();
    virtual ~Config();
// dolphin
public:
    /**
     * whether srs is in dolphin mode.
     */
    virtual bool is_dolphin();
private:
    virtual void set_config_directive(ConfDirective* parent, std::string dir, std::string value);
// reload
public:
    /**
    * for reload handler to register itself,
    * when config service do the reload, callback the handler.
    */
    virtual void subscribe(IReloadHandler* handler);
    /**
    * for reload handler to unregister itself.
    */
    virtual void unsubscribe(IReloadHandler* handler);
    /**
    * reload the config file.
    * @remark, user can test the config before reload it.
    */
    virtual int reload();

protected:
    /**
    * reload from the config.
    * @remark, use protected for the utest to override with mock.
    */
    virtual int reload_conf(Config* conf);

// parse options and file
public:
    /**
    * parse the cli, the main(argc,argv) function.
    */
    virtual int parse_options(int argc, char** argv);
    /**
    * get the config file path.
    */
    virtual std::string config();
private:
    /**
    * parse each argv.
    */
    virtual int parse_argv(int& i, char** argv);
    /**
    * print help and exit.
    */
    virtual void print_help(char** argv);
public:
    /**
    * parse the config file, which is specified by cli.
    */
    virtual int parse_file(const char* filename);
    /**
    * check the parsed config.
    */
    virtual int check_config();
protected:
    /**
    * parse config from the buffer.
    * @param buffer, the config buffer, user must delete it.
    * @remark, use protected for the utest to override with mock.
    */
    virtual int parse_buffer(__internal::ConfigBuffer* buffer);
// global env
public:
    /**
    * get the current work directory.
    */
    virtual std::string         cwd();
    /**
    * get the cli, the main(argc,argv), program start command.
    */
    virtual std::string         argv();
// global section
public:
    /**
    * get the directive root, corresponding to the config file.
    * the root directive, no name and args, contains directives.
    * all directive parsed can retrieve from root.
    */
    virtual ConfDirective*   get_root();
    /**
    * get the deamon config.
    * if true, SRS will run in deamon mode, fork and fork to reap the 
    * grand-child process to init process.
    */
    virtual bool                get_deamon();
    /**
    * get the max connections limit of system.
    * if exceed the max connection, SRS will disconnect the connection.
    * @remark, linux will limit the connections of each process, 
    *       for example, when you need SRS to service 10000+ connections,
    *       user must use "ulimit -HSn 10000" and config the max connections
    *       of SRS.
    */
    virtual int                 get_max_connections();
    /**
    * get the listen port of SRS.
    * user can specifies multiple listen ports,
    * each args of directive is a listen port.
    */
    virtual std::vector<std::string>        get_listens();
    /**
    * get the pid file path.
    * the pid file is used to save the pid of SRS,
    * use file lock to prevent multiple SRS starting.
    * @remark, if user need to run multiple SRS instance,
    *       for example, to start multiple SRS for multiple CPUs,
    *       user can use different pid file for each process.
    */
    virtual std::string         get_pid_file();
    /**
    * get pithy print pulse ms,
    * for example, all rtmp connections only print one message
    * every this interval in ms.
    */
    virtual int                 get_pithy_print_ms();
    /**
     * whether use utc-time to format the time.
     */
    virtual bool                get_utc_time();

// log section
public:
    /**
    * whether log to file.
    */
    virtual bool                get_log_tank_file();
    /**
    * get the log level.
    */
    virtual std::string         get_log_level();
    /**
    * get the log file path.
    */
    virtual std::string         get_log_file();

// http heartbeart section
private:
    /**
    * get the heartbeat directive.
    */
    virtual ConfDirective*   get_heartbeart();
public:
    /**
    * whether heartbeat enabled.
    */
    virtual bool                get_heartbeat_enabled();
    /**
    * get the heartbeat interval, in ms.
    */
    virtual int64_t             get_heartbeat_interval();
    /**
    * get the heartbeat report url.
    */
    virtual std::string         get_heartbeat_url();
    /**
    * get the device id of heartbeat, to report to server.
    */
    virtual std::string         get_heartbeat_device_id();
    /**
    * whether report with summaries of http api: /api/v1/summaries.
    */
    virtual bool                get_heartbeat_summaries();
// stats section
private:
    /**
    * get the stats directive.
    */
    virtual ConfDirective*   get_stats();
public:
    /**
    * get the network device index, used to retrieve the ip of device,
    * for heartbeat to report to server, or to get the local ip.
    * for example, 0 means the eth0 maybe.
    */
    virtual int                 get_stats_network();
    /**
    * get the disk stat device name list.
    * the device name configed in args of directive.
    * @return the disk device name to stat. NULL if not configed.
    */
    virtual ConfDirective*   get_stats_disk_device();
};

namespace __internal
{
    /**
    * the buffer of config content.
    */
    class ConfigBuffer
    {
    protected:
        // last available position.
        char* last;
        // end of buffer.
        char* end;
        // start of buffer.
        char* start;
    public:
        // current consumed position.
        char* pos;
        // current parsed line.
        int line;
    public:
        ConfigBuffer();
        virtual ~ConfigBuffer();
    public:
        /**
        * fullfill the buffer with content of file specified by filename.
        */
        virtual int fullfill(const char* filename);
        /**
        * whether buffer is empty.
        */
        virtual bool empty();
    };
};

/**
* deep compare directive.
*/
extern bool srs_directive_equals(ConfDirective* a, ConfDirective* b);
//
///**
// * helper utilities, used for compare the consts values.
// */
//extern bool srs_config_hls_is_on_error_ignore(std::string strategy);
//extern bool srs_config_hls_is_on_error_continue(std::string strategy);
//extern bool srs_config_ingest_is_file(std::string type);
//extern bool srs_config_ingest_is_stream(std::string type);
//extern bool srs_config_dvr_is_plan_segment(std::string plan);
//extern bool srs_config_dvr_is_plan_session(std::string plan);
//extern bool srs_config_dvr_is_plan_append(std::string plan);
//extern bool srs_stream_caster_is_udp(std::string caster);
//extern bool srs_stream_caster_is_rtsp(std::string caster);
//extern bool srs_stream_caster_is_flv(std::string caster);

// global config
extern Config* _config;

#endif

