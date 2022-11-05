#pragma once

#include "log_func.h"
#include "log_macro.h"

#define DUMP_PATH "C:\\ElevocTest\\"
#define LOG_PATH "C:\\ElevocTest\\ElevocUAPO.log"
//#define SPEECH_LOG_PATH "C:\\ElevocTest\\ElevocSpeechAPO.log"

#define DEFAULT_LOG "default"
#define SPEECH_LOG "speech"
#define RAW_LOG "raw"


#define LOG_TRACE(logger_name, format, ...) ngx_log_error_core(logger_name,NGX_LOG_TRACE, 0, format, __VA_ARGS__);
#define LOG_DEBUG(logger_name, format, ...) ngx_log_error_core(logger_name,NGX_LOG_DEBUG, 0, format, __VA_ARGS__);
#define LOG_INFO(logger_name, format, ...)  ngx_log_error_core(logger_name,NGX_LOG_INFO, 0, format, __VA_ARGS__);
#define LOG_WARN(logger_name, format, ...)  ngx_log_error_core(logger_name,NGX_LOG_WARN, 0, format, __VA_ARGS__);
#define LOG_ERROR(logger_name, format, ...) ngx_log_error_core(logger_name,NGX_LOG_ERR, 0, format, __VA_ARGS__);

#define ENABLE_LOG(level) LogHelper::EnableLog(level)
#define DISABLE_LOG() LogHelper::DisableLog()

static bool g_log_state = false;

class LogHelper
{
public:
	static void Init()
	{
		
	}

	static void EnableLog(int level)
	{
		level == NGX_LOG_OFF ? g_log_state = false : g_log_state = true;
		ngx_log_level(level);
	}

	static void DisableLog()
	{
		ngx_log_level(NGX_LOG_OFF);
	}

	static bool GetLogState()
	{
		return g_log_state;
	}

	static void Close()
	{
		
	}
};


