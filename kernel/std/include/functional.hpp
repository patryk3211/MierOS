#pragma once

namespace std {
    template<typename T> struct less { bool operator()(const T& a, const T& b) const { return a < b; } };
    template<typename T> struct greater { bool operator()(const T& a, const T& b) const { return a > b; } };
}
