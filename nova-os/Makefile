# NOVA OS - Microkernel Build System
# Compiles x86_64 bare-metal kernel and userspace test runner

CC = gcc
LD = ld
AS = nasm

CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pie -fno-pic \
         -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra \
         -std=c11 -O2 -Ikernel/include -Iabi

LDFLAGS = -n -nostdlib -T linker.ld -m elf_x86_64

BUILD_DIR = build
BIN_DIR = bin

C_SRCS = kernel/core/main.c \
         kernel/core/string.c \
         kernel/core/kprintf.c \
         kernel/arch/x86_64/serial.c \
         kernel/arch/x86_64/gdt.c \
         kernel/arch/x86_64/idt.c \
         kernel/arch/x86_64/pic.c \
         kernel/arch/x86_64/pit.c \
         kernel/mm/pmm.c \
         kernel/mm/vmm.c \
         kernel/mm/heap.c \
         kernel/security/capabilities.c \
         kernel/security/canary.c \
         kernel/security/audit.c \
         kernel/sched/process.c \
         kernel/sched/scheduler.c \
         kernel/ipc/ipc.c \
         kernel/syscalls/syscall.c \
         drivers/pci/pci.c \
         servers/filesystem/vfs.c \
         servers/filesystem/novafs.c \
         servers/device_manager/devmgr.c \
         servers/network/netserver.c \
         servers/graphics/compositor.c

ASM_SRCS = boot/boot.asm \
           kernel/arch/x86_64/interrupts.asm \
           kernel/sched/context.asm

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS)) \
       $(patsubst %.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))

TARGET = $(BIN_DIR)/nova-kernel.elf
TEST_BIN = $(BIN_DIR)/test_runner

.PHONY: all kernel test test-abi test-security benchmark clean image run

all: kernel test

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/boot \
	          $(BUILD_DIR)/kernel/core \
	          $(BUILD_DIR)/kernel/arch/x86_64 \
	          $(BUILD_DIR)/kernel/mm \
	          $(BUILD_DIR)/kernel/security \
	          $(BUILD_DIR)/kernel/sched \
	          $(BUILD_DIR)/kernel/ipc \
	          $(BUILD_DIR)/kernel/syscalls \
	          $(BUILD_DIR)/drivers/pci \
	          $(BUILD_DIR)/servers/filesystem \
	          $(BUILD_DIR)/servers/device_manager \
	          $(BUILD_DIR)/servers/network \
	          $(BUILD_DIR)/servers/graphics \
	          $(BUILD_DIR)/tests \
	          $(BIN_DIR)

# Compile C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "  [CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble NASM files
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	@echo "  [AS]  $<"
	@$(AS) -f elf64 $< -o $@

# Link Kernel ELF
kernel: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	@echo "  [LD]  Linking $@"
	@$(LD) $(LDFLAGS) $(OBJS) -o $@
	@objcopy -O elf32-i386 $@ $(BIN_DIR)/nova-kernel-mb.elf
	@echo "  [OK]  NOVA Kernel ELF built successfully: $@"
	@echo "  [OK]  Multiboot ELF generated: $(BIN_DIR)/nova-kernel-mb.elf"

# Compile and Run Tests
test: $(TEST_BIN)
	@echo "\n>>> Running Automated NOVA OS Test Suite..."
	@$(TEST_BIN)

test-abi: $(TEST_BIN)
	@$(TEST_BIN)

test-security: $(TEST_BIN)
	@$(TEST_BIN)

benchmark: $(TEST_BIN)
	@$(TEST_BIN)

$(TEST_BIN): tests/test_runner.c | $(BUILD_DIR)
	@echo "  [CC]  Building Test Runner $<"
	@$(CC) -O3 -Wall -Wextra -Iabi tests/test_runner.c -o $@

image: kernel
	@echo "  [IMG] Generating bootable disk image..."
	@dd if=/dev/zero of=$(BIN_DIR)/nova-os.img bs=1M count=32 2>/dev/null
	@echo "  [IMG] nova-os.img ready for QEMU / Bare-metal UEFI/BIOS."

run: image
	@echo "  [QEMU] Launching NOVA OS in QEMU with serial console..."
	@qemu-system-x86_64 -kernel $(TARGET) -nographic -serial mon:stdio -m 512M

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "  [CLEAN] Build artifacts removed."
