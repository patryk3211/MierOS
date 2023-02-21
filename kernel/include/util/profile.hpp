#pragma once

#include <types.h>
#include <arch/time.h>
#include <dmesg.h>

namespace kernel {
    class Profile {
        const char* f_name;
        time_t f_threshold;
        time_t f_start;
        int f_sample_count;

    public:
        Profile(const char* name, time_t threshold)
            : f_name(name)
            , f_threshold(threshold) {
            f_sample_count = 0;
            f_start = get_uptime();
        }

        ~Profile() {
            time_t end = get_uptime();
            time_t duration = end - f_start;
            if(duration > f_threshold)
                dmesg("(profile) %s took %dms", f_name, duration);
        }

        void sample() {
            time_t duration = get_uptime() - f_start;
            dmesg("(profile) Sample %d, t = %dms", f_sample_count++, duration);
        }

        void sample(const char* name) {
            time_t duration = get_uptime() - f_start;
            dmesg("(profile) Sample %d (%s), t = %dms", f_sample_count, name, duration);
        }
    };
}

