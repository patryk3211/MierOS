#pragma once

#include <types.h>
#include <errno.h>

#define SEEK_MODE_BEG 0
#define SEEK_MODE_CUR 1
#define SEEK_MODE_END 2

namespace kernel {
    class Stream {
        unsigned char f_type;
        int f_flags;

    protected:
        Stream(unsigned char type)
            : f_type(type) {
            f_flags = 0;
        }

        void set_type(unsigned char type) {
            f_type = type;
        }

    public:
        virtual ~Stream() = default;

        unsigned char type() const { return f_type; }

        int& flags() { return f_flags; }

        /**
         * @brief Read the given amount of bytes into the buffer.
         * 
         * @param buffer Destination for the read data
         * @param length Amount of data to be read
         * @return size_t Amount of data actually read
         */
        virtual ValueOrError<size_t> read(void* buffer, size_t length);

        /**
         * @brief Write the given amount of bytes from buffer into the stream.
         * 
         * @param buffer Source of data
         * @param length Amount of data to write
         * @return size_t Amount of data actually written
         */
        virtual ValueOrError<size_t> write(const void* buffer, size_t length);

        /**
         * @brief Change the current position of the stream.
         * 
         * @param position New stream position
         * @param mode Move mode
         * @return size_t Old stream position
         */
        virtual ValueOrError<size_t> seek(size_t position, int mode);
    };
}
