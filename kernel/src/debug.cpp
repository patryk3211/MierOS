#include <debug.h>
#include <arch/interrupts.h>
#include <arch/x86_64/ports.h>
#include <memory/virtual.hpp>

#define IO_PORT 0x3F8

char serial_buffer[80];
int serial_buffer_index = 0;

u64_t hex_to_int(char* hex) {
    u64_t result = 0;
    while(*hex != 0) {
        result <<= 4;
        if(*hex >= '0' && *hex <= '9') result |= *hex - '0';
        else if(*hex >= 'A' && *hex <= 'F') result |= *hex - 'A' + 10;
        else break;
        ++hex;
    }
    return result;
}

// Advanced cmd to next arg
char* next_arg(char* cmd) {
    while(*cmd++ != ' ') if(*cmd == 0) return 0;
    *(cmd-1) = 0;
    while(*cmd == ' ') {
        if(*cmd == 0) return 0;
        ++cmd;
    }
    return cmd;
}

void serial_command(char* cmd) {
    char* command = cmd;
    cmd = next_arg(cmd);

    if(!strcmp(command, "X")) {
        if(cmd == 0) {
            kprintf("You have to specify the start address\n");
            return;
        }
        char* addr_str = cmd;
        cmd = next_arg(cmd);

        size_t count = 1;
        if(cmd != 0) {
            char* count_str = cmd;
            cmd = next_arg(cmd);

            count = hex_to_int(count_str);
        }

        u64_t start_addr = (u64_t)hex_to_int(addr_str);

        auto& pager = kernel::Pager::active();
        if(!pager.try_lock()) {
            kprintf("Failed to lock the pager!\n");
            return;
        }

        u64_t prevPage = -1;
        u64_t i;
        for(i = 0; i < count; ++i) {
            u64_t addr = start_addr + i;

            if(i % 16 == 0) {
                // Print current address
                kprintf("%x16: ", addr);
            }

            if(prevPage != (addr & ~0xFFF)) {
                // Check if page is mapped
                auto flags = pager.getFlags(addr);
                prevPage = addr & ~0xFFF;
                if(!flags.present) {
                    kprintf("Address is not readable\n");
                    return;
                }
            }

            if(i % 8 == 0 && i % 16 != 0) kprintf(" ");

            kprintf("%x2 ", *((u8_t*)addr));

            if(i % 16 == 15) {
                char textRepresentation[17];
                textRepresentation[16] = 0;
                memcpy(textRepresentation, (void*)(addr - 15), 16);
                for(int j = 0; j < 16; ++j) {
                    char c = textRepresentation[j];
                    if(c < 0x20 || c >= 0x7F) c = '.';
                    textRepresentation[j] = c;
                }

                kprintf("|%s|\n", textRepresentation);
            }
        }

        if(i % 16 != 0) {
            // Add the last text representation
            int readBytes = i % 16;
            int missingSpaces = 16 - readBytes;
            for(int j = 0; j < missingSpaces; ++j) kprintf("   ");
            if(missingSpaces >= 8) kprintf(" ");
            
            char textRepresentation[17];
            memset(textRepresentation, 0, 17);
            memcpy(textRepresentation, (void*)(start_addr + i - 1 - readBytes), readBytes);
            for(int j = 0; j < readBytes; ++j) {
                char c = textRepresentation[j];
                if(c < 0x20 || c >= 0x7F) c = '.';
                textRepresentation[j] = c;
            }

            kprintf("|%s|\n", textRepresentation);
        }

        pager.unlock();
    } else if(!strcmp(command, "HELP")) {
        kprintf("Serial Debug Terminal\n"
                "All numbers passed as arguments are hexadecimal\n"
                "Commands:\n"
                "x [address] [count] - Print [count] amount of bytes starting from [address] of virtual memory\n");
    }
}

extern "C" void write_serial(char c);

void serial_handle() {
    if((inb(IO_PORT + 5) & 1) == 0) return;

    char c = inb(IO_PORT);
    if(c == 0x0D) {
        serial_buffer[serial_buffer_index] = 0;
        strupper(serial_buffer);
        write_serial('\n');

        serial_command(serial_buffer);
        serial_buffer_index = 0;

        kprintf("> ");
    } else if(c == 0x7F) {
        --serial_buffer_index;
        write_serial('\b');
        write_serial(' ');
        write_serial('\b');
    } else {
        serial_buffer[serial_buffer_index++] = c;
        write_serial(c);
    }
}

extern "C" void init_debug() {
    register_handler(0x24, &serial_handle);
    dmesg("Debug console ready!");
}
