#include "pci_header.h"
#include "pci_driver.h"
#include "pci_device.hpp"
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

std::List<PCI_Device> pci_devices;

extern void detect_pci();

static kernel::VNodePtr sysfs_deviceRoot = nullptr;
static kernel::VNodePtr sysfs_busRoot = nullptr;
static kernel::VNodePtr sysfs_busDriversRoot = nullptr;
static kernel::VNodePtr sysfs_busDevicesRoot = nullptr;

#define SYSFS_ADD_NODE(root, location, dataFormat, ...) { \
        auto nodeData = kernel::SystemFilesystem::instance()->add_node((root), (location)); \
        if(nodeData) {\
            sprintf((*nodeData)->small_data, (dataFormat), #__VA_ARGS__); \
            device.sysfs_nodes.push_back((*nodeData)->node); \
            (*nodeData)->node->f_permissions = 0444; \
        } else { \
            return nodeData.errno(); \
        } \
    }

kernel::ValueOrError<kernel::VNodePtr> sysfs_mkdir(kernel::VNodePtr root, const char* path) {
    auto dirRes = kernel::SystemFilesystem::instance()->add_directory(root, path);
    if(dirRes)
        return *dirRes;
    if(dirRes.errno() == EEXIST) {
        auto getRes = kernel::SystemFilesystem::instance()->get_file(nullptr, path, { .resolve_link = true, .follow_links = true });
        if(!getRes)
            return getRes.errno();
        return *getRes;
    }
    return dirRes.errno();
}

#define SYSFS_NODE(root, location) \
    if(auto nodeRes = kernel::SystemFilesystem::instance()->add_node((root), (location)); \
       nodeRes)

kernel::ValueOrError<size_t> uevent_dev_attach_wcb(void* arg, kernel::FileStream*, const void* buffer, size_t length) {
    kernel::UEvent* uevent = kernel::parse_uevent(buffer, length);
    PCI_Device* pciDevice = static_cast<PCI_Device*>(arg);
    PCI_Header* pciHeader = &pciDevice->header;

    // Event name is the module to attach to
    TRACE("(pci) Attach called for device %02x:%02x.%01x to module '%s'",
            pciHeader->bus, pciHeader->device, pciHeader->function, uevent->event_name);

    auto modMajor = kernel::ModuleManager::get().find_module(uevent->event_name);
    if(!modMajor)
        return ENOEXEC;

    auto* mod = kernel::ModuleManager::get().get_module(*modMajor);
    PCI_Driver* driver = static_cast<PCI_Driver*>(mod->get_symbol_ptr("pci_driver"));
    int ret = driver->attach(pciHeader);
    if(ret)
        return (err_t)ret;

    // Create new sysfs enteries related to the newly attached module
    auto moduleDirRes = sysfs_mkdir(sysfs_busDriversRoot, uevent->event_name);
    if(moduleDirRes) {
        auto moduleDir = *moduleDirRes;
        
        char buffer[128];
        sprintf(buffer, "../../devices/%02x:%02x.%01x", pciHeader->bus, pciHeader->device, pciHeader->function);
        auto deviceLink = kernel::SystemFilesystem::instance()->symlink(moduleDir, uevent->event_name, buffer);
        if(deviceLink)
            pciDevice->sysfs_nodes.push_back(*deviceLink);
    }

    return length;
}

