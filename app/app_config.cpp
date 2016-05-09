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

#include <app_config.hpp>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// file operations.
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <algorithm>

using namespace std;

#include <kernel_error.hpp>
#include <kernel_log.hpp>
#include <core_autofree.hpp>
//#include <srs_app_source.hpp>
#include <kernel_file.hpp>
#include <app_utility.hpp>
//#include <srs_core_performance.hpp>

using namespace __internal;

// when user config an invalid value, macros to perfer true or false.
#define SRS_CONF_PERFER_FALSE(conf_arg) conf_arg == "on"
#define SRS_CONF_PERFER_TRUE(conf_arg) conf_arg != "off"

///////////////////////////////////////////////////////////
// default consts values
///////////////////////////////////////////////////////////
#define SRS_CONF_DEFAULT_PID_FILE "./im.pid"
#define SRS_CONF_DEFAULT_LOG_FILE "./im.log"
#define SRS_CONF_DEFAULT_LOG_LEVEL "trace"
#define SRS_CONF_DEFAULT_LOG_TANK_CONSOLE "console"
#define SRS_CONF_DEFAULT_COFNIG_FILE "conf/im.conf"
#define SRS_CONF_DEFAULT_UTC_TIME false

#define SRS_CONF_DEFAULT_MAX_CONNECTIONS 1000
#define SRS_CONF_DEFAULT_HLS_PATH "./objs/nginx/html"
#define SRS_CONF_DEFAULT_HLS_M3U8_FILE "[app]/[stream].m3u8"
#define SRS_CONF_DEFAULT_HLS_TS_FILE "[app]/[stream]-[seq].ts"
#define SRS_CONF_DEFAULT_HLS_TS_FLOOR false
#define SRS_CONF_DEFAULT_HLS_FRAGMENT 10
#define SRS_CONF_DEFAULT_HLS_TD_RATIO 1.5
#define SRS_CONF_DEFAULT_HLS_AOF_RATIO 2.0
#define SRS_CONF_DEFAULT_HLS_WINDOW 60
#define SRS_CONF_DEFAULT_HLS_ON_ERROR_IGNORE "continue"
#define SRS_CONF_DEFAULT_HLS_ON_ERROR_DISCONNECT "disconnect"
#define SRS_CONF_DEFAULT_HLS_ON_ERROR_CONTINUE "continue"
#define SRS_CONF_DEFAULT_HLS_ON_ERROR SRS_CONF_DEFAULT_HLS_ON_ERROR_IGNORE
#define SRS_CONF_DEFAULT_HLS_STORAGE "disk"
#define SRS_CONF_DEFAULT_HLS_MOUNT "[vhost]/[app]/[stream].m3u8"
#define SRS_CONF_DEFAULT_HLS_ACODEC "aac"
#define SRS_CONF_DEFAULT_HLS_VCODEC "h264"
#define SRS_CONF_DEFAULT_HLS_CLEANUP true
#define SRS_CONF_DEFAULT_HLS_WAIT_KEYFRAME true
#define SRS_CONF_DEFAULT_HLS_NB_NOTIFY 64
#define SRS_CONF_DEFAULT_DVR_PATH "./objs/nginx/html/[app]/[stream].[timestamp].flv"
#define SRS_CONF_DEFAULT_DVR_PLAN_SESSION "session"
#define SRS_CONF_DEFAULT_DVR_PLAN_SEGMENT "segment"
#define SRS_CONF_DEFAULT_DVR_PLAN_APPEND "append"
#define SRS_CONF_DEFAULT_DVR_PLAN SRS_CONF_DEFAULT_DVR_PLAN_SESSION
#define SRS_CONF_DEFAULT_DVR_DURATION 30
#define SRS_CONF_DEFAULT_TIME_JITTER "full"
#define SRS_CONF_DEFAULT_ATC_AUTO true
#define SRS_CONF_DEFAULT_MIX_CORRECT false
// in seconds, the paused queue length.
#define SRS_CONF_DEFAULT_PAUSED_LENGTH 10
// the interval in seconds for bandwidth check
#define SRS_CONF_DEFAULT_BANDWIDTH_INTERVAL 30
// the interval in seconds for bandwidth check
#define SRS_CONF_DEFAULT_BANDWIDTH_LIMIT_KBPS 1000

#define SRS_CONF_DEFAULT_HTTP_MOUNT "[vhost]/"
#define SRS_CONF_DEFAULT_HTTP_REMUX_MOUNT "[vhost]/[app]/[stream].flv"
#define SRS_CONF_DEFAULT_HTTP_DIR SRS_CONF_DEFAULT_HLS_PATH
#define SRS_CONF_DEFAULT_HTTP_AUDIO_FAST_CACHE 0

#define SRS_CONF_DEFAULT_HTTP_STREAM_PORT "8080"
#define SRS_CONF_DEFAULT_HTTP_API_PORT "1985"
#define SRS_CONF_DEFAULT_HTTP_API_CROSSDOMAIN true

#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_ENABLED false
#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_INTERVAL 9.9
#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_URL "http://"SRS_CONSTS_LOCALHOST":8085/api/v1/servers"
#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_SUMMARIES false

#define SRS_CONF_DEFAULT_SECURITY_ENABLED false

#define SRS_CONF_DEFAULT_STREAM_CASTER_ENABLED false
#define SRS_CONF_DEFAULT_STREAM_CASTER_MPEGTS_OVER_UDP "mpegts_over_udp"
#define SRS_CONF_DEFAULT_STREAM_CASTER_RTSP "rtsp"
#define SRS_CONF_DEFAULT_STREAM_CASTER_FLV "flv"

#define SRS_CONF_DEFAULT_STATS_NETWORK_DEVICE_INDEX 0

#define SRS_CONF_DEFAULT_PITHY_PRINT_MS 10000

#define SRS_CONF_DEFAULT_INGEST_TYPE_FILE "file"
#define SRS_CONF_DEFAULT_INGEST_TYPE_STREAM "stream"

#define SRS_CONF_DEFAULT_TRANSCODE_IFORMAT "flv"
#define SRS_CONF_DEFAULT_TRANSCODE_OFORMAT "flv"

#define SRS_CONF_DEFAULT_EDGE_TOKEN_TRAVERSE false
#define SRS_CONF_DEFAULT_EDGE_TRANSFORM_VHOST "[vhost]"

