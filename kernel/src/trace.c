#include <defines.h>
#include <stdlib.h>
#include <trace.h>

struct record {
    u64_t address;
    u32_t line;
    char name[];
} PACKED;

NO_EXPORT struct record* line_map;

void set_line_map(void* map) {
    line_map = map;
}

struct file_line_pair addr_to_line(u64_t address) {
    size_t len;
    struct record* current_record = line_map;
    while((len = strlen(current_record->name)) != 0) {
        if(current_record->address > address) break;
        current_record = (struct record*)((u8_t*)current_record + len + 13);
    }
    struct file_line_pair pair;
    pair.line = current_record->line;
    pair.name = current_record->name;
    return pair;
}
