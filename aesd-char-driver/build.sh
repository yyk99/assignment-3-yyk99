:
set -x
set -e
make -C /usr/src/linux-headers-$(uname -r) M=`pwd` modules CC=x86_64-linux-gnu-gcc-12
make aesd_test
sudo ./aesdchar_unload ; sudo ./aesdchar_load