// hds default value
#define SRS_CONF_DEFAULT_HDS_PATH       "./objs/nginx/html"
#define SRS_CONF_DEFAULT_HDS_WINDOW     (60)
#define SRS_CONF_DEFAULT_HDS_FRAGMENT   (10)

// '\n'
#define SRS_LF (char)SRS_CONSTS_LF

// '\r'
#define SRS_CR (char)SRS_CONSTS_CR

bool is_common_space(char ch)
{
    return (ch == ' ' || ch == '\t' || ch == SRS_CR || ch == SRS_LF);
}

ConfDirective::ConfDirective()
{
}

ConfDirective::~ConfDirective()
{
    std::vector<ConfDirective*>::iterator it;
    for (it = directives.begin(); it != directives.end(); ++it) {
        ConfDirective* directive = *it;
        srs_freep(directive);
    }
    directives.clear();
}

string ConfDirective::arg0()
{
    if (args.size() > 0) {
        return args.at(0);
    }
    
    return "";
}

string ConfDirective::arg1()
{
    if (args.size() > 1) {
        return args.at(1);
    }
    
    return "";
}

string ConfDirective::arg2()
{
    if (args.size() > 2) {
        return args.at(2);
    }
    
    return "";
}

ConfDirective* ConfDirective::at(int index)
{
    srs_assert(index < (int)directives.size());
    return directives.at(index);
}

ConfDirective* ConfDirective::get(string _name)
{
    std::vector<ConfDirective*>::iterator it;
    for (it = directives.begin(); it != directives.end(); ++it) {
        ConfDirective* directive = *it;
        if (directive->name == _name) {
            return directive;
        }
    }
    
    return NULL;
}

ConfDirective* ConfDirective::get(string _name, string _arg0)
{
    std::vector<ConfDirective*>::iterator it;
    for (it = directives.begin(); it != directives.end(); ++it) {
        ConfDirective* directive = *it;
        if (directive->name == _name && directive->arg0() == _arg0) {
            return directive;
        }
    }
    
    return NULL;
}

bool ConfDirective::is_vhost()
{
    return name == "vhost";
}

bool ConfDirective::is_stream_caster()
{
    return name == "stream_caster";
}

int ConfDirective::parse(ConfigBuffer* buffer)
{
    return parse_conf(buffer, parse_file);
}

// see: ngx_conf_parse
int ConfDirective::parse_conf(ConfigBuffer* buffer, SrsDirectiveType type)
{
    int ret = ERROR_SUCCESS;
    
    while (true) {
        std::vector<string> args;
        int line_start = 0;
        ret = read_token(buffer, args, line_start);
        
        /**
        * ret maybe:
        * ERROR_SYSTEM_CONFIG_INVALID           error.
        * ERROR_SYSTEM_CONFIG_DIRECTIVE         directive terminated by ';' found
        * ERROR_SYSTEM_CONFIG_BLOCK_START       token terminated by '{' found
        * ERROR_SYSTEM_CONFIG_BLOCK_END         the '}' found
        * ERROR_SYSTEM_CONFIG_EOF               the config file is done
        */
        if (ret == ERROR_SYSTEM_CONFIG_INVALID) {
            return ret;
        }
        if (ret == ERROR_SYSTEM_CONFIG_BLOCK_END) {
            if (type != parse_block) {
                srs_error("line %d: unexpected \"}\", ret=%d", buffer->line, ret);
                return ret;
            }
            return ERROR_SUCCESS;
        }
        if (ret == ERROR_SYSTEM_CONFIG_EOF) {
            if (type == parse_block) {
                srs_error("line %d: unexpected end of file, expecting \"}\", ret=%d", conf_line, ret);
                return ret;
            }
            return ERROR_SUCCESS;
        }
        
        if (args.empty()) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("line %d: empty directive. ret=%d", conf_line, ret);
            return ret;
        }
        
        // build directive tree.
        ConfDirective* directive = new ConfDirective();

        directive->conf_line = line_start;
        directive->name = args[0];
        args.erase(args.begin());
        directive->args.swap(args);
        
        directives.push_back(directive);
        
        if (ret == ERROR_SYSTEM_CONFIG_BLOCK_START) {
            if ((ret = directive->parse_conf(buffer, parse_block)) != ERROR_SUCCESS) {
                return ret;
            }
        }
    }
    
    return ret;
}

// see: ngx_conf_read_token
int ConfDirective::read_token(ConfigBuffer* buffer, vector<string>& args, int& line_start)
{
    int ret = ERROR_SUCCESS;

    char* pstart = buffer->pos;

    bool sharp_comment = false;
    
    bool d_quoted = false;
    bool s_quoted = false;
    
    bool need_space = false;
    bool last_space = true;
    
    while (true) {
        if (buffer->empty()) {
            ret = ERROR_SYSTEM_CONFIG_EOF;
            
            if (!args.empty() || !last_space) {
                srs_error("line %d: unexpected end of file, expecting ; or \"}\"", buffer->line);
                return ERROR_SYSTEM_CONFIG_INVALID;
            }
            srs_trace("config parse complete");
            
            return ret;
        }
        
        char ch = *buffer->pos++;
        
        if (ch == SRS_LF) {
            buffer->line++;
            sharp_comment = false;
        }
        
        if (sharp_comment) {
            continue;
        }
        
        if (need_space) {
            if (is_common_space(ch)) {
                last_space = true;
                need_space = false;
                continue;
            }
            if (ch == ';') {
                return ERROR_SYSTEM_CONFIG_DIRECTIVE;
            }
            if (ch == '{') {
                return ERROR_SYSTEM_CONFIG_BLOCK_START;
            }
            srs_error("line %d: unexpected '%c'", buffer->line, ch);
            return ERROR_SYSTEM_CONFIG_INVALID; 
        }
        
        // last charecter is space.
        if (last_space) {
            if (is_common_space(ch)) {
                continue;
            }
            pstart = buffer->pos - 1;
            switch (ch) {
                case ';':
                    if (args.size() == 0) {
                        srs_error("line %d: unexpected ';'", buffer->line);
                        return ERROR_SYSTEM_CONFIG_INVALID;
                    }
                    return ERROR_SYSTEM_CONFIG_DIRECTIVE;
                case '{':
                    if (args.size() == 0) {
                        srs_error("line %d: unexpected '{'", buffer->line);
                        return ERROR_SYSTEM_CONFIG_INVALID;
                    }
                    return ERROR_SYSTEM_CONFIG_BLOCK_START;
                case '}':
                    if (args.size() != 0) {
                        srs_error("line %d: unexpected '}'", buffer->line);
                        return ERROR_SYSTEM_CONFIG_INVALID;
                    }
                    return ERROR_SYSTEM_CONFIG_BLOCK_END;
                case '#':
                    sharp_comment = 1;
                    continue;
                case '"':
                    pstart++;
                    d_quoted = true;
                    last_space = 0;
                    continue;
                case '\'':
                    pstart++;
                    s_quoted = true;
                    last_space = 0;
                    continue;
                default:
                    last_space = 0;
                    continue;
            }
        } else {
        // last charecter is not space
            if (line_start == 0) {
                line_start = buffer->line;
            }
            
            bool found = false;
            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = false;
                    need_space = true;
                    found = true;
                }
            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = false;
                    need_space = true;
                    found = true;
                }
            } else if (is_common_space(ch) || ch == ';' || ch == '{') {
                last_space = true;
                found = 1;
            }
            
            if (found) {
                int len = (int)(buffer->pos - pstart);
                char* aword = new char[len];
                memcpy(aword, pstart, len);
                aword[len - 1] = 0;
                
                string word_str = aword;
                if (!word_str.empty()) {
                    args.push_back(word_str);
                }
                srs_freepa(aword);
                
                if (ch == ';') {
                    return ERROR_SYSTEM_CONFIG_DIRECTIVE;
                }
                if (ch == '{') {
                    return ERROR_SYSTEM_CONFIG_BLOCK_START;
                }
            }
        }
    }
    
    return ret;
}

