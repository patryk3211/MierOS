#pragma once

#include <types.h>

namespace kernel {
    #define SEEK_MODE_CUR 0
    #define SEEK_MODE_BEG 1
    #define SEEK_MODE_END 2

    class Stream {
        unsigned char f_type;
    protected:
        Stream(unsigned char type) : f_type(type) { }

    public:
        virtual ~Stream() = default;

        unsigned char type() { return f_type; }

        /**
         * @brief Read the given amount of bytes into the buffer.
         * 
         * @param buffer Destination for the read data
         * @param length Amount of data to be read
         * @return size_t Amount of data actually read
         */
        virtual size_t read(void* buffer, size_t length);

        /**
         * @brief Write the given amount of bytes from buffer into the stream.
         * 
         * @param buffer Source of data
         * @param length Amount of data to write
         * @return size_t Amount of data actually written
         */
        virtual size_t write(void* buffer, size_t length);

        /**
         * @brief Change the current position of the stream.
         * 
         * @param position New stream position
         * @param mode Move mode
         * @return size_t Old stream position
         */
        virtual size_t seek(size_t position, int mode);
    };
}
