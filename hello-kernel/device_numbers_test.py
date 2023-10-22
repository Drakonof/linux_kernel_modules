import os

dev = os.open("/dev/device_numbers", os.O_RDONLY)

if dev == -1:
	print("Openning failed")
else:
	print("Openning successfull")

os.close(dev)

log = os.system('dmesg | grep device_numbers')
print(log)