Config::Config()
{
    dolphin = false;
    
    show_help = false;
    show_version = false;
    test_conf = false;
    
    root = new ConfDirective();
    root->conf_line = 0;
    root->name = "root";
}

Config::~Config()
{
    srs_freep(root);
}

bool Config::is_dolphin()
{
    return dolphin;
}

void Config::set_config_directive(ConfDirective* parent, string dir, string value)
{
    ConfDirective* d = parent->get(dir);
    
    if (!d) {
        d = new ConfDirective();
        if (!dir.empty()) {
            d->name = dir;
        }
        parent->directives.push_back(d);
    }
    
    d->args.clear();
    if (!value.empty()) {
        d->args.push_back(value);
    }
}

void Config::subscribe(IReloadHandler* handler)
{
    std::vector<IReloadHandler*>::iterator it;
    
    it = std::find(subscribes.begin(), subscribes.end(), handler);
    if (it != subscribes.end()) {
        return;
    }
    
    subscribes.push_back(handler);
}

void Config::unsubscribe(IReloadHandler* handler)
{
    std::vector<IReloadHandler*>::iterator it;
    
    it = std::find(subscribes.begin(), subscribes.end(), handler);
    if (it == subscribes.end()) {
        return;
    }
    
    subscribes.erase(it);
}

int Config::reload()
{
    int ret = ERROR_SUCCESS;

    Config conf;

    if ((ret = conf.parse_file(config_file.c_str())) != ERROR_SUCCESS) {
        srs_error("ignore config reloader parse file failed. ret=%d", ret);
        ret = ERROR_SUCCESS;
        return ret;
    }
    srs_info("config reloader parse file success.");

    if ((ret = conf.check_config()) != ERROR_SUCCESS) {
        srs_error("ignore config reloader check config failed. ret=%d", ret);
        ret = ERROR_SUCCESS;
        return ret;
    }
    
    return reload_conf(&conf);
}

