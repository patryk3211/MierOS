#include <streams/pipestream.hpp>

using namespace kernel;

PipeStream::PipeStream(const std::SharedPtr<StreamData>& data, bool write)
    : Stream(STREAM_TYPE_PIPE)
    , f_data(data)
    , f_isWrite(write) {
}

std::Pair<PipeStream*, PipeStream*> PipeStream::make_pair() {
    auto data = std::make_shared<StreamData>();

    auto* readEnd = new PipeStream(data, false);
    auto* writeEnd = new PipeStream(data, true);

    return { readEnd, writeEnd };
}

PipeStream::StreamData::StreamData() {
    f_writeClosed = false;
    f_readClosed = false;

    f_writeHead = 0;
    f_readHead = 0;
}

PipeStream::~PipeStream() {
    // We are closing the PipeStream twice
    if(f_isWrite) {
        f_data->f_writeClosed = true;
    } else {
        f_data->f_readClosed = true;
    }
}

ValueOrError<size_t> PipeStream::read(void *buffer, size_t length) {
    if(f_isWrite)
        return ENOTSUP;

    if(f_data->f_writeClosed)
        return 0;

    u8_t* byteBuffer = static_cast<u8_t*>(buffer);

    size_t readCount = 0;
    while(readCount < length) {
        while(f_data->f_readHead == f_data->f_writeHead) {
            if(f_data->f_writeClosed) {
                // Pipe closed mid read
                goto end;
            }
        }

        byteBuffer[readCount++] = f_data->f_buffer[f_data->f_readHead++];
        f_data->f_readHead %= sizeof(f_data->f_buffer);
    }

end:
    return readCount;
}

ValueOrError<size_t> PipeStream::write(const void *buffer, size_t length) {
    if(!f_isWrite)
        return ENOTSUP;

    if(f_data->f_readClosed) {
        // TODO: [18.02.2023] We need to send SIGPIPE to caller
        return EPIPE;
    }

    const u8_t* byteBuffer = static_cast<const u8_t*>(buffer);

    size_t writeCount = 0;
    while(writeCount < length) {
        while((f_data->f_writeHead + 1) % sizeof(f_data->f_buffer) == f_data->f_readHead);

        f_data->f_buffer[f_data->f_writeHead++] = byteBuffer[writeCount++];
        f_data->f_writeHead %= sizeof(f_data->f_buffer);
    }

    return writeCount;
}

ValueOrError<size_t> PipeStream::seek(size_t, int) {
    return ESPIPE;
}

