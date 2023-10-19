TARGET := hello-kernel

obj-m += $(TARGET).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

insmod:
	sudo insmod ./$(TARGET).ko major_num=64 hello_str="Hello"

mknod: insmod
	sudo mknod /dev/$(TARGET) c 64 0
	ls -l /dev/$(TARGET)

rmmod:
	sudo rmmod $(TARGET)

rmnod: rmmod
	sudo rm /dev/$(TARGET)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

tidy: clean
	sudo rm /dev/$(TARGET)
