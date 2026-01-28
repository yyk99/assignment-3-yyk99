# AESD Char Driver

Template source code for the AESD char driver used with assignments 8 and later

## Build the host module (DEBUG)

	make -C /usr/src/linux-headers-$(uname -r) M=`pwd` modules CC=x86_64-linux-gnu-gcc-12

or

	make clean
