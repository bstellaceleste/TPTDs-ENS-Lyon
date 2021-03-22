CC=gcc

obj-m += uio_device2.o

all: kmod app

kmod:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

app:
	$(CC) -o uio_user2 uio_user2.c

kmod-clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

app-clean:
	rm -f uio_user2

clean: kmod-clean app-clean
