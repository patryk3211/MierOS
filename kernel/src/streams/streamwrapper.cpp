#include <streams/streamwrapper.hpp>
#include <assert.h>

using namespace kernel;

StreamWrapper::StreamWrapper(Stream* base)
    : Stream(base->type()) {
    f_data = new DataStorage();
    f_data->f_base = base;
    f_data->f_referenceCount.store(1);
    flags() = base->flags();
}
        
StreamWrapper::StreamWrapper(const StreamWrapper& other)
    : Stream(other.type())
    , f_data(other.f_data) {
    f_data->f_referenceCount.fetch_add(1);
    flags() = other.flags();
}

StreamWrapper::StreamWrapper(StreamWrapper&& other)
    : Stream(other.type())
    , f_data(other.leak()) {
    flags() = other.flags();
}

StreamWrapper& StreamWrapper::operator=(const StreamWrapper& other) {
    clear();
    f_data = other.f_data;
    f_data->f_referenceCount.fetch_add(1);
    flags() = other.flags();
    return *this;
}

StreamWrapper& StreamWrapper::operator=(StreamWrapper&& other) {
    clear();
    f_data = other.leak();
    flags() = other.flags();
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

/// I think we are defining multiple StreamWrapper on one streams
void StreamWrapper::clear() {
    int refCount = f_data->f_referenceCount.fetch_sub(1);
    ASSERT_F(refCount >= 1, "Clearing a StreamWrapper after all references have been deleted");
    if(refCount == 1) {
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

