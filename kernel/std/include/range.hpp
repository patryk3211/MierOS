#pragma once

namespace std {
    template<typename T> struct Range {
        T start;
        T end;

        Range() { }
        Range(T start, T end) : start(start), end(end) { }
        ~Range() { }
    };
}
