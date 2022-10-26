QEMU_OPTIONS = -M q35 -m 128M -smp 4 -serial stdio -d cpu_reset -d int -device qemu-xhci,id=xhci -device usb-tablet,bus=xhci.0,port=1 -no-reboot -no-shutdown -monitor telnet:127.0.0.1:1234,server,nowait
PROJECT_NAME = gewodsys

.PHONY: all
all: $(PROJECT_NAME).iso

.PHONY: all-hdd
all-hdd: $(PROJECT_NAME).hdd

.PHONY: run
run: $(PROJECT_NAME).iso
	qemu-system-x86_64 $(QEMU_OPTIONS) -cdrom $(PROJECT_NAME).iso -boot d

.PHONY: run-uefi
run-uefi: ovmf-x64 $(PROJECT_NAME).iso
	qemu-system-x86_64 $(QEMU_OPTIONS) -bios ovmf-x64/OVMF.fd -cdrom $(PROJECT_NAME).iso -boot d

.PHONY: run-uefi-lldb
run-uefi-lldb: ovmf-x64 $(PROJECT_NAME).iso
	qemu-system-x86_64 $(QEMU_OPTIONS) -bios ovmf-x64/OVMF.fd -cdrom $(PROJECT_NAME).iso -boot d -S -s

.PHONY: run-hdd
run-hdd: $(PROJECT_NAME).hdd
	qemu-system-x86_64 $(QEMU_OPTIONS) -hda $(PROJECT_NAME).hdd

.PHONY: run-hdd-uefi
run-hdd-uefi: ovmf-x64 $(PROJECT_NAME).hdd
	qemu-system-x86_64 $(QEMU_OPTIONS) -bios ovmf-x64/OVMF.fd -hda $(PROJECT_NAME).hdd

ovmf-x64:
	mkdir -p ovmf-x64
	cd ovmf-x64 && curl -o OVMF-X64.zip https://efi.akeo.ie/OVMF/OVMF-X64.zip && unzip OVMF-X64.zip

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1
	make -C limine

.PHONY: kernel
kernel:
	make -C kernel

$(PROJECT_NAME).iso: limine kernel userland
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/kernel.elf \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(PROJECT_NAME).iso
	limine/limine-deploy $(PROJECT_NAME).iso
	rm -rf iso_root

$(PROJECT_NAME).hdd: limine kernel userland
	rm -f $(PROJECT_NAME).hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(PROJECT_NAME).hdd
	parted -s $(PROJECT_NAME).hdd mklabel gpt
	parted -s $(PROJECT_NAME).hdd mkpart ESP fat32 2048s 100%
	parted -s $(PROJECT_NAME).hdd set 1 esp on
	limine/limine-deploy $(PROJECT_NAME).hdd
	sudo losetup -Pf --show $(PROJECT_NAME).hdd >loopback_dev
	sudo mkfs.fat -F 32 `cat loopback_dev`p1
	mkdir -p img_mount
	sudo mount `cat loopback_dev`p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v kernel/kernel.elf limine.cfg limine/limine.sys img_mount/
	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d `cat loopback_dev`
	rm -rf loopback_dev img_mount

.PHONY: clean
clean:
	rm -rf iso_root $(PROJECT_NAME).iso $(PROJECT_NAME).hdd
	make -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine ovmf-x64
	make -C kernel distclean
