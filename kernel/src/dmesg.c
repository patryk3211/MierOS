#include <dmesg.h>
#include <types.h>
#include <defines.h>
#include <arch/x86_64/ports.h>
#include <arch/time.h>

#define IO_PORT 0x3F8

TEXT_FREE_AFTER_INIT void init_serial() {
    // Disable Interrupts
    outb(IO_PORT+1, 0b00000000);
    // Set DLAB
    outb(IO_PORT+3, 0b10000000);

    // Set Divisor (to 3)
    outb(IO_PORT+0, 0b00000011);
    outb(IO_PORT+1, 0b00000000);

    // Clear DLAB and set mode to 8 bits, no parity, 1 stop bit (8N1)
    outb(IO_PORT+3, 0b00000011);

    // Enable DTR and RTS
    outb(IO_PORT+4, 0b00000011);
}

void write_serial(char c) {
    while(!(inb(IO_PORT+5) & 0x20));
    outb(IO_PORT, c);
}

void dmesg(const char* msg) {
    while(*msg != 0) write_serial(*msg++);
}

void dmesgl(const char* msg, size_t length) {
    for(size_t i = 0; i < length; ++i) write_serial(msg[i]);
}

void kprintf(const char* format, ...) {
    va_list list;
    va_start(list, format);
    va_kprintf(format, list);
    va_end(list);
}

void panic(const char* msg) {
    kprintf("\033[1;31mKERNEL PANIC! %s\n", msg);
    while(1) asm volatile("cli; hlt");
}

const char lookup_lower[] = "0123456789abcdef";
const char lookup_upper[] = "0123456789ABCDEF";

void va_kprintf(const char* format, va_list args) {
    int dwOffset = -1;
    int i;
    for(i = 0; format[i] != 0; ++i) {
        if(format[i] == '%') {
            if(dwOffset != -1) {
                dmesgl(format+dwOffset, i-dwOffset);
                dwOffset = -1;
            }
            switch(format[++i]) {
                case 's': {
                    char* str = va_arg(args, char*);
                    dmesg(str);
                    break;
                } case 'c': {
                    char val = va_arg(args, int);
                    dmesgl(&val, 1);
                    break;
                } case 'i':
                  case 'd': {
                    int n = va_arg(args, int);
                    int neg = n < 0;
                    char buffer[80];
                    int index = 0;
                    do {
                        int digit = n % 10;
                        n /= 10;
                        buffer[index++] = '0' + digit;
                    } while(n > 0);
                    char fin[index+neg];
                    for(int i = 0; i < index; i++) fin[i+neg] = buffer[index-i-1];
                    if(neg) fin[0] = '-';
                    dmesgl(fin, index+neg);
                    break;
                } case 'x': 
                  case 'X': {
                    const char* lookup;
                    if(format[i++] == 'x') lookup = lookup_lower;
                    else lookup = lookup_upper;
                    
                    int count = 0;
                    while(format[i] >= '0' && format[i] <= '9') count = count*10+((format[i++])-'0');
                    
                    if(count > 16) count = 16; // Max size

                    u64_t num_to_conv = va_arg(args, u64_t);
                    if(count == 0) {
                        int started = 0;
                        for(int j = 15; j >= 0; --j) {
                            char digit = (num_to_conv >> (j*4)) & 0xF;
                            if(digit == 0 && !started) continue;
                            dmesgl(lookup+digit, 1);
                            started = 1;
                        }
                    } else {
                        for(int j = count-1; j >= 0; --j) {
                            char digit = (num_to_conv >> (j*4)) & 0xF;
                            dmesgl(lookup+digit, 1);
                        }
                    }
                    --i;
                    break;
                } case 'T': {
                    // Timestamp
                    time_t uptime = get_uptime();
                    {
                        time_t seconds = uptime / 1000;
                        char buffer[80];
                        int index = 0;
                        do {
                            int digit = seconds % 10;
                            seconds /= 10;
                            buffer[index++] = '0' + digit;
                        } while(seconds > 0);
                        char fin[index];
                        for(int i = 0; i < index; i++) fin[i] = buffer[index-i-1];
                        dmesgl(fin, index);
                    } 
                    dmesg(".");
                    {
                        time_t millis = uptime % 1000;
                        char buffer[3];
                        buffer[0] = '0' + (millis / 100);
                        buffer[1] = '0' + (millis / 10 % 10);
                        buffer[2] = '0' + (millis % 10);
                        dmesgl(buffer, 3);
                    }
                    break;
                }
            }
        } else if(dwOffset == -1) dwOffset = i;
    }
    if(dwOffset != -1) dmesgl(format+dwOffset, i-dwOffset);
}