extern "C" int init() {
    auto pciDevices = sysfs_mkdir(nullptr, "devices/pci");
    if(!pciDevices) return pciDevices.errno();
    sysfs_deviceRoot = *pciDevices;

    auto pciBus = sysfs_mkdir(nullptr, "bus/pci");
    if(!pciBus) return pciBus.errno();
    sysfs_busRoot = *pciBus;

    auto pciBusDevices = sysfs_mkdir(nullptr, "bus/pci/devices");
    if(!pciBusDevices) return pciBusDevices.errno();
    sysfs_busDevicesRoot = *pciBusDevices;

    auto pciBusDrivers = sysfs_mkdir(nullptr, "bus/pci/drivers");
    if(!pciBusDrivers) return pciBusDrivers.errno();
    sysfs_busDriversRoot = *pciBusDrivers;

    dmesg("(pci) Detecting PCI devices...");
    detect_pci();

    dmesg("(pci) Found %d devices.", pci_devices.size());

    for(auto& device : pci_devices) {
        auto& header = device.header;

        dmesg("(pci) Vendor=%04x DeviceID=%04x Class=%02x Subclass=%02x ProgIf=%02x Rev=%02x",
                header.vendor_id, header.device_id, header.classcode, header.subclass, header.prog_if, header.revision);

        char buffer[256];
        sprintf(buffer, "%02x:%02x.%01x", header.bus, header.device, header.function);
        auto deviceDirRes = kernel::SystemFilesystem::instance()->add_directory(sysfs_deviceRoot, buffer);
        if(!deviceDirRes)
            return deviceDirRes.errno();

        device.sysfs_device_root = *deviceDirRes;
        SYSFS_ADD_NODE(*deviceDirRes, "device", "0x%04x\n", header.device_id);
        SYSFS_ADD_NODE(*deviceDirRes, "vendor", "0x%04x\n", header.vendor_id);
        SYSFS_ADD_NODE(*deviceDirRes, "class", "0x%02x%02x%02x\n", header.classcode, header.subclass, header.prog_if);
        SYSFS_ADD_NODE(*deviceDirRes, "revision", "0x%02x\n", header.revision);

        SYSFS_NODE(*deviceDirRes, "attach") {
            auto nodeData = *nodeRes;
            nodeData->node->f_permissions = 0200;

            nodeData->callback_arg = &device;
            nodeData->write_callback = &uevent_dev_attach_wcb;

            device.sysfs_nodes.push_back(nodeData->node);
        }

        SYSFS_NODE(*deviceDirRes, "uevent") {
            auto nodeData = *nodeRes;
            nodeData->node->f_permissions = 0444;

            std::String<> ueventStaticData;
            ueventStaticData += "BUS_TYPE=pci\n";
            sprintf(buffer, "PCI_CLASS=%02x%02x%02x\n"
                            "PCI_ID=%04x:%04x\n"
                            "PCI_SUBSYS_ID=%04x:%04x\n",
                            header.classcode, header.subclass, header.prog_if,
                            header.vendor_id, header.device_id,
                            header.subsys_vendor, header.subsys_device);
            ueventStaticData += buffer;
            sprintf(buffer, "MODALIAS=pci:v%04xd%04xsv%04xsd%04xbc%02xsc%02xi%02x\n",
                            header.vendor_id, header.device_id, header.subsys_vendor, header.subsys_device,
                            header.classcode, header.subclass, header.prog_if);
            ueventStaticData += buffer;

            char* ueventData = new char[ueventStaticData.length() + 1];
            memcpy(ueventData, ueventStaticData.c_str(), ueventStaticData.length());
            ueventData[ueventStaticData.length()] = 0;
            nodeData->static_data = ueventData;
            nodeData->node->f_size = ueventStaticData.length();

            device.sysfs_nodes.push_back(nodeData->node);
        }

        char buffer2[32];
        sprintf(buffer, "../../../devices/pci/%02x:%02x.%01x", header.bus, header.device, header.function);
        sprintf(buffer2, "%02x:%02x.%01x", header.bus, header.device, header.function);
        
        auto linkRes = kernel::SystemFilesystem::instance()->symlink(sysfs_busDevicesRoot, buffer2, buffer);
        if(linkRes)
            device.sysfs_nodes.push_back(*linkRes);
    } 

    return 0;
}

extern "C" int destroy() {
    pci_devices.clear();
    // TODO: [12.02.2023] A lot more has to happen here
    return 0;
}
