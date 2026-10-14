/*
    Utility.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once
#include <chrono>
#include <cmath>
#include <string>

namespace GuiSupport {
    std::string formatString(const char *fmt, ...);

    // Android log tag, ignored on desktop platforms
    void setSystemLogPrefix(const char *);

    // __android_log_print() on Android, stderr on desktop platforms
    void log(const char *fmt, ...);

    class RateMonitor {
        const double alpha = 0.25;
        const double epsilon = 0.0001;
        bool haveLastTime = false;
        std::chrono::steady_clock::time_point lastTime;
        double period = 0.0;

    public:
        double rate() {
            return std::fabs(period) >= epsilon ? 1 / period : NAN;
        }
        void tick() {
            auto now = std::chrono::steady_clock::now();
            if (haveLastTime) {
                double dt = std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastTime).count() / 1.0e9;
                period = alpha * dt + (1 - alpha) * period;
            }
            lastTime = now;
            haveLastTime = true;
        }
        void reset() {
            haveLastTime = false;
            period = 0.0;
        }
    };
}
