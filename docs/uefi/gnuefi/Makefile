ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

GLIB_LIB     = $(shell pkg-config --libs gthread-2.0)
GLIB_INCLUDE = $(shell pkg-config --cflags gun-efi)

$(info $(ARCH))
OBJS            = main.o
TARGET          = hello.efi

# 普通系统的配置
EFI_LIBPATH		= /usr
EFI_LIBPATH		= $(shell nix-build -E '(import <nixpkgs> {}).gnu-efi' --no-out-link)/

EFIINC          = $(EFI_LIBPATH)/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB             = $(EFI_LIBPATH)/lib
EFILIB          = $(EFI_LIBPATH)/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS          = $(EFIINCS) -fno-stack-protector -fpic \
		  -fshort-wchar -mno-red-zone -Wall

ifeq ($(ARCH),x86_64)
  CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
		  -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

all: $(TARGET)

hello.so: $(OBJS)
	ld $(LDFLAGS) $(OBJS) -o $@ -lefi -lgnuefi

%.efi: %.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $@
