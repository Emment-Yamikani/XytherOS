APP_DIR				:= $(USR_DIR)/app
USR_LIB_DIR 		:= $(USR_DIR)/lib

USR_FLAGS:=
USR_C_INCLUDE_PATH	:= $(USR_DIR)/include
USR_LINKER_SCRIPT	:= $(USR_DIR)/linker.ld

# USR_CFLAGS 			:= -ffreestanding -O2 -g -Wall -Wextra -Werror $(USR_FLAGS) \
# 						-fno-stack-protector -fno-exceptions -fno-pic \
# 						-mno-red-zone -mno-mmx -msse -m64 -mcmodel=large \
# 						-I$(USR_C_INCLUDE_PATH) -nostdlib -nostartfiles -nodefaultlibs \
# 						-std=gnu99 -ffunction-sections -fdata-sections

USR_CFLAGS			:= -O2 -g -ffreestanding -nostartfiles -nostdinc -nostdlib -lgcc -std=gnu2x \
						-march=x86-64 -mcmodel=large -mno-red-zone -mmmx -mno-sse \
						-I$(USR_C_INCLUDE_PATH) -Wall -Werror -Werror

USR_LDFLAGS 		:= -T $(USR_LINKER_SCRIPT) --gc-sections -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000

USR_ASFLAGS     	:= -f elf64

USR_C_SRCS  		:= $(shell find $(USR_DIR) -name '*.c')
USR_ASM_SRCS   		:= $(shell find $(USR_DIR) -name '*.s')
USR_NASM_SRCS  		:= $(shell find $(USR_DIR) -name '*.asm')
USR_FONT_SRCS		:= $(shell find $(uSR_DIR) -name '*.tf')

USR_LIB_OBJS    	:= $(USR_C_SRCS:.c=.o) $(USR_ASM_SRCS:.s=.s.o) $(USR_NASM_SRCS:.asm=.o) $(USR_FONT_SRCS:.tf=.o)

USR_LIB_C_SRCS  	:= $(shell find $(USR_LIB_DIR) -name '*.c')
USR_LIB_ASM_SRCS   	:= $(shell find $(USR_LIB_DIR) -name '*.s')
USR_LIB_NASM_SRCS  	:= $(shell find $(USR_LIB_DIR) -name '*.asm')
USR_LIB_FONT_SRCS	:= $(shell find $(uSR_LIB_DIR) -name '*.tf')

USR_LIB_OBJS    	:= $(USR_LIB_C_SRCS:.c=.o) $(USR_LIB_ASM_SRCS:.s=.s.o) $(USR_LIB_NASM_SRCS:.asm=.o) $(USR_LIB_FONT_SRCS:.tf=.o)

# Compile C Sources
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(USR_CFLAGS) -c $< -o $@

$(USR_LIB_DIR)/xlib.a: $(USR_LIB_OBJS)
	$(AR) rcs $(USR_LIB_DIR)/xlib.a $^

USR_LIB := $(USR_LIB_DIR)/xlib.a

APP_OBJ := $(APP_DIR)/%.o

%: $(APP_OBJ) $(USR_LIB)
	$(LD) $(USR_LDFLAGS) -o $(RAMFS_DIR)/$@ $^