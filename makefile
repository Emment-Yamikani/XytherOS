# Target and Output Configuration
TARGET      := boot/xyther.elf
ISO_DIR     := iso
ISO_IMAGE   := xytheros.iso
KERNEL_DIR  := kernel
KERNEL_LINKER_SCRIPT:= kernel.ld

KERNEL_FLAGS:= -DDEBUG_BUILD -D__x86_64__ #-DUSE_SPINLOCK_FUNCTIONS

# Toolchain Configuration
CC          := x86_64-elf-gcc
AS          := nasm
LD          := x86_64-elf-ld
GRUBMKRESCUE:= grub-mkrescue

CINCLUDE	:= $(KERNEL_DIR)/include

# Compiler and Linker Flags
CFLAGS 		:= -ffreestanding -O2 -g -Wall -Wextra -Werror $(KERNEL_FLAGS) -fno-stack-protector \
			   -fno-exceptions -fno-pic -mno-red-zone -mno-mmx -msse -m64 -mcmodel=large -I$(CINCLUDE) -nostdlib \
			   -nostartfiles -nodefaultlibs -std=gnu99 -ffunction-sections -fdata-sections

LDFLAGS 	:= -T $(KERNEL_DIR)/$(KERNEL_LINKER_SCRIPT) --gc-sections -nostdlib -static -m elf_x86_64 \
        	   -z max-page-size=0x1000

ASFLAGS     := -f elf64

# Source and Object Files
C_SRCS      := $(shell find $(KERNEL_DIR) -name '*.c')
ASM_SRCS    := $(shell find $(KERNEL_DIR) -name '*.s')
NASM_SRCS   := $(shell find $(KERNEL_DIR) -name '*.asm')

OBJS        := $(C_SRCS:.c=.c.o) $(ASM_SRCS:.s=.s.o) $(NASM_SRCS:.asm=.o)

# Phony Targets
.PHONY: all clean iso run debug

# Default Target
all: $(ISO_DIR)/$(TARGET)

# Link the Kernel
$(ISO_DIR)/$(TARGET): $(OBJS)
	@echo "Linking kernel..."
	@mkdir -p $(ISO_DIR)/boot
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "Kernel linked: $@"

# Compile C Sources
%.c.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly Sources (GAS Syntax)
%.s.o: %.s
	@echo "Assembling $<..."
	$(CC) -c $< -o $@

# Compile NASM Assembly Sources
%.o: %.asm
	@echo "Assembling $<..."
	$(AS) $(ASFLAGS) $< -o $@

# Clean Build Artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJS) $(ISO_DIR)/$(TARGET) $(ISO_IMAGE)
	rm -rf $(ISO_DIR)/boot
	rm -rf *.log xyther.asm
	clear

# Build ISO Image
iso: all
	@echo "Creating ISO image..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "xytherOS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '	multiboot --quirk-modules-after-kernel /$(TARGET) 0x1000000' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '	boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUBMKRESCUE) -o $(ISO_IMAGE) $(ISO_DIR)
	@echo "ISO image created: $(ISO_IMAGE)"

CPU_COUNT 	:= 1
RAM_SIZE	:= 2048M
QUEUE_FLAGS := -no-reboot -no-shutdown -parallel none

# Run in QEMU
run: iso
	@echo "Starting QEMU..."
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -monitor stdio \
    $(QUEUE_FLAGS) -smp $(CPU_COUNT) -m size=$(RAM_SIZE) -vga std \
    -chardev file,id=char0,path=serial.log -serial chardev:char0

run_serial: iso
	@echo "Starting QEMU..."
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) \
    $(QUEUE_FLAGS) -smp $(CPU_COUNT) -m size=$(RAM_SIZE) -vga std \
    -chardev stdio,id=char0,logfile=serial.log -serial chardev:char0

# Debug in QEMU with GDB
debug: iso
	@echo "Starting QEMU in debug mode..."
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -monitor stdio -s -S -d in_asm \
	-D qemu_gdb.log $(QUEUE_FLAGS) -smp $(CPU_COUNT) -m size=$(RAM_SIZE) -vga std \
    -chardev file,id=char0,path=serial.log -serial chardev:char0

debug_log: iso
	@echo "Starting QEMU in debug mode..."
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -monitor stdio -d in_asm \
	-D qemu.log $(QUEUE_FLAGS) -smp $(CPU_COUNT) -m size=$(RAM_SIZE) -vga std \
    -chardev file,id=char0,path=serial.log -serial chardev:char0

dump: $(ISO_DIR)/$(TARGET)
	objdump -d $< -M intel | less > xyther.asm

