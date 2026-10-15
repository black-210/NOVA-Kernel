/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: drivers/pci/pci.c
 * Description: Hardware Discovery & PCI Configuration Space Enumerator.
 * License: Apache-2.0
 */

#include "pci.h"
#include "cpu.h"
#include "kprintf.h"

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((1U << 31) | ((uint32_t)bus << 16) |
                                  ((uint32_t)slot << 11) | ((uint32_t)func << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (uint32_t)((1U << 31) | ((uint32_t)bus << 16) |
                                  ((uint32_t)slot << 11) | ((uint32_t)func << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, val);
}

static const char *pci_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller (VGA/3D)";
        case 0x04: return "Multimedia Controller (Audio)";
        case 0x06: return "Bridge Device (Host/PCI)";
        case 0x0C: return "Serial Bus Controller (USB)";
        default:   return "Other/Generic Peripheral";
    }
}

void pci_scan_bus(void) {
    uint32_t devices_found = 0;
    kprintf("[PCI] Starting PCI Hardware Enumeration...\n");

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t dword0 = pci_read_dword((uint8_t)bus, slot, 0, 0);
            uint16_t vendor_id = (uint16_t)(dword0 & 0xFFFF);
            uint16_t device_id = (uint16_t)(dword0 >> 16);

            if (vendor_id == 0xFFFF || vendor_id == 0x0000) {
                continue; // No device present
            }

            uint32_t dword8 = pci_read_dword((uint8_t)bus, slot, 0, 8);
            uint8_t class_code = (uint8_t)(dword8 >> 24);
            uint8_t subclass = (uint8_t)((dword8 >> 16) & 0xFF);
            uint32_t bar0 = pci_read_dword((uint8_t)bus, slot, 0, 0x10);

            devices_found++;
            kprintf("  [PCI %02x:%02x.0] Vendor: 0x%04x Dev: 0x%04x | Class 0x%02x (sub 0x%02x): %s | BAR0: 0x%x\n",
                    bus, slot, vendor_id, device_id, class_code, subclass, pci_class_name(class_code), bar0);
        }
    }
    kprintf("[PCI] Enumeration complete: %u active hardware devices discovered.\n", devices_found);
}

void pci_init(void) {
    pci_scan_bus();
}