int Config::reload_conf(Config* conf)
{
    int ret = ERROR_SUCCESS;
    
    ConfDirective* old_root = root;
    SrsAutoFree(ConfDirective, old_root);
    
    root = conf->root;
    conf->root = NULL;
    
    // merge config.
    std::vector<IReloadHandler*>::iterator it;

    // never support reload:
    //      daemon
    //
    // always support reload without additional code:
    //      chunk_size, ff_log_dir,
    //      bandcheck, http_hooks, heartbeat, 
    //      token_traverse, debug_srs_upnode,
    //      security
    
    // merge config: max_connections
    if (!srs_directive_equals(root->get("max_connections"), old_root->get("max_connections"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_max_conns()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload max_connections failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload max_connections success.");
    }

    // merge config: listen
    if (!srs_directive_equals(root->get("listen"), old_root->get("listen"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_listen()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload listen failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload listen success.");
    }
    
    // merge config: pid
    if (!srs_directive_equals(root->get("pid"), old_root->get("pid"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_pid()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload pid failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload pid success.");
    }
    
    // merge config: srs_log_tank
    if (!srs_directive_equals(root->get("srs_log_tank"), old_root->get("srs_log_tank"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_log_tank()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload srs_log_tank failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload srs_log_tank success.");
    }
    
    // merge config: srs_log_level
    if (!srs_directive_equals(root->get("srs_log_level"), old_root->get("srs_log_level"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_log_level()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload srs_log_level failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload srs_log_level success.");
    }
    
    // merge config: srs_log_file
    if (!srs_directive_equals(root->get("srs_log_file"), old_root->get("srs_log_file"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_log_file()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload srs_log_file failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload srs_log_file success.");
    }

#if 0
    // merge config: utc_time
    if (!srs_directive_equals(root->get("utc_time"), old_root->get("utc_time"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_utc_time()) != ERROR_SUCCESS) {
                srs_error("notify subscribes reload utc_time failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload utc_time success.");
    }
#endif
    
    // merge config: pithy_print_ms
    if (!srs_directive_equals(root->get("pithy_print_ms"), old_root->get("pithy_print_ms"))) {
        for (it = subscribes.begin(); it != subscribes.end(); ++it) {
            IReloadHandler* subscribe = *it;
            if ((ret = subscribe->on_reload_pithy_print()) != ERROR_SUCCESS) {
                srs_error("notify subscribes pithy_print_ms failed. ret=%d", ret);
                return ret;
            }
        }
        srs_trace("reload pithy_print_ms success.");
    }
    
#if 0
    // merge config: http_api
    if ((ret = reload_http_api(old_root)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // merge config: http_stream
    if ((ret = reload_http_stream(old_root)) != ERROR_SUCCESS) {
        return ret;
    }

    // TODO: FIXME: support reload stream_caster.

    // merge config: vhost
    if ((ret = reload_vhost(old_root)) != ERROR_SUCCESS) {
        return ret;
    }
#endif

    return ret;
}

// see: ngx_get_options
int Config::parse_options(int argc, char** argv)
{
    int ret = ERROR_SUCCESS;
    
    // argv
    for (int i = 0; i < argc; i++) {
        _argv.append(argv[i]);
        
        if (i < argc - 1) {
            _argv.append(" ");
        }
    }
    
    // cwd
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    _cwd = cwd;
    
    // config
    show_help = true;
    for (int i = 1; i < argc; i++) {
        if ((ret = parse_argv(i, argv)) != ERROR_SUCCESS) {
            return ret;
        }
    }
    
    if (show_help) {
        print_help(argv);
        exit(0);
    }
    
    if (show_version) {
        fprintf(stderr, "%s\n", SIG_VERSION);
        exit(0);
    }
    
    // first hello message.
    srs_trace(SIG_VERSION);
    
    if (config_file.empty()) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("config file not specified, see help: %s -h, ret=%d", argv[0], ret);
        return ret;
    }

    ret = parse_file(config_file.c_str());
    
    if (test_conf) {
        // the parse_file never check the config,
        // we check it when user requires check config file.
        if (ret == ERROR_SUCCESS) {
            ret = check_config();
        }

        if (ret == ERROR_SUCCESS) {
            srs_trace("config file is ok");
            exit(0);
        } else {
            srs_error("config file is invalid");
            exit(ret);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // check log name and level
    ////////////////////////////////////////////////////////////////////////
    if (true) {
        std::string log_filename = this->get_log_file();
        if (get_log_tank_file() && log_filename.empty()) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("must specifies the file to write log to. ret=%d", ret);
            return ret;
        }
        if (get_log_tank_file()) {
            srs_trace("write log to file %s", log_filename.c_str());
            srs_trace("you can: tailf %s", log_filename.c_str());
        } else {
            srs_trace("write log to console");
        }
    }
    
    return ret;
}

string Config::config()
{
    return config_file;
}

int Config::parse_argv(int& i, char** argv)
{
    int ret = ERROR_SUCCESS;
    
    char* p = argv[i];
        
    if (*p++ != '-') {
        show_help = true;
        return ret;
    }
    
    while (*p) {
        switch (*p++) {
            case '?':
            case 'h':
                show_help = true;
                break;
            case 't':
                show_help = false;
                test_conf = true;
                break;
            case 'p':
                dolphin = true;
                if (*p) {
                    dolphin_rtmp_port = p;
                    continue;
                }
                if (argv[++i]) {
                    dolphin_rtmp_port = argv[i];
                    continue;
                }
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("option \"-p\" requires params, ret=%d", ret);
                return ret;
            case 'x':
                dolphin = true;
                if (*p) {
                    dolphin_http_port = p;
                    continue;
                }
                if (argv[++i]) {
                    dolphin_http_port = argv[i];
                    continue;
                }
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("option \"-x\" requires params, ret=%d", ret);
                return ret;
            case 'v':
            case 'V':
                show_help = false;
                show_version = true;
                break;
            case 'c':
                show_help = false;
                if (*p) {
                    config_file = p;
                    continue;
                }
                if (argv[++i]) {
                    config_file = argv[i];
                    continue;
                }
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("option \"-c\" requires parameter, ret=%d", ret);
                return ret;
            default:
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("invalid option: \"%c\", see help: %s -h, ret=%d", *(p - 1), argv[0], ret);
                return ret;
        }
    }
    
    return ret;
}

void Config::print_help(char** argv)
{
    printf(
#if 0
    	RTMP_SIG_SRS_SERVER" "RTMP_SIG_SRS_COPYRIGHT"\n"
        "License: "RTMP_SIG_SRS_LICENSE"\n"
        "Primary: "RTMP_SIG_SRS_PRIMARY"\n"
        "Authors: "RTMP_SIG_SRS_AUTHROS"\n"
        "Build: "SRS_AUTO_BUILD_DATE" Configuration:"SRS_AUTO_USER_CONFIGURE"\n"
        "Features:"SRS_AUTO_CONFIGURE"\n""\n"
#endif
        "Usage: %s [-h?vV] [[-t] -c <filename>]\n" 
        "\n"
        "Options:\n"
        "   -?, -h              : show this help and exit(0)\n"
        "   -v, -V              : show version and exit(0)\n"
        "   -t                  : test configuration file, exit(error_code).\n"
        "   -c filename         : use configuration file for SRS\n"
        "For srs-dolphin:\n"
        "   -p  IM-port         : the IM port to listen.\n"
        "   -x  http-port       : the http port to listen.\n"
        "\n"
        "For example:\n"
        "   %s -v\n"
        "   %s -t -c "SRS_CONF_DEFAULT_COFNIG_FILE"\n"
        "   %s -c "SRS_CONF_DEFAULT_COFNIG_FILE"\n",
        argv[0], argv[0], argv[0], argv[0]);
}

int Config::parse_file(const char* filename)
{
    int ret = ERROR_SUCCESS;
    
    config_file = filename;
    
    if (config_file.empty()) {
        return ERROR_SYSTEM_CONFIG_INVALID;
    }
    
    ConfigBuffer buffer;
    
    if ((ret = buffer.fullfill(config_file.c_str())) != ERROR_SUCCESS) {
        return ret;
    }
    
    return parse_buffer(&buffer);
}

int Config::check_config()
{
    int ret = ERROR_SUCCESS;

    srs_trace("srs checking config...");

    ////////////////////////////////////////////////////////////////////////
    // check empty
    ////////////////////////////////////////////////////////////////////////
    if (root->directives.size() == 0) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("conf is empty, ret=%d", ret);
        return ret;
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check root directives.
    ////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < (int)root->directives.size(); i++) {
        ConfDirective* conf = root->at(i);
        std::string n = conf->name;
        if (n != "listen" && n != "pid" && n != "chunk_size" && n != "ff_log_dir" 
            && n != "srs_log_tank" && n != "srs_log_level" && n != "srs_log_file"
            && n != "max_connections" && n != "daemon" && n != "heartbeat"
            && n != "http_api" && n != "stats" && n != "vhost" && n != "pithy_print_ms"
            && n != "http_stream" && n != "http_server" && n != "stream_caster"
            && n != "utc_time"
        ) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("unsupported directive %s, ret=%d", n.c_str(), ret);
            return ret;
        }
    }

#if 0
    if (true) {
        ConfDirective* conf = get_http_api();
        for (int i = 0; conf && i < (int)conf->directives.size(); i++) {
            string n = conf->at(i)->name;
            if (n != "enabled" && n != "listen" && n != "crossdomain") {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("unsupported http_api directive %s, ret=%d", n.c_str(), ret);
                return ret;
            }
        }
    }
    if (true) {
        ConfDirective* conf = get_http_stream();
        for (int i = 0; conf && i < (int)conf->directives.size(); i++) {
            string n = conf->at(i)->name;
            if (n != "enabled" && n != "listen" && n != "dir") {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("unsupported http_stream directive %s, ret=%d", n.c_str(), ret);
                return ret;
            }
        }
    }
#endif

    if (true) {
        ConfDirective* conf = get_heartbeart();
        for (int i = 0; conf && i < (int)conf->directives.size(); i++) {
            string n = conf->at(i)->name;
            if (n != "enabled" && n != "interval" && n != "url"
                && n != "device_id" && n != "summaries"
                ) {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("unsupported heartbeat directive %s, ret=%d", n.c_str(), ret);
                return ret;
            }
        }
    }
    if (true) {
        ConfDirective* conf = get_stats();
        for (int i = 0; conf && i < (int)conf->directives.size(); i++) {
            string n = conf->at(i)->name;
            if (n != "network" && n != "disk") {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("unsupported stats directive %s, ret=%d", n.c_str(), ret);
                return ret;
            }
        }
    }
    
    
    ////////////////////////////////////////////////////////////////////////
    // check listen for rtmp.
    ////////////////////////////////////////////////////////////////////////
    if (true) {
        vector<string> listens = get_listens();
        if (listens.size() <= 0) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("directive \"listen\" is empty, ret=%d", ret);
            return ret;
        }
        for (int i = 0; i < (int)listens.size(); i++) {
            string port = listens[i];
            if (port.empty() || ::atoi(port.c_str()) <= 0) {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("directive listen invalid, port=%s, ret=%d", port.c_str(), ret);
                return ret;
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check max connections
    ////////////////////////////////////////////////////////////////////////
    if (get_max_connections() <= 0) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("directive max_connections invalid, max_connections=%d, ret=%d", get_max_connections(), ret);
        return ret;
    }

#if 0
    // check max connections of system limits
    if (true) {
        int nb_consumed_fds = (int)get_listens().size();
        if (!get_http_api_listen().empty()) {
            nb_consumed_fds++;
        }
        if (!get_http_stream_listen().empty()) {
            nb_consumed_fds++;
        }
        if (get_log_tank_file()) {
            nb_consumed_fds++;
        }
        // 0, 1, 2 for stdin, stdout and stderr.
        nb_consumed_fds += 3;
        
        int nb_connections = get_max_connections();
        int nb_total = nb_connections + nb_consumed_fds;
        
        int max_open_files = (int)sysconf(_SC_OPEN_MAX);
        int nb_canbe = max_open_files - nb_consumed_fds - 1;

        // for each play connections, we open a pipe(2fds) to convert SrsConsumver to io,
        // refine performance, @see: https://github.com/ossrs/srs/issues/194
        if (nb_total >= max_open_files) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("invalid max_connections=%d, required=%d, system limit to %d, "
                "total=%d(max_connections=%d, nb_consumed_fds=%d), ret=%d. "
                "you can change max_connections from %d to %d, or "
                "you can login as root and set the limit: ulimit -HSn %d", 
                nb_connections, nb_total + 1, max_open_files, 
                nb_total, nb_connections, nb_consumed_fds,
                ret, nb_connections, nb_canbe, nb_total + 1);
            return ret;
        }
    }
#endif
    
    ////////////////////////////////////////////////////////////////////////
    // check heartbeat
    ////////////////////////////////////////////////////////////////////////
    if (get_heartbeat_interval() <= 0) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("directive heartbeat interval invalid, interval=%"PRId64", ret=%d", 
            get_heartbeat_interval(), ret);
        return ret;
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check stats
    ////////////////////////////////////////////////////////////////////////
    if (get_stats_network() < 0) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("directive stats network invalid, network=%d, ret=%d", 
            get_stats_network(), ret);
        return ret;
    }
#if 0
    if (true) {
        vector<std::string> ips = srs_get_local_ipv4_ips();
        int index = get_stats_network();
        if (index >= (int)ips.size()) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("stats network invalid, total local ip count=%d, index=%d, ret=%d",
                (int)ips.size(), index, ret);
            return ret;
        }
        srs_warn("stats network use index=%d, ip=%s", index, ips.at(index).c_str());
    }
    if (true) {
        ConfDirective* conf = get_stats_disk_device();
        if (conf == NULL || (int)conf->args.size() <= 0) {
            srs_warn("stats disk not configed, disk iops disabled.");
        } else {
            string disks;
            for (int i = 0; i < (int)conf->args.size(); i++) {
                disks += conf->args.at(i);
                disks += " ";
            }
            srs_warn("stats disk list: %s", disks.c_str());
        }
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check http api
    ////////////////////////////////////////////////////////////////////////
    if (get_http_api_listen().empty()) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("directive http_api listen invalid, listen=%s, ret=%d",
            get_http_api_listen().c_str(), ret);
        return ret;
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check http stream
    ////////////////////////////////////////////////////////////////////////
    if (get_http_stream_listen().empty()) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("directive http_stream listen invalid, listen=%s, ret=%d",
            get_http_stream_listen().c_str(), ret);
        return ret;
    }
#endif
    ////////////////////////////////////////////////////////////////////////
    // check log name and level
    ////////////////////////////////////////////////////////////////////////
    if (true) {
        std::string log_filename = this->get_log_file();
        if (get_log_tank_file() && log_filename.empty()) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("must specifies the file to write log to. ret=%d", ret);
            return ret;
        }
        if (get_log_tank_file()) {
            srs_trace("write log to file %s", log_filename.c_str());
            srs_trace("you can: tailf %s", log_filename.c_str());
        } else {
            srs_trace("write log to console");
        }
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check features
    ////////////////////////////////////////////////////////////////////////
#if 0
#ifndef SRS_AUTO_HTTP_SERVER
    if (get_http_stream_enabled()) {
        srs_warn("http_stream is disabled by configure");
    }
#endif
#ifndef SRS_AUTO_HTTP_API
    if (get_http_api_enabled()) {
        srs_warn("http_api is disabled by configure");
    }
#endif

    vector<ConfDirective*> stream_casters = get_stream_casters();
    for (int n = 0; n < (int)stream_casters.size(); n++) {
        ConfDirective* stream_caster = stream_casters[n];
        for (int i = 0; stream_caster && i < (int)stream_caster->directives.size(); i++) {
            ConfDirective* conf = stream_caster->at(i);
            string n = conf->name;
            if (n != "enabled" && n != "caster" && n != "output"
                && n != "listen" && n != "rtp_port_min" && n != "rtp_port_max"
                ) {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("unsupported stream_caster directive %s, ret=%d", n.c_str(), ret);
                return ret;
            }
        }
    }

    vector<ConfDirective*> vhosts;
    get_vhosts(vhosts);
    for (int n = 0; n < (int)vhosts.size(); n++) {
        ConfDirective* vhost = vhosts[n];
        for (int i = 0; vhost && i < (int)vhost->directives.size(); i++) {
            ConfDirective* conf = vhost->at(i);
            string n = conf->name;
            if (n != "enabled" && n != "chunk_size"
                && n != "mode" && n != "origin" && n != "token_traverse" && n != "vhost"
                && n != "dvr" && n != "ingest" && n != "hls" && n != "http_hooks"
                && n != "gop_cache" && n != "queue_length"
                && n != "refer" && n != "refer_publish" && n != "refer_play"
                && n != "forward" && n != "transcode" && n != "bandcheck"
                && n != "time_jitter" && n != "mix_correct"
                && n != "atc" && n != "atc_auto"
                && n != "debug_srs_upnode"
                && n != "mr" && n != "mw_latency" && n != "min_latency" && n != "publish"
                && n != "tcp_nodelay" && n != "send_min_interval" && n != "reduce_sequence_header"
                && n != "publish_1stpkt_timeout" && n != "publish_normal_timeout"
                && n != "security" && n != "http_remux"
                && n != "http" && n != "http_static"
                && n != "hds"
            ) {
                ret = ERROR_SYSTEM_CONFIG_INVALID;
                srs_error("unsupported vhost directive %s, ret=%d", n.c_str(), ret);
                return ret;
            }
            // for each sub directives of vhost.
            if (n == "dvr") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "dvr_path" && m != "dvr_plan"
                        && m != "dvr_duration" && m != "dvr_wait_keyframe" && m != "time_jitter"
                        ) {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost dvr directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "mr") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "latency"
                        ) {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost mr directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "publish") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "parse_sps"
                        ) {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost publish directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "ingest") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "input" && m != "ffmpeg"
                        && m != "engine"
                        ) {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost ingest directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "http" || n == "http_static") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "mount" && m != "dir") {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost http directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "http_remux") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "mount" && m != "fast_cache" && m != "hstrs") {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost http_remux directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "hls") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "hls_entry_prefix" && m != "hls_path" && m != "hls_fragment" && m != "hls_window" && m != "hls_on_error"
                        && m != "hls_storage" && m != "hls_mount" && m != "hls_td_ratio" && m != "hls_aof_ratio" && m != "hls_acodec" && m != "hls_vcodec"
                        && m != "hls_m3u8_file" && m != "hls_ts_file" && m != "hls_ts_floor" && m != "hls_cleanup" && m != "hls_nb_notify"
                        && m != "hls_wait_keyframe" && m != "hls_dispose"
                        ) {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost hls directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "http_hooks") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "on_connect" && m != "on_close" && m != "on_publish"
                        && m != "on_unpublish" && m != "on_play" && m != "on_stop"
                        && m != "on_dvr" && m != "on_hls" && m != "on_hls_notify"
                        ) {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost http_hooks directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "forward") {
                // TODO: FIXME: implements it.
                /*for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "vhost" && m != "refer") {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost forward directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }*/
            } else if (n == "security") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    ConfDirective* security = conf->at(j);
                    string m = security->name.c_str();
                    if (m != "enabled" && m != "deny" && m != "allow") {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost security directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            } else if (n == "transcode") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    ConfDirective* trans = conf->at(j);
                    string m = trans->name.c_str();
                    if (m != "enabled" && m != "ffmpeg" && m != "engine") {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost transcode directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                    if (m == "engine") {
                        for (int k = 0; k < (int)trans->directives.size(); k++) {
                            string e = trans->at(k)->name;
                            if (e != "enabled" && e != "vfilter" && e != "vcodec"
                                && e != "vbitrate" && e != "vfps" && e != "vwidth" && e != "vheight"
                                && e != "vthreads" && e != "vprofile" && e != "vpreset" && e != "vparams"
                                && e != "acodec" && e != "abitrate" && e != "asample_rate" && e != "achannels"
                                && e != "aparams" && e != "output"
                                && e != "iformat" && e != "oformat"
                                ) {
                                ret = ERROR_SYSTEM_CONFIG_INVALID;
                                srs_error("unsupported vhost transcode engine directive %s, ret=%d", e.c_str(), ret);
                                return ret;
                            }
                        }
                    }
                }
            } else if (n == "bandcheck") {
                for (int j = 0; j < (int)conf->directives.size(); j++) {
                    string m = conf->at(j)->name.c_str();
                    if (m != "enabled" && m != "key" && m != "interval" && m != "limit_kbps") {
                        ret = ERROR_SYSTEM_CONFIG_INVALID;
                        srs_error("unsupported vhost bandcheck directive %s, ret=%d", m.c_str(), ret);
                        return ret;
                    }
                }
            }
        }
    }
    // check ingest id unique.
    for (int i = 0; i < (int)vhosts.size(); i++) {
        ConfDirective* vhost = vhosts[i];
        std::vector<std::string> ids;

        for (int j = 0; j < (int)vhost->directives.size(); j++) {
            ConfDirective* conf = vhost->at(j);
            if (conf->name != "ingest") {
                continue;
            }

            std::string id = conf->arg0();
            for (int k = 0; k < (int)ids.size(); k++) {
                if (id == ids.at(k)) {
                    ret = ERROR_SYSTEM_CONFIG_INVALID;
                    srs_error("directive \"ingest\" id duplicated, vhost=%s, id=%s, ret=%d",
                              vhost->name.c_str(), id.c_str(), ret);
                    return ret;
                }
            }
            ids.push_back(id);
        }
    }
    
    ////////////////////////////////////////////////////////////////////////
    // check chunk size
    ////////////////////////////////////////////////////////////////////////
    if (get_global_chunk_size() < SRS_CONSTS_RTMP_MIN_CHUNK_SIZE 
        || get_global_chunk_size() > SRS_CONSTS_RTMP_MAX_CHUNK_SIZE
    ) {
        ret = ERROR_SYSTEM_CONFIG_INVALID;
        srs_error("directive chunk_size invalid, chunk_size=%d, must in [%d, %d], ret=%d", 
            get_global_chunk_size(), SRS_CONSTS_RTMP_MIN_CHUNK_SIZE, 
            SRS_CONSTS_RTMP_MAX_CHUNK_SIZE, ret);
        return ret;
    }
    for (int i = 0; i < (int)vhosts.size(); i++) {
        ConfDirective* vhost = vhosts[i];
        if (get_chunk_size(vhost->arg0()) < SRS_CONSTS_RTMP_MIN_CHUNK_SIZE 
            || get_chunk_size(vhost->arg0()) > SRS_CONSTS_RTMP_MAX_CHUNK_SIZE
        ) {
            ret = ERROR_SYSTEM_CONFIG_INVALID;
            srs_error("directive vhost %s chunk_size invalid, chunk_size=%d, must in [%d, %d], ret=%d", 
                vhost->arg0().c_str(), get_chunk_size(vhost->arg0()), SRS_CONSTS_RTMP_MIN_CHUNK_SIZE, 
                SRS_CONSTS_RTMP_MAX_CHUNK_SIZE, ret);
            return ret;
        }
    }
    for (int i = 0; i < (int)vhosts.size(); i++) {
        ConfDirective* vhost = vhosts[i];
        srs_assert(vhost != NULL);
#ifndef SRS_AUTO_DVR
        if (get_dvr_enabled(vhost->arg0())) {
            srs_warn("dvr of vhost %s is disabled by configure", vhost->arg0().c_str());
        }
#endif
#ifndef SRS_AUTO_HLS
        if (get_hls_enabled(vhost->arg0())) {
            srs_warn("hls of vhost %s is disabled by configure", vhost->arg0().c_str());
        }
#endif
#ifndef SRS_AUTO_HTTP_CALLBACK
        if (get_vhost_http_hooks_enabled(vhost->arg0())) {
            srs_warn("http_hooks of vhost %s is disabled by configure", vhost->arg0().c_str());
        }
#endif
#ifndef SRS_AUTO_TRANSCODE
        if (get_transcode_enabled(get_transcode(vhost->arg0(), ""))) {
            srs_warn("transcode of vhost %s is disabled by configure", vhost->arg0().c_str());
        }
#endif
#ifndef SRS_AUTO_INGEST
        vector<ConfDirective*> ingesters = get_ingesters(vhost->arg0());
        for (int j = 0; j < (int)ingesters.size(); j++) {
            ConfDirective* ingest = ingesters[j];
            if (get_ingest_enabled(ingest)) {
                srs_warn("ingest %s of vhost %s is disabled by configure", 
                    ingest->arg0().c_str(), vhost->arg0().c_str()
                );
            }
        }
#endif
        // TODO: FIXME: required http server when hls storage is ram or both.
    }
#endif
    
    return ret;
}

int Config::parse_buffer(ConfigBuffer* buffer)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = root->parse(buffer)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // mock by dolphin mode.
    // for the dolphin will start srs with specified params.
    if (dolphin) {
        // for RTMP.
        set_config_directive(root, "listen", dolphin_rtmp_port);
        
        // for HTTP
        set_config_directive(root, "http_server", "");
        ConfDirective* http_server = root->get("http_server");
        set_config_directive(http_server, "enabled", "on");
        set_config_directive(http_server, "listen", dolphin_http_port);
        
        // others.
        set_config_directive(root, "daemon", "off");
        set_config_directive(root, "srs_log_tank", "console");
    }

    return ret;
}

string Config::cwd()
{
    return _cwd;
}

string Config::argv()
{
    return _argv;
}

bool Config::get_deamon()
{
    srs_assert(root);
    
    ConfDirective* conf = root->get("daemon");
    if (!conf || conf->arg0().empty()) {
        return true;
    }
    
    return SRS_CONF_PERFER_TRUE(conf->arg0());
}

ConfDirective* Config::get_root()
{
    return root;
}

int Config::get_max_connections()
{
    srs_assert(root);
    
    ConfDirective* conf = root->get("max_connections");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_MAX_CONNECTIONS;
    }
    
    return ::atoi(conf->arg0().c_str());
}

