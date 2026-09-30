CC = gcc
OBJCOPY = objcopy
CARGO = cargo

GUEST_CFLAGS = -m64 -O2 -g -Wall -Wextra -Werror -ffreestanding \
	-fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	-fno-unwind-tables -mno-red-zone -mgeneral-regs-only
GUEST_LDFLAGS = -m64 -nostdlib -static -no-pie \
	-Wl,--build-id=none,-z,max-page-size=0x1000,-T,guest/linker.ld

.PHONY: all vmm run check clean
.DELETE_ON_ERROR:

all: vmm guest.out

# Cargo intermediates stay in ignored target/; the runnable copy uses .out.
vmm:
	$(CARGO) build --target-dir target
	cp target/debug/mini-kvm mini-kvm.out

guest/main.o: guest/main.c
	$(CC) $(GUEST_CFLAGS) -c $< -o $@

guest/start.o: guest/start.S
	$(CC) $(GUEST_CFLAGS) -c $< -o $@

guest.elf.out: guest/start.o guest/main.o guest/linker.ld
	$(CC) $(GUEST_LDFLAGS) guest/start.o guest/main.o -o $@

guest.out: guest.elf.out
	$(OBJCOPY) -O binary $< $@

run: all
	./mini-kvm.out guest.out

check: all
	$(CARGO) fmt --check
	$(CARGO) clippy --target-dir target --all-targets -- -D warnings
	timeout 10s ./mini-kvm.out guest.out

clean:
	$(CARGO) clean --target-dir target
	$(RM) mini-kvm.out guest.out guest.elf.out guest/start.o guest/main.o
