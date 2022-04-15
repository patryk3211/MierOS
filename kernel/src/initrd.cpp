#include <defines.h>
#include <initrd.h>
#include <list.hpp>
#include <stdlib.h>
#include <types.h>

NO_EXPORT void* initrd_root;

struct TarHeader {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];

    size_t getSize() {
        size_t val = 0;
        u32_t mul = 1;

        for(int i = 10; i >= 0; i--) {
            val += (size[i] - '0') * mul;
            mul *= 8;
        }

        return val;
    }
};

TEXT_FREE_AFTER_INIT int oct2bin(char* str, int size) {
    int n = 0;
    char* c = str;
    while(size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

extern "C" TEXT_FREE_AFTER_INIT void set_initrd(void* ptr) {
    initrd_root = ptr;
}

extern "C" TEXT_FREE_AFTER_INIT void* get_file(const char* name) {
    TarHeader* header = (TarHeader*)initrd_root;
    while(memcmp((char*)header + 257, "ustar", 5)) {
        int fileSize = oct2bin(header->size, 11);
        if(!strcmp(header->filename, name)) {
            return (u8_t*)header + 512;
        }
        header = (TarHeader*)((virtaddr_t)header + 512 + ((fileSize / 512 + (fileSize % 512 == 0 ? 0 : 1)) * 512));
    }
    return 0;
}

extern "C" TEXT_FREE_AFTER_INIT void** get_files(const char* name_wildcard) {
    TarHeader* header = (TarHeader*)initrd_root;
    std::List<void*> files;
    while(memcmp((char*)header + 257, "ustar", 5)) {
        int fileSize = oct2bin(header->size, 11);
        if(strmatch(name_wildcard, header->filename)) files.push_back((u8_t*)header + 512);
        header = (TarHeader*)((virtaddr_t)header + 512 + ((fileSize / 512 + (fileSize % 512 == 0 ? 0 : 1)) * 512));
    }
    void** array = new void*[files.size() + 1];
    int index = 0;
    for(auto& file : files) array[index++] = file;
    array[index] = 0;
    return array;
}
