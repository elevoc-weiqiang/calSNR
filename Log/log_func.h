#pragma once

#include <stdio.h>

void ngx_log_error_core(const char* name, int level, int err, const char* fmt, ...);
void ngx_log_level(int level = 6);

unsigned char* ngx_vslprintf(unsigned char* buf, unsigned char* last, const char* fmt, va_list args);
unsigned char* ngx_log_errno(unsigned char* buf, unsigned char* last, int err);
unsigned char* ngx_slprintf(unsigned char* buf, unsigned char* last, const char* fmt, ...);