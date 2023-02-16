#include <streams/dmesgstream.hpp>
#include <dmesg.h>
#include <string.h>

using namespace kernel;

DMesgStream::DMesgStream()
    : Stream(STREAM_TYPE_DMESG) {
    f_buffer_pos = 0;
}

ValueOrError<size_t> DMesgStream::write(const void *buffer, size_t length) {
    const char* currentBuffer = static_cast<const char*>(buffer);
    size_t leftToRead = length;

    while(leftToRead) {
        size_t lengthLeft = sizeof(f_buffer) - f_buffer_pos - 1; // We need 1 byte for the null char
        size_t lengthWritten = length < lengthLeft ? length : lengthLeft;

        memcpy(f_buffer + f_buffer_pos, currentBuffer, lengthWritten);
        f_buffer[f_buffer_pos + lengthWritten] = 0;

        // Move all pointers
        f_buffer_pos += lengthWritten;
        leftToRead -= lengthWritten;
        currentBuffer += lengthWritten;

        // Call dmesg for each line in buffer.
        char* delimiter;
        while((delimiter = strchr(f_buffer, '\n'))) {
            *delimiter = 0;
            dmesg(f_buffer);

            size_t printedLength = delimiter - f_buffer + 1;
            memcpy(f_buffer, f_buffer + printedLength, f_buffer_pos - printedLength);
            f_buffer_pos -= printedLength;
        }

        if(f_buffer_pos == sizeof(f_buffer) - 1) {
            // We need to send out the buffer since it's full.
            f_buffer[sizeof(f_buffer) - 1] = 0;
            dmesg(f_buffer);
            f_buffer_pos = 0;
        }
    }

    return length;
}

