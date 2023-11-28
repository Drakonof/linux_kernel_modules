obj-m += kcu105_base.o
obj-m += gpio.o
obj-m += uartlite.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

