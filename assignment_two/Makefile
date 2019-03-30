obj-m :=block_device.o

all:	
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	rm -rf *.o *.mod *.mod.o *.symvers *.order *.mod.c *.ko
