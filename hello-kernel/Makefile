TARGET := hello-kernel

obj-m += $(TARGET).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mknod:
	sudo mknod /dev/$(TARGET) c 64 0
	ls -l /dev/$(TARGET)

rmnod:
	sudo rm /dev/$(TARGET)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

tidy: clean
	sudo rm /dev/$(TARGET)
