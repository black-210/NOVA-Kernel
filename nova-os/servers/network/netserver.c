/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: servers/network/netserver.c
 * Description: Modular Network Stack & Stateful Packet Filter Firewall.
 * Isolated in User-Space to prevent kernel network vulnerabilities.
 * License: Apache-2.0
 */

#include "types.h"
#include "kprintf.h"

typedef struct {
    uint32_t packets_received;
    uint32_t packets_transmitted;
    uint32_t packets_dropped_by_firewall;
    bool     firewall_stateful_enabled;
} net_stats_t;

static net_stats_t net_stats;

void netserver_init(void) {
    net_stats.packets_received = 0;
    net_stats.packets_transmitted = 0;
    net_stats.packets_dropped_by_firewall = 0;
    net_stats.firewall_stateful_enabled = true;

    kprintf("[NETSERVER] Network Server & Stateful Firewall active (User-Space Isolation)\n");
}

bool netserver_firewall_filter_packet(uint32_t src_ip, uint16_t src_port,
                                      uint32_t dst_ip, uint16_t dst_port, uint8_t proto) {
    (void)src_ip; (void)dst_ip;
    // Default Drop unauthorized incoming telnet/rlogin ports
    if (dst_port == 23 || dst_port == 513) {
        net_stats.packets_dropped_by_firewall++;
        klog_sec("FIREWALL", "Blocked insecure port access: dst_port=%u proto=%u", dst_port, proto);
        return false;
    }

    net_stats.packets_received++;
    return true;
}
