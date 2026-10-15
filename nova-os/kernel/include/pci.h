/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/pci.h
 * Description: PCI/PCIe bus scanner and configuration space accessor.
 * License: Apache-2.0
 */

#ifndef NOVA_PCI_H
#define NOVA_PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

typedef struct {
    uint8_t  bus;
    uint8_t  slot;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint32_t bar0;
} pci_device_t;

void pci_init(void);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
void pci_scan_bus(void);

#endif /* NOVA_PCI_H */
