#pragma once

#include <streams/stream.hpp>
#include <atomic.hpp>

namespace kernel {
    class StreamWrapper : public Stream {
        struct DataStorage {
            Stream* f_base;
            std::Atomic<int> f_referenceCount;
        } *f_data;
        Stream* f_base;

    public:
        StreamWrapper(Stream* base);
        
        StreamWrapper(const StreamWrapper& other);
        StreamWrapper(StreamWrapper&& other);

        StreamWrapper& operator=(const StreamWrapper& other);
        StreamWrapper& operator=(StreamWrapper&& other);

        ~StreamWrapper();

        virtual size_t read(void* buffer, size_t length) override;
        virtual size_t write(const void* buffer, size_t length) override;

        virtual size_t seek(size_t position, int mode) override;

        Stream& base();

    private:
        void clear();
        DataStorage* leak();
    };
}

