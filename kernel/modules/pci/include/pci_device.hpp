#pragma once

#include "pci_header.h"
#include <fs/vnode.hpp>
#include <list.hpp>

struct PCI_Device {
    PCI_Header header;

    kernel::VNodePtr sysfs_device_root;
    std::List<kernel::VNodePtr> sysfs_nodes;

    PCI_Device(const PCI_Header& header)
        : header(header)
        , sysfs_device_root(nullptr) { }
};
