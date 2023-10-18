TARGET := hello-kernel

obj-m += $(TARGET).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

nod:
	sudo mknod /dev/$(TARGET) c 64 0

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

tidy: clean
	sudo rm /dev/$(TARGET)
