TARGET := hello-kernel

obj-m += $(TARGET).o
obj-m += timer.o
obj-m += hrtimer.o
obj-m += dt_probe.o
obj-m += ioctl-demo.o
obj-m += kthread-demo.o

all: dt
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dt: testoverlay.dts
	dtc -@ -I dts -O dtb -o testoverlay.dtbo testoverlay.dts

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
