:
set -x
make -C /usr/src/linux-headers-$(uname -r) M=`pwd` modules CC=x86_64-linux-gnu-gcc-12
