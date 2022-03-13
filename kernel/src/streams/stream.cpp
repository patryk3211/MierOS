#include <streams/stream.hpp>

using namespace kernel;

size_t Stream::read(void*, size_t) {
    return 0;
}

size_t Stream::write(const void*, size_t) {
    return 0;
}

size_t Stream::seek(size_t, int) {
    return 0;
}
