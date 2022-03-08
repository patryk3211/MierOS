#ifndef _MIEROS_STANDARD_ATOMIC_HPP
#error "This header cannot be included directly"
#endif

template<typename T> T Atomic<T>::load() {
    T temp;
    asm volatile("mov %1, %0; mfence" : "=a"(temp) : "m"(value) : "memory");
    return temp;
}

template<typename T> void Atomic<T>::store(T val) {
    asm volatile("mov %1, %0; mfence" : "=m"(value) : "a"(val) : "memory");
}

template<typename T> T Atomic<T>::fetch_add(T val) {
    T ret;
    asm volatile("lock xadd %2, %1; mov %2, %0" : "=m"(ret) : "m"(value), "a"(val) : "memory");
    return ret;
}

template<typename T> T Atomic<T>::fetch_sub(T val) {
    T ret;
    asm volatile("lock xadd %2, %1; mov %2, %0" : "=m"(ret) : "m"(value), "a"(-val) : "memory");
    return ret;
}
