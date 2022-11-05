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

//�ȼ�����
static unsigned char err_levels[][20] =
{
    {"stderr"},    //0������̨����
    {"emerg"},     //1������
    {"alert"},     //2������
    {"crit"},      //3������
    {"error"},     //4������
    {"warn"},      //5������
    {"notice"},    //6��ע��
    {"info"},      //7����Ϣ
    {"debug"},     //8������
    {"trace"}      //9��
};

int g_log_level = NGX_LOG_NOTICE;

//----------------------------------------------------------------------------------------------------------------------
//��������һ���ڴ棬һ�������ţ���Ҫ��ϳ�һ���ַ��������磺   (������: ����ԭ��)���ŵ���������ڴ���ȥ
//     ��������Ҹ���ıȽ϶࣬��ԭʼ��nginx������в�ͬ
//buf���Ǹ��ڴ棬Ҫ�����ﱣ������
//last���ŵ����ݲ�Ҫ��������
//err�������ţ�������Ҫȡ����������Ŷ�Ӧ�Ĵ����ַ��������浽buffer��
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
* ����Ϊ��׼������ֱ������Ļ��д��־��������־�ļ��򲻿������ֱ�Ӷ�λ����׼���󣬴�ʱ��־�ʹ�ӡ����Ļ�ϣ��ο�ngx_log_init()��
* level:һ���ȼ����֣����ǰ���־�ֳ�һЩ�ȼ����Է��������ʾ�����˵ȵȣ��������ȼ����ֱ������ļ��еĵȼ�����"LogLevel"����ô������Ϣ����д����־�ļ���
* err���Ǹ�������룬�������0����Ӧ��ת������ʾ��Ӧ�Ĵ�����Ϣ,һ��д����־�ļ��У�
* ngx_log_error_core(5,8,"���XXX������������,��ʾ�Ľ����=%s","YYYY");
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
    unsigned char* p;    //ָ��ǰҪ�������ݵ����е��ڴ�λ��
    va_list        args;

    unsigned char strCurrTime[40] = { 0 };
    getTimeStr(strCurrTime);

    p = ngx_cpymem(errStr, strCurrTime, strlen((const char*)strCurrTime));
    //��־�ȼ�
    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);
    p = ngx_slprintf(p, last, " [%s] ", name);
    //���̺�
    //p = ngx_slprintf(p, last, " [%p] ", log_pid);
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

    //�����벻Ϊ0����ʾ��������
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