#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno

#include "log_macro.h"
#include "log_func.h"

#include <Windows.h>

void write_file(const char* msg, size_t len)
{
    FILE* fp;
    errno_t err = fopen_s(&fp, NGX_ERROR_LOG_PATH, "at");
    if (0 != err)
    {
        return;
    }
    fprintf(fp, msg);
    fclose(fp);
}

void getTimeStr(unsigned char* strCurrTime)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    ngx_slprintf(strCurrTime, (unsigned char*)-1, "%4d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

//等级错误
static unsigned char err_levels[][20] =
{
    {"stderr"},    //0：控制台错误
    {"emerg"},     //1：紧急
    {"alert"},     //2：警戒
    {"crit"},      //3：严重
    {"error"},     //4：错误
    {"warn"},      //5：警告
    {"notice"},    //6：注意
    {"info"},      //7：信息
    {"debug"},     //8：调试
    {"trace"}      //9：
};

int g_log_level = NGX_LOG_NOTICE;

//----------------------------------------------------------------------------------------------------------------------
//描述：给一段内存，一个错误编号，我要组合出一个字符串，形如：   (错误编号: 错误原因)，放到给的这段内存中去
//     这个函数我改造的比较多，和原始的nginx代码多有不同
//buf：是个内存，要往这里保存数据
//last：放的数据不要超过这里
//err：错误编号，我们是要取得这个错误编号对应的错误字符串，保存到buffer中
unsigned char* ngx_log_errno(unsigned char* buf, unsigned char* last, int err)
{
    char* perrorInfo = strerror(err);
    size_t len = strlen(perrorInfo);

    char leftStr[10] = { 0 };
    sprintf(leftStr, "(%d: ", err);
    size_t lLen = strlen(leftStr);

    char rightStr[] = ")";
    size_t rLen = strlen(rightStr);

    if ((buf + lLen + len + rLen) < last)
    {
        buf = ngx_cpymem(buf, leftStr, lLen);
        buf = ngx_cpymem(buf, perrorInfo, len);
        buf = ngx_cpymem(buf, rightStr, rLen);
    }

    return buf;
}
/**
* 定向为标准错误，则直接往屏幕上写日志【比如日志文件打不开，则会直接定位到标准错误，此时日志就打印到屏幕上，参考ngx_log_init()】
* level:一个等级数字，我们把日志分成一些等级，以方便管理、显示、过滤等等，如果这个等级数字比配置文件中的等级数字"LogLevel"大，那么该条信息不被写到日志文件中
* err：是个错误代码，如果不是0，就应该转换成显示对应的错误信息,一起写到日志文件中，
* ngx_log_error_core(5,8,"这个XXX工作的有问题,显示的结果是=%s","YYYY");
*/
void ngx_log_error_core(const char* name, int level, int err, const char* fmt, ...)
{
    if (level > g_log_level)
    {
        return;
    }

    unsigned char* last;
    unsigned char errStr[MAX_ERR_LEN + 1];

    memset(errStr, 0, sizeof(errStr));

    last = errStr + MAX_ERR_LEN;
    unsigned char* p;    //指向当前要拷贝数据到其中的内存位置
    va_list        args;

    unsigned char strCurrTime[40] = { 0 };
    getTimeStr(strCurrTime);

    p = ngx_cpymem(errStr, strCurrTime, strlen((const char*)strCurrTime));
    //日志等级
    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);
    p = ngx_slprintf(p, last, " [%s] ", name);
    //进程号
    //p = ngx_slprintf(p, last, " [%p] ", log_pid);
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

    //错误码不为0，表示发生错误
    if (err)
    {
        p = ngx_log_errno(p, last, err);
    }

    if (p > (last - 1))
    {
        p = (last - 1) - 1;
    }

    *p++ = '\n';
    write_file((const char*)errStr, p - errStr);
    return;
}

void ngx_log_level(int level)
{
    g_log_level = level;
}