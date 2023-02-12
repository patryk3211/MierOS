#include <streams/streamwrapper.hpp>

using namespace kernel;

StreamWrapper::StreamWrapper(Stream* base)
    : Stream(base->type()) {
    f_data = new DataStorage();
    f_data->f_base = base;
    f_data->f_referenceCount.store(1);
}
        
StreamWrapper::StreamWrapper(const StreamWrapper& other)
    : Stream(other.type())
    , f_data(other.f_data) {
    f_data->f_referenceCount.fetch_add(1);
}

StreamWrapper::StreamWrapper(StreamWrapper&& other)
    : Stream(other.type())
    , f_data(other.leak()) { }

StreamWrapper& StreamWrapper::operator=(const StreamWrapper& other) {
    clear();
    f_data = other.f_data;
    f_data->f_referenceCount.fetch_add(1);
    return *this;
}

StreamWrapper& StreamWrapper::operator=(StreamWrapper&& other) {
    clear();
    f_data = other.leak();
    return *this;
}

StreamWrapper::~StreamWrapper() {
    clear();
}

ValueOrError<size_t> StreamWrapper::read(void* buffer, size_t length) {
    return f_data->f_base->read(buffer, length);
}

ValueOrError<size_t> StreamWrapper::write(const void* buffer, size_t length) {
    return f_data->f_base->write(buffer, length);
}

ValueOrError<size_t> StreamWrapper::seek(size_t position, int mode) {
    return f_data->f_base->seek(position, mode);
}

Stream& StreamWrapper::base() {
    return *f_data->f_base;
}

void StreamWrapper::clear() {
    if(f_data->f_referenceCount.fetch_sub(1) == 1) {
        // Last reference
        delete f_data->f_base;
        delete f_data;
    }
    f_data = 0;
}

StreamWrapper::DataStorage* StreamWrapper::leak() {
    auto* ret = f_data;
    f_data = 0;
    return ret;
}

