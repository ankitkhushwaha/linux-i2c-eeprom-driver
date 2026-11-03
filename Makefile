# SPDX-License-Identifier: GPL-2.0
obj-m += at24c02_drv.o

at24c02_drv-y := at24c02_core.o
at24c02_drv-$(CONFIG_AT24C02_CHARDEV) += at24c02_chrdev.o

ccflags-y += -DDEBUG -I$(src)/include

KERNELDIR ?= /home/ankit/dev/linux-src/linux-mainline/build_qemu
HOSTDIR := /lib/modules/$(shell uname -r)/build

#QEMUDIR := /home/ankit/dev/qemu/linux-7.2.8/build-qemu
#QEMU_CC := /home/ankit/dev/qemu/arm-gnu-toolchain/bin/arm-none-linux-gnueabihf-
#QEMU_ARCH := arm

QEMUDIR := /home/ankit/dev/linux-src/linux-6.6.20-ti-rt-arm32-r3/build
QEMU_CC := arm-linux-gnueabihf-
QEMU_ARCH := arm

modules:
	make -C $(KERNELDIR) M=$(PWD) modules

qemu:
	make ARCH=$(QEMU_ARCH) CROSS_COMPILE=$(QEMU_CC) -C $(QEMUDIR) M=$(PWD) modules

host:
	make -C $(HOSTDIR) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
