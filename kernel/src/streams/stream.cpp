#include <streams/stream.hpp>

using namespace kernel;

ValueOrError<size_t> Stream::read(void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> Stream::write(const void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> Stream::seek(size_t, int) {
    return ENOTSUP;
}