vector<string> Config::get_listens()
{
    std::vector<string> ports;
    
    ConfDirective* conf = root->get("listen");
    if (!conf) {
        return ports;
    }
    
    for (int i = 0; i < (int)conf->args.size(); i++) {
        ports.push_back(conf->args.at(i));
    }
    
    return ports;
}

string Config::get_pid_file()
{
    ConfDirective* conf = root->get("pid");
    
    if (!conf) {
        return SRS_CONF_DEFAULT_PID_FILE;
    }
    
    return conf->arg0();
}

int Config::get_pithy_print_ms()
{
    ConfDirective* pithy = root->get("pithy_print_ms");
    if (!pithy || pithy->arg0().empty()) {
        return SRS_CONF_DEFAULT_PITHY_PRINT_MS;
    }
    
    return ::atoi(pithy->arg0().c_str());
}

bool Config::get_utc_time()
{
    ConfDirective* conf = root->get("utc_time");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_UTC_TIME;
    }

    return SRS_CONF_PERFER_FALSE(conf->arg0());
}

bool Config::get_log_tank_file()
{
    srs_assert(root);
    
    ConfDirective* conf = root->get("srs_log_tank");
    if (!conf || conf->arg0().empty()) {
        return true;
    }
    
    return conf->arg0() != SRS_CONF_DEFAULT_LOG_TANK_CONSOLE;
}

