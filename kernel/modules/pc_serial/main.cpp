#include <fs/vnode.hpp>
#include "registers.hpp"
#include <defines.h>
#include <dmesg.h>
#include <modules/module_header.h>
#include <fs/devicefs.hpp>
#include <arch/x86_64/ports.h>
#include <trace.h>
#include <arch/interrupts.h>
#include "port.hpp"

using namespace kernel;

MODULE_HEADER static module_header header {
    .magic = MODULE_HEADER_MAGIC,
    .mod_name = "pc_serial",
    .dependencies = 0
};

static const u16_t serial_ports[] = {
    0x3F8, // COM1
    0x2F8, // COM2
    //0x3E8, // COM3
    //0x2E8  // COM4
    // I will not add the rest of the com ports for now
};

static u32_t working_ports;
//static VNodePtr device_nodes[sizeof(serial_ports) / sizeof(u16_t)];
Port* device_ports[sizeof(serial_ports) / sizeof(u16_t)];

extern u16_t major;

bool configure_serial(u16_t port) {
    // Check the scrach register
    outb(port + SCRATCH_REGISTER, 0xA5);
    /// This might be a 8250 device
    if(inb(port + SCRATCH_REGISTER) != 0xA5) {
        TRACE("(Serial) Port 0x%x3 failed scratch register test\n", port);
        return false;
    }

    // Disable all interrupts
    outb(port + INT_CTRL_REGISTER, 0);

    // Check the loopback connection
    outb(port + MODEM_CTRL_REGISTER, 0b00010000);

    outb(port + DATA_REGISTER, 0x55);
    if(inb(port + DATA_REGISTER) != 0x55) {
        TRACE("(Serial) Port 0x%x3 failed loopback test 1\n", port);
        outb(port + MODEM_CTRL_REGISTER, 0);
        return false;
    }

    outb(port + DATA_REGISTER, 0xaa);
    if(inb(port + DATA_REGISTER) != 0xaa) {
        TRACE("(Serial) Port 0x%x3 failed loopback test 2\n", port);
        outb(port + MODEM_CTRL_REGISTER, 0);
        return false;
    }

    // Tests were succesfull so we can assume that the port works
    outb(port + MODEM_CTRL_REGISTER, 0);
    
    // Set DLAB = 1
    outb(port + LINE_CTRL_REGISTER, inb(port + LINE_CTRL_REGISTER) | 0x80);
    // Default baudrate = 9600
    outb(port + DIVISOR_LSB, 0x0C);
    outb(port + DIVISOR_MSB, 0x00);

    // Enable 8N1 mode on line control
    outb(port + LINE_CTRL_REGISTER, 0b00000011);

    // Enable DTS, RTS and OUT2
    outb(port + MODEM_CTRL_REGISTER, 0b00001011);

    // Enable interrupts (Transmitter Empty and Data Available)
    outb(port + INT_CTRL_REGISTER, 0x03);

    TRACE("(Serial) Port 0x%x3 configured succesfully\n", port);
    return true;
}

void serial_interrupt() {
    size_t portIdx = 0;
    for(auto ports = working_ports; ports; ports >>= 1, ++portIdx) {
        if(!(ports & 1))
            continue;
        
        u8_t intStat = inb(INT_STATUS_REGISTER);
        if(!(intStat & 1))
            continue;

        u16_t port = device_ports[portIdx]->ioPort;

        u8_t intReason = (intStat >> 1) & 0x7;
        if(intReason == 0x02) {
            // Receiver Data Available
            u8_t lineStatus;
            while((lineStatus = inb(port + LINE_STATUS_REGISTER)) & 0x01) {
                u8_t dataByte = inb(port + DATA_REGISTER);
                u8_t statusByte = 0;
                statusByte |= (lineStatus & 0x0C) ? 0x01 : 0x00;
                statusByte |= (lineStatus & 0x10) ? 0x02 : 0x00;
                device_ports[portIdx]->termiosHelper.character_received(dataByte | ((u16_t)statusByte << 8));
            }
        } else if(intReason == 0x01) {
            // Transmitter Empty
            // Write data until transmit holding buffer is full
            while(inb(port + LINE_STATUS_REGISTER) & 0x20) {
                u8_t dataByte;
                bool success = device_ports[portIdx]->outputBuffer.read(&dataByte, false);
                if(!success)
                    break;

                outb(port + DATA_REGISTER, dataByte);
            }
        }
    }
}

bool termios_write_callback(void* arg, u8_t c) {
    Port* port = (Port*)arg;
    if(inb(port->ioPort + LINE_STATUS_REGISTER) & 0x20) {
        // Transmit holding register is empty, we need to
        // send data so that an interrupt chain starts.
        outb(port->ioPort + DATA_REGISTER, c);
        return true;
    } else {
        return port->outputBuffer.write(c, true);
    }
}

extern "C" int init() {
    dmesg("(Serial) Initializing serial tty interfaces");

    for(size_t i = 0; i < sizeof(serial_ports) / sizeof(u16_t); ++i) {
        if(configure_serial(serial_ports[i])) {
            char name[] = "ttyS?";
            name[4] = '0' + i;

            // Configure a devfs node
            DeviceFilesystem::instance()->add_dev(name, major, i);

            device_ports[i] = new Port(serial_ports[i], 256, &termios_write_callback);

            working_ports |= 1 << i;
        }
    }

    register_handler(0x23, &serial_interrupt);
    register_handler(0x24, &serial_interrupt);

    return 0;
}

extern "C" int destroy() {
    unregister_handler(0x23, &serial_interrupt);
    unregister_handler(0x24, &serial_interrupt);

    return 0;
}

