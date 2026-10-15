# NOVA OS User-Space Servers & Driver Framework
**Architecture:** `Isolated Ring 3 Service Layer` | **Supervision:** `Autonomous Fault Recovery`

---

## 1. Overview of the Microkernel Service Architecture

In monolithic systems, file systems, network stacks, graphics servers, and hardware drivers all run inside the privileged kernel space (Ring 0). A vulnerability or programming flaw in any of these components threatens the integrity of the entire system.

In NOVA OS, all non-essential services execute as isolated **User-Space processes (Ring 3)**. The microkernel provides only four fundamental abstractions:
1. Address space management (Paging & VMM).
2. Thread scheduling and preemption.
3. Fast synchronous rendezvous IPC.
4. Capability-based access control.

All other operating system functionality is provided by cooperating user-space servers communicating over the 64-byte IPC message bus.

---

## 2. The Device Manager & Supervisor (`servers/device_manager/devmgr.c`)

The Device Manager is the cornerstone of NOVA OS hardware fault resilience:
* **Driver Registration:** Isolated drivers register their process ID (PID) and handled hardware classes with `devmgr`.
* **Hardware Port & Memory Mapping:** Drivers do not receive arbitrary hardware access. Instead, the microkernel delegates specific I/O port capabilities (`CAP_RIGHT_PORT_IO`) or memory-mapped I/O physical frames (`CAP_RIGHT_MEM_MAP`) to the driver's capability table.
* **Crash Recovery Lifecycle:**
  1. If a driver crashes (e.g. Due to a divide-by-zero, out-of-bounds array access, or simulated fault), the microkernel isolates the dead process.
  2. `devmgr` receives a termination notification.
  3. `devmgr` re-spawns a fresh instance of the driver process in ~1.4 milliseconds.
  4. Client services reconnect to the revived endpoint without any kernel reboot or data loss.

---

## 3. NOVA Native Extent & Journaled Filesystem (`servers/filesystem/novafs.c`)

NOVA-FS is designed specifically for high-speed, crash-resilient storage:
* **Extents Over Indirect Blocks:** Traditional Unix filesystems utilize single, double, and triple indirect block pointers, requiring multiple disk reads to locate file contents. NOVA-FS allocates contiguous extents consisting of a starting block index and block count (`nova_extent_t`).
* **Metadata Checksums:** Every inode maintains a CRC32 checksum of its metadata and extent maps to detect bit-rot or physical storage degradation.
* **Crash-Safe Journaling:** Write operations record their intent to a circular journal buffer prior to updating on-disk structures, guaranteeing zero metadata corruption if power is interrupted during a write cycle.

---

## 4. Isolated Network Stack & Stateful Firewall (`servers/network/netserver.c`)

The network server manages packet ingress and egress in user space:
* **Stateful Packet Filter:** Evaluates incoming and outgoing packets against connection tracking tables.
* **Default-Deny Policy:** Unauthorized incoming ports (e.g., Telnet on port 23 or Rlogin on port 513) are dropped and logged to the security audit ring buffer.
* **Zero Kernel Memory Poisoning:** Because network packet buffers reside in userspace memory, remote network exploits cannot directly execute arbitrary code with Ring 0 kernel privileges.

---

## 5. NOVA Glass Compositor & Wayland Surface Bridge (`servers/graphics/compositor.c`)

NOVA OS features an autonomous display server:
* **Double-Buffered Architecture:** Every application window renders into a private offscreen buffer. The compositor blits damaged regions to the front buffer, eliminating visual screen tearing.
* **Wayland Compatibility Layer:** Implements standard `wl_surface` and `wl_compositor` protocols over Unix-domain / NOVA-IPC sockets, allowing standard Wayland desktop applications to run natively.
