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
                    pager.unlock();
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
            memcpy(textRepresentation, (void*)(start_addr + i - readBytes), readBytes);
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
                "x address [count] - Print count amount of bytes starting from address of virtual memory\n"
                "b address value - Sets byte at address to a given value\n"
                "map physical_address [virtual_address] - Map the given physical address (In kernel's pager)\n"
                "unmap virtual_address - Unmap the given virtual address\n");
    } else if(!strcmp(command, "B")) {
        if(cmd == 0) {
            kprintf("You have to specify the address\n");
            return;
        }
        char* addr_str = cmd;
        cmd = next_arg(cmd);
        if(cmd == 0) {
            kprintf("You have to specify the value\n");
            return;
        }
        char* value_str = cmd;
        cmd = next_arg(cmd);
        
        u64_t addr = hex_to_int(addr_str);
        u8_t val = hex_to_int(value_str);

        auto& pager = kernel::Pager::active();
        if(!pager.try_lock()) {
            kprintf("Failed to lock the pager!\n");
            return;
        }

        if(!pager.getFlags(addr).present) {
            kprintf("Address not mapped!\n");
            pager.unlock();
            return;
        }

        *((u8_t*)addr) = val;

        pager.unlock();
    } else if(!strcmp(command, "MAP")) {
        if(cmd == 0) {
            kprintf("You have to specify the physical address\n");
            return;
        }
        char* phys_str = cmd;
        cmd = next_arg(cmd);

        physaddr_t phys = hex_to_int(phys_str);
        virtaddr_t virt = 0xFFF;

        if(cmd != 0) {
            char* virt_str = cmd;
            cmd = next_arg(cmd);

            virt = hex_to_int(virt_str) & ~0xFFF;
        }

        auto& pager = kernel::Pager::kernel();
        if(!pager.try_lock()) {
            kprintf("Failed to lock the pager!\n");
            return;
        }

        if(virt == 0xFFF)
            virt = pager.kmap(phys, 1, { 1, 1, 0, 0, 1, 0 });
        else
            pager.map(phys, virt, 1, { 1, 1, 0, 0, 1, 0 });
        
        kprintf("Address 0x%x16 mapped at 0x%x16\n", phys, virt);

        pager.unlock();
    } else if(!strcmp(command, "UNMAP")) {
        if(cmd == 0) {
            kprintf("You have to specify the virtual address\n");
            return;
        }
        char* virt_str = cmd;
        cmd = next_arg(cmd);

        virtaddr_t virt = hex_to_int(virt_str);

        auto& pager = kernel::Pager::kernel();
        if(!pager.try_lock()) {
            kprintf("Failed to lock the pager!\n");
            return;
        }

        pager.unmap(virt, 1);

        pager.unlock();
    } else if(!strcmp(command, "REBOOT")) {
        // Cause a triple-fault
        
        struct IDTR {
            u64_t offset;
            u16_t size;
        } idtr;
        idtr.offset = 0;
        idtr.size = 0;
        asm volatile("lidt (%0)" : : "a"(&idtr));

        *((char*)0) = 'F';

        kprintf("HOW DID IT FAIL?\n");
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
        if(serial_buffer_index > 0) {
            --serial_buffer_index;
            write_serial('\b');
            write_serial(' ');
            write_serial('\b');
        }
    } else {
        serial_buffer[serial_buffer_index++] = c;
        write_serial(c);
    }
}

extern "C" void init_debug() {
    register_handler(0x24, &serial_handle);
    dmesg("Debug console ready!");
}
