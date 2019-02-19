obj-m := chardev.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Some tutorials use M= instead of SUBDIRS= You may need to be root to
# compile the module. You must be root to insert it.
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -o ioctl ioctl.c
	mknod /dev/char_dev c 100 0
clean:
	rm -f ioctl
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Added by PAP 
test:
	dmesg -C
	insmod chardev.ko
	ls -l /dev/char_dev
	echo "Hello DVS. PAP" > /dev/char_dev
	cat /dev/char_dev
	./ioctl
#	rmmod chardev.ko
	dmesg | grep char_dev