string Config::get_log_level()
{
    srs_assert(root);
    
    ConfDirective* conf = root->get("srs_log_level");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_LOG_LEVEL;
    }
    
    return conf->arg0();
}

string Config::get_log_file()
{
    srs_assert(root);
    
    ConfDirective* conf = root->get("srs_log_file");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_LOG_FILE;
    }
    
    return conf->arg0();
}

ConfDirective* Config::get_heartbeart()
{
    return root->get("heartbeat");
}

bool Config::get_heartbeat_enabled()
{
    ConfDirective* conf = get_heartbeart();

    if (!conf) {
        return SRS_CONF_DEFAULT_HTTP_HEAETBEAT_ENABLED;
    }

    conf = conf->get("enabled");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_HTTP_HEAETBEAT_ENABLED;
    }

    return SRS_CONF_PERFER_FALSE(conf->arg0());
}

int64_t Config::get_heartbeat_interval()
{
    ConfDirective* conf = get_heartbeart();

    if (!conf) {
        return (int64_t)(SRS_CONF_DEFAULT_HTTP_HEAETBEAT_INTERVAL * 1000);
    }
    conf = conf->get("interval");
    if (!conf || conf->arg0().empty()) {
        return (int64_t)(SRS_CONF_DEFAULT_HTTP_HEAETBEAT_INTERVAL * 1000);
    }

    return (int64_t)(::atof(conf->arg0().c_str()) * 1000);
}

