/*
    Utility.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "Utility.h"
#include <limits>
#include <mutex>
#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#if __ANDROID__
#    include <android/log.h>
#endif

static std::string formatStringV(const char *fmt, va_list ap0, va_list ap1) {
    int len = vsnprintf(NULL, 0, fmt, ap0);
    if (len < 0) {
        throw std::runtime_error("vsnprintf() failed");
    }
    else if ((size_t)len == std::numeric_limits<size_t>::max()) {
        throw std::runtime_error("vsnprintf() length would exceed size_t range");
    }
    char *s = new char[(size_t)len + 1];
    vsnprintf(s, (size_t)len + 1, fmt, ap1);
    std::string ss(s);
    delete[] s;
    return ss;
}

std::string GuiSupport::formatString(const char *fmt, ...) {
    va_list ap0;
    va_list ap1;
    va_start(ap0, fmt);
    va_start(ap1, fmt);
    auto s = formatStringV(fmt, ap0, ap1);
    va_end(ap0);
    va_end(ap1);
    return s;
}

#if __ANDROID__

/* Dynamically allocated to prevent destruction on main thread exit (may be in
   use by background threads) */
static std::mutex *_logPrefixLock = new std::mutex;
static std::string _logPrefix = "NOT_SET";

void GuiSupport::setSystemLogPrefix(const char *pfx) {
    _logPrefixLock->lock();
    _logPrefix = std::string(pfx);
    _logPrefixLock->unlock();
}

void GuiSupport::log(const char *fmt, ...) {
    _logPrefixLock->lock();
    std::string tag = _logPrefix;
    _logPrefixLock->unlock();

    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, tag.c_str(), fmt, ap);
    va_end(ap);
}

#else

void GuiSupport::setSystemLogPrefix(const char *) {}

void GuiSupport::log(const char *fmt, ...) {
    va_list ap0;
    va_list ap1;
    va_start(ap0, fmt);
    va_start(ap1, fmt);
    fputs((formatStringV(fmt, ap0, ap1) + "\n").c_str(), stderr);
    va_end(ap0);
    va_end(ap1);
}

#endif
