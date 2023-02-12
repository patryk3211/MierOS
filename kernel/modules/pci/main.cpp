#include "pci_header.h"
#include "pci_driver.h"
#include <defines.h>
#include <dmesg.h>
#include <list.hpp>
#include <modules/module_header.h>
#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>
#include <modules/module.hpp>
#include <modules/module_manager.hpp>
#include <fs/systemfs.hpp>
#include <fs/vfs.hpp>
#include <printf.h>
#include <event/uevent.hpp>

MODULE_HEADER static module_header header {
    .magic = MODULE_HEADER_MAGIC,
    .mod_name = "pci",
    .dependencies = 0
};

std::List<PCI_Header> pci_headers;

extern void detect_pci();

#define SYSFS_DEVICE_PREFIX "devices/pci/%02x:%02x.%01x/%s", header.bus, header.device, header.function
#define SYSFS_ADD_NODE(location, dataFormat, ...) { \
        auto nodeData = kernel::SystemFilesystem::instance()->add_node((location)); \
        if(nodeData) {\
            sprintf((*nodeData)->small_data, (dataFormat), #__VA_ARGS__); \
            header.sysfs_nodes.push_back((*nodeData)->node); \
        } \
    }

static std::List<kernel::VNodePtr> global_pci_nodes;

kernel::ValueOrError<size_t> uevent_write_callback(void*, kernel::FileStream*, const void* buffer, size_t length) {
    kernel::UEvent* uevent = kernel::parse_uevent(buffer, length);
    
    if(!strcmp(uevent->event_name, "attach")) {
        auto* deviceToAttach = uevent->get_arg("DEVICE");
        if(!deviceToAttach)
            return ENODEV;

        TRACE("(pci) Attach event called for device '%s'", deviceToAttach);

        char buffer[256];
        sprintf(buffer, "/sys/bus/pci/devices/%s/driver/module", deviceToAttach);
        auto res = kernel::VFS::instance()->get_file(nullptr, buffer, { .resolve_link = true, .follow_links = true });
        if(!res)
            return res.errno();

        PCI_Header* header = 0;
        for(auto& entry : pci_headers) {
            char buffer2[32];
            sprintf(buffer2, "%02x:%02x.%01x", entry.bus, entry.device, entry.function);
            if(!strcmp(deviceToAttach, buffer2)) {
                header = &entry;
                break;
            }
        }

        if(!header) {
            TRACE("(pci) Could not find device header");
            return 0;
        }

        auto& modName = (*res)->name();
        TRACE("(pci) Attaching to '%s' module", modName.c_str());

        auto modMajor = kernel::ModuleManager::get().find_module(modName);
        if(!modMajor)
            return ENOEXEC;

        auto* mod = kernel::ModuleManager::get().get_module(modMajor);
        PCI_Driver* driver = (PCI_Driver*)mod->get_symbol_ptr("pci_driver");
        driver->attach(header);
    }

    return length;
}

kernel::ValueOrError<kernel::VNodePtr> sysfs_mkdir(const char* path) {
    auto dirRes = kernel::SystemFilesystem::instance()->add_directory(path);
    if(dirRes) {
        global_pci_nodes.push_back(*dirRes);
        return *dirRes;
    } else {
        return dirRes.errno();
    }
}

extern "C" int init() {
    auto pciDevices = sysfs_mkdir("devices/pci");
    if(!pciDevices) return pciDevices.errno();
    auto pciBus = sysfs_mkdir("bus/pci");
    if(!pciBus) return pciBus.errno();
    auto pciBusDevices = sysfs_mkdir("bus/pci/devices");
    if(!pciBusDevices) return pciBusDevices.errno();
    auto pciBusDrivers = sysfs_mkdir("bus/pci/drivers");
    if(!pciBusDrivers) return pciBusDrivers.errno();

    auto ueventRes = kernel::SystemFilesystem::instance()->add_node("bus/pci/uevent");
    if(!ueventRes)
        return ueventRes.errno();
    auto ueventNode = *ueventRes;
    ueventNode->node->f_permissions = 0200;
    ueventNode->write_callback = &uevent_write_callback;

    dmesg("(pci) Detecting PCI devices...");
    detect_pci();

    dmesg("(pci) Found %d devices.", pci_headers.size());

    for(auto& header : pci_headers) {
        dmesg("(pci) Vendor=%04x DeviceID=%04x Class=%02x Subclass=%02x ProgIf=%02x Rev=%02x",
                header.vendor_id, header.device_id, header.classcode, header.subclass, header.prog_if, header.revision);

        char buffer[256];
        sprintf(buffer, SYSFS_DEVICE_PREFIX, "device");
        SYSFS_ADD_NODE(buffer, "0x%04x\n", header.device_id);

        sprintf(buffer, SYSFS_DEVICE_PREFIX, "vendor");
        SYSFS_ADD_NODE(buffer, "0x%04x\n", header.vendor_id);

        sprintf(buffer, SYSFS_DEVICE_PREFIX, "class");
        SYSFS_ADD_NODE(buffer, "0x%02x%02x%02x\n", header.classcode, header.subclass, header.prog_if);

        sprintf(buffer, SYSFS_DEVICE_PREFIX, "revision");
        SYSFS_ADD_NODE(buffer, "0x%02x\n", header.revision);

        char buffer2[32];
        sprintf(buffer, "../../../devices/pci/%02x:%02x.%01x", header.bus, header.device, header.function);
        sprintf(buffer2, "%02x:%02x.%01x", header.bus, header.device, header.function);
        
        auto linkRes = kernel::SystemFilesystem::instance()->symlink(*pciBusDevices, buffer2, buffer);
        if(linkRes)
            header.sysfs_nodes.push_back(*linkRes);
    } 

    return 0;
}

extern "C" int destroy() {
    pci_headers.clear();
    // TODO: [12.02.2023] A lot more has to happen here
    return 0;
}