string Config::get_heartbeat_url()
{
    ConfDirective* conf = get_heartbeart();

    if (!conf) {
        return SRS_CONF_DEFAULT_HTTP_HEAETBEAT_URL;
    }

    conf = conf->get("url");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_HTTP_HEAETBEAT_URL;
    }

    return conf->arg0();
}

string Config::get_heartbeat_device_id()
{
    ConfDirective* conf = get_heartbeart();

    if (!conf) {
        return "";
    }

    conf = conf->get("device_id");
    if (!conf || conf->arg0().empty()) {
        return "";
    }

    return conf->arg0();
}

bool Config::get_heartbeat_summaries()
{
    ConfDirective* conf = get_heartbeart();

    if (!conf) {
        return SRS_CONF_DEFAULT_HTTP_HEAETBEAT_SUMMARIES;
    }

    conf = conf->get("summaries");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_HTTP_HEAETBEAT_SUMMARIES;
    }

    return SRS_CONF_PERFER_FALSE(conf->arg0());
}

ConfDirective* Config::get_stats()
{
    return root->get("stats");
}

int Config::get_stats_network()
{
    ConfDirective* conf = get_stats();

    if (!conf) {
        return SRS_CONF_DEFAULT_STATS_NETWORK_DEVICE_INDEX;
    }

    conf = conf->get("network");
    if (!conf || conf->arg0().empty()) {
        return SRS_CONF_DEFAULT_STATS_NETWORK_DEVICE_INDEX;
    }

    return ::atoi(conf->arg0().c_str());
}

