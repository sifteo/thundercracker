#! /bin/sh

# libusb(x) uses a couple funkily flags that make it impossible to simply specify
# multiple arch flags. instead, compile for each platform separately and then
# stitch them together with lipo
#
# this should be executed from within the libusbx source tree

make distclean

env CFLAGS="-arch i386" ./configure
make
mv libusb/.libs/libusb-1.0.0.dylib /tmp/libusb-1.0.0.dylib.i386
mv libusb/.libs/libusb-1.0.a /tmp/libusb-1.0.a.i386

make distclean

env CFLAGS="-arch x86_64" ./configure
make
mv libusb/.libs/libusb-1.0.0.dylib /tmp/libusb-1.0.0.dylib.x86_64
mv libusb/.libs/libusb-1.0.a /tmp/libusb-1.0.a.x86_64

lipo -create /tmp/libusb-1.0.0.dylib.i386 /tmp/libusb-1.0.0.dylib.x86_64 -output libusb/.libs/libusb-1.0.0.dylib
lipo -create /tmp/libusb-1.0.a.i386 /tmp/libusb-1.0.a.x86_64 -output libusb/.libs/libusb-1.0.a

# print out the result
echo
echo "Done! Check it."
file libusb/.libs/libusb-1.0.0.dylib
file libusb/.libs/libusb-1.0.a
