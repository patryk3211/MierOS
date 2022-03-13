#include "fs_func.hpp"
#include "mount_info.hpp"
#include "data_storage.hpp"

using namespace kernel;

extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

ValueOrError<void> open(u16_t minor, FileStream* filestream, int mode) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    ASSERT_F(mi.filesystem == filestream->node()->filesystem(), "Using a filestream from a different filesystem");

    filestream->fs_data = new Ext2StreamDataStorage(mi);

    /// TODO: [13.03.2022] Check if this file can be opened in the given mode and if permissions match.
    return { };
}

ValueOrError<void> close(u16_t minor, FileStream* filestream) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    ASSERT_F(mi.filesystem == filestream->node()->filesystem(), "Using a filestream from a different filesystem");

    delete static_cast<Ext2StreamDataStorage*>(filestream->fs_data);

    /// TODO: [13.03.2022] Flush the buffer and do some other things too probably.
    return { };
}
