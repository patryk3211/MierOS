#include <fs/systemfs.hpp>
#include <fs/vnode.hpp>

using namespace kernel;

SystemFilesystem* SystemFilesystem::s_instance = 0;

SystemFilesystem::SystemFilesystem()
    : f_root(nullptr) {
    
    s_instance = this;
}

SystemFilesystem* SystemFilesystem::instance() {
    return s_instance;
}

ValueOrError<void> SystemFilesystem::open(FileStream* stream, int mode) {
    stream->fs_data = new SysFsStreamData();

    return { };
}

ValueOrError<void> SystemFilesystem::close(FileStream* stream) {
    delete static_cast<SysFsStreamData*>(stream->fs_data);

    return { };
}

ValueOrError<size_t> SystemFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    auto* vnodeData = static_cast<SysFsVNodeData*>(stream->node()->fs_data); 
    auto* streamData = static_cast<SysFsStreamData*>(stream->fs_data);

    // Sanitize the input arguments before passing them
    // to the node's callback
    size_t lengthLeft = stream->node()->f_size - streamData->offset;
    if(length > lengthLeft) length = lengthLeft;

    if(vnodeData->read_callback) {
        // This node provides a read callback.
        return vnodeData->read_callback(vnodeData->callback_arg, stream, buffer, length);
    } else if(vnodeData->static_data) {
        // If the node doesn't provide a read callback it can provide static data
        // (or it should have been set as write-only)
        memcpy(buffer, (u8_t*)vnodeData->static_data + streamData->offset, length);
        streamData->offset += length;
        return length;
    } else {
        // We can also use the small data field
        if(streamData->offset > sizeof(vnodeData->small_data))
            return 0;
        if(length > sizeof(vnodeData->small_data) - streamData->offset)
            length = sizeof(vnodeData->small_data) - streamData->offset;

        memcpy(buffer, vnodeData->small_data + streamData->offset, length);
        streamData->offset += length;
        return length;
    }
}

ValueOrError<size_t> SystemFilesystem::write(FileStream* stream, const void* buffer, size_t length) {
    auto* vnodeData = static_cast<SysFsVNodeData*>(stream->node()->fs_data);
    
    ASSERT_F(vnodeData->write_callback, "A writable node must provide a write callback");
    return vnodeData->write_callback(vnodeData->callback_arg, stream, buffer, length);
}

ValueOrError<size_t> SystemFilesystem::seek(FileStream* stream, size_t position, int mode) {
    auto* stream_data = reinterpret_cast<SysFsStreamData*>(stream->fs_data);
    size_t old_offset = stream_data->offset;
    switch(mode) {
        case SEEK_MODE_CUR:
            stream_data->offset += position;
            break;
        case SEEK_MODE_BEG:
            stream_data->offset = position;
            break;
        case SEEK_MODE_END:
            stream_data->offset = stream->node()->f_size + position;
            break;
    }
    return old_offset;
}

ValueOrError<void> SystemFilesystem::stat(VNodePtr node, mieros_stat* stat) {
    node->stat(stat);
    return { };
}

ValueOrError<SysFsVNodeData*> SystemFilesystem::add_node(VNodePtr root, const char* path) {
    auto result = resolve_path(path, root);
    if(!result)
        return result.errno();
    if(result->key->f_children.at(result->value))
        return EEXIST;

    auto node = std::make_shared<VNode>(0, 0, 0, 0, 0, 0, 0, result->value, VNode::FILE, this);

    auto* data = new SysFsVNodeData(node);
    node->fs_data = data;

    result->key->add_child(node);
    node->f_parent = result->key;

    return data;
}

