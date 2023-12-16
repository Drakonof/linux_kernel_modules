#!/usr/bin/python

#todo: args: dev name, led and button num, button test

"""
Author: A. Shimko
License: Public Domain

gpio LKM test
"""

import fcntl
import ctypes
import time
import sys

IORW = lambda t, n, s: _IO(t, n, ctypes.sizeof(s))

class Test:
	def __init__(self):
		self.dev_name = '/dev/device'
		self.wr = IORW(ord("g"), ord("w"), ctypes.c_ubyte)

		try:
		 	self.fd = open(self.dev_name, 'r+')

		except FileNotFoundError:
		    print(f"Device not found: {self.dev_name}")
		except IOError as e:
		    print(f"IOError occurred: {e}")
		except Exception as e:
		    print(f"An error occurred: {e}")

	def write(self):
		for i in range(8):
			fcntl.ioctl(self.fd, self.wr, i)
			time.sleep(0.5)

	def __del__(self):
		self.fd.close()

def main():
	test = Test()

	test.write()


if __name__ == "__main__":
    sys.exit(main())	