ConfDirective* Config::get_stats_disk_device()
{
    ConfDirective* conf = get_stats();

    if (!conf) {
        return NULL;
    }

    conf = conf->get("disk");
    if (!conf || conf->args.size() == 0) {
        return NULL;
    }

    return conf;
}

namespace __internal
{
    ConfigBuffer::ConfigBuffer()
    {
        line = 1;

        pos = last = start = NULL;
        end = start;
    }

    ConfigBuffer::~ConfigBuffer()
    {
        srs_freepa(start);
    }

    int ConfigBuffer::fullfill(const char* filename)
    {
        int ret = ERROR_SUCCESS;

        FileReader reader;

        // open file reader.
        if ((ret = reader.open(filename)) != ERROR_SUCCESS) {
            srs_error("open conf file error. ret=%d", ret);
            return ret;
        }

        // read all.
        int filesize = (int)reader.filesize();

        // create buffer
        srs_freepa(start);
        pos = last = start = new char[filesize];
        end = start + filesize;

        // read total content from file.
        ssize_t nread = 0;
        if ((ret = reader.read(start, filesize, &nread)) != ERROR_SUCCESS) {
            srs_error("read file read error. expect %d, actual %d bytes, ret=%d",
                filesize, nread, ret);
            return ret;
        }

        return ret;
    }

    bool ConfigBuffer::empty()
    {
        return pos >= end;
    }
};

bool srs_directive_equals(ConfDirective* a, ConfDirective* b)
{
    // both NULL, equal.
    if (!a && !b) {
        return true;
    }

    if (!a || !b) {
        return false;
    }

    if (a->name != b->name) {
        return false;
    }

    if (a->args.size() != b->args.size()) {
        return false;
    }

    for (int i = 0; i < (int)a->args.size(); i++) {
        if (a->args.at(i) != b->args.at(i)) {
            return false;
        }
    }

    if (a->directives.size() != b->directives.size()) {
        return false;
    }

    for (int i = 0; i < (int)a->directives.size(); i++) {
        ConfDirective* a0 = a->at(i);
        ConfDirective* b0 = b->at(i);

        if (!srs_directive_equals(a0, b0)) {
            return false;
        }
    }

    return true;
}

#if 0
bool srs_config_hls_is_on_error_ignore(string strategy)
{
    return strategy == SRS_CONF_DEFAULT_HLS_ON_ERROR_IGNORE;
}

bool srs_config_hls_is_on_error_continue(string strategy)
{
    return strategy == SRS_CONF_DEFAULT_HLS_ON_ERROR_CONTINUE;
}

bool srs_config_ingest_is_file(string type)
{
    return type == SRS_CONF_DEFAULT_INGEST_TYPE_FILE;
}

bool srs_config_ingest_is_stream(string type)
{
    return type == SRS_CONF_DEFAULT_INGEST_TYPE_STREAM;
}

bool srs_config_dvr_is_plan_segment(string plan)
{
    return plan == SRS_CONF_DEFAULT_DVR_PLAN_SEGMENT;
}

bool srs_config_dvr_is_plan_session(string plan)
{
    return plan == SRS_CONF_DEFAULT_DVR_PLAN_SESSION;
}

bool srs_config_dvr_is_plan_append(string plan)
{
    return plan == SRS_CONF_DEFAULT_DVR_PLAN_APPEND;
}

bool srs_stream_caster_is_udp(string caster)
{
    return caster == SRS_CONF_DEFAULT_STREAM_CASTER_MPEGTS_OVER_UDP;
}

bool srs_stream_caster_is_rtsp(string caster)
{
    return caster == SRS_CONF_DEFAULT_STREAM_CASTER_RTSP;
}

bool srs_stream_caster_is_flv(string caster)
{
    return caster == SRS_CONF_DEFAULT_STREAM_CASTER_FLV;
}
#endif
