obj-m := main.o

all:	
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

i: main.ko
	sudo insmod main.ko

r: main.ko
	sudo rmmod main.ko

c: userapp.c
	gcc -o userapp userapp.c

clean:
	rm -rf *.o *.mod *.mod.o *.symvers *.order *.ko *main.mod.c *userapp

