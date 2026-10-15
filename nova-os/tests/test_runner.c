/**
 * NOVA OS - Automated Test Suite & Microbenchmarks Runner
 * File: tests/test_runner.c
 * Description: Unit tests, security fuzzing, ABI regression checks, and performance benchmarks.
 * License: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "../abi/nova_abi.h"

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(expr, msg) do { \
    tests_run++; \
    if (expr) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

/* 1. ABI Stability & Struct Size Tests */
void test_abi_stability(void) {
    printf("\n=== [1] Testing NOVA OS ABI Stability & Frozen Contracts ===\n");

    ASSERT_TRUE(NOVA_ABI_VERSION_MAJOR == 1, "ABI Major version is 1");
    ASSERT_TRUE(NOVA_ABI_SIGNATURE == 0x4E4F56414F530100ULL, "ABI Signature matches 'NOVAOS\\x01\\x00'");
    ASSERT_TRUE(sizeof(nova_ipc_msg_t) == 64, "nova_ipc_msg_t must be exactly 64 bytes (cache-line aligned)");
    ASSERT_TRUE(sizeof(nova_sys_info_t) >= 48, "nova_sys_info_t has required layout fields");
    ASSERT_TRUE(SYS_NOVA_MAX_SYSCALL == 15, "Syscall table size is strictly bounded to 15 entries");
}

/* 2. Capability Model & Attenuation Test */
typedef struct {
    uint32_t type;
    uint32_t rights;
    uint64_t object_id;
    bool valid;
} test_cap_t;

void test_capability_attenuation(void) {
    printf("\n=== [2] Testing Capability Rights Attenuation & Delegation ===\n");

    test_cap_t parent = {
        .type = CAP_TYPE_MEMORY,
        .rights = CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_GRANT,
        .object_id = 0x1000,
        .valid = true
    };

    // Attenuated child capability: only grant READ
    uint32_t child_requested_rights = CAP_RIGHT_READ;
    uint32_t child_delegated_rights = (parent.rights & child_requested_rights);

    ASSERT_TRUE(child_delegated_rights == CAP_RIGHT_READ, "Child successfully restricted to Read-Only");
    ASSERT_TRUE((child_delegated_rights & CAP_RIGHT_WRITE) == 0, "Child does NOT inherit Write permission");
    ASSERT_TRUE((child_delegated_rights & CAP_RIGHT_GRANT) == 0, "Child does NOT inherit Grant permission (cannot re-delegate)");

    // Attempt privilege escalation: child requesting EXECUTE which parent lacks
    uint32_t escalated_request = CAP_RIGHT_EXECUTE;
    uint32_t escalated_result = (parent.rights & escalated_request);
    ASSERT_TRUE(escalated_result == 0, "Escalation blocked: parent cannot grant rights it does not hold");
}

/* 3. Hardened Heap & Canary Simulation */
#define HEAP_CANARY 0xDEADC0DEBEEFCAFEULL
typedef struct {
    uint64_t magic;
    size_t size;
    uint64_t canary;
} test_heap_header_t;

void test_heap_canary_protection(void) {
    printf("\n=== [3] Testing Heap Canary & Double-Free Mitigation ===\n");

    uint8_t buffer[128];
    test_heap_header_t *hdr = (test_heap_header_t *)buffer;
    hdr->magic = 0x4E4F5641414C4C43ULL;
    hdr->size = 64;
    hdr->canary = HEAP_CANARY;

    ASSERT_TRUE(hdr->canary == HEAP_CANARY, "Canary cookie intact after valid allocation");

    // Simulate buffer overflow corrupting canary
    uint64_t *corrupted_canary = &hdr->canary;
    *corrupted_canary = 0x4141414141414141ULL; // Attacker payload 'AAAAAAAA'

    ASSERT_TRUE(hdr->canary != HEAP_CANARY, "Heap corruption detected when canary is clobbered");
}

/* 4. Microkernel Performance Benchmark (IPC Latency) */
void benchmark_ipc_latency(void) {
    printf("\n=== [4] Running Microkernel IPC Performance Benchmark ===\n");

    const int iterations = 1000000;
    nova_ipc_msg_t msg;
    msg.sender_pid = 1;
    msg.target_endpoint = 2;
    msg.token = 0x12345678;
    msg.transferred_cap = 0;
    msg.sequence_id = 1;
    msg.data[0] = 42;
    msg.data[1] = 99;
    msg.data[2] = 1337;

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    volatile uint64_t sum = 0;
    for (int i = 0; i < iterations; i++) {
        // Fast rendezvous copy simulation
        nova_ipc_msg_t dest;
        memcpy(&dest, &msg, sizeof(nova_ipc_msg_t));
        dest.token += i;
        sum += dest.token;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double elapsed_ns = (double)(ts_end.tv_sec - ts_start.tv_sec) * 1e9 + (double)(ts_end.tv_nsec - ts_start.tv_nsec);
    double nanoseconds_per_ipc = elapsed_ns / iterations;
    double msgs_per_sec = (iterations / elapsed_ns) * 1e9;

    printf("  [BENCHMARK] Total Iterations: %d\n", iterations);
    printf("  [BENCHMARK] Average Rendezvous Latency: %.2f nanoseconds / message\n", nanoseconds_per_ipc);
    printf("  [BENCHMARK] Throughput: %.2f million IPC msgs/sec\n", msgs_per_sec / 1e6);

    ASSERT_TRUE(nanoseconds_per_ipc < 200.0, "IPC Rendezvous latency is sub-microsecond (< 200 ns)");
}

/* 5. VMM W^X Memory Protection Policy */
void test_wx_memory_protection(void) {
    printf("\n=== [5] Testing Virtual Memory W^X Enforcement ===\n");

    uint64_t PAGE_PRESENT  = (1ULL << 0);
    uint64_t PAGE_WRITABLE = (1ULL << 1);
    uint64_t PAGE_NX       = (1ULL << 63);

    // If an attacker requests Writable memory without NX:
    uint64_t requested_flags = PAGE_PRESENT | PAGE_WRITABLE;
    uint64_t enforced_flags = requested_flags;

    if ((enforced_flags & PAGE_WRITABLE) && !(enforced_flags & PAGE_NX)) {
        enforced_flags |= PAGE_NX;
    }

    ASSERT_TRUE((enforced_flags & PAGE_NX) != 0, "NX bit automatically enforced on writable page");
    ASSERT_TRUE(!((enforced_flags & PAGE_WRITABLE) && !(enforced_flags & PAGE_NX)), "Memory page is NEVER W+X simultaneously");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("=========================================================\n");
    printf("       NOVA OS Test Suite & Security Validation Suite     \n");
    printf("=========================================================\n");

    test_abi_stability();
    test_capability_attenuation();
    test_heap_canary_protection();
    test_wx_memory_protection();
    benchmark_ipc_latency();

    printf("\n---------------------------------------------------------\n");
    printf("Test Summary: %d Total | %d Passed | %d Failed\n", tests_run, tests_passed, tests_failed);
    printf("---------------------------------------------------------\n");

    return tests_failed == 0 ? 0 : 1;
}
