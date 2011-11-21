This is a wee bit strange, but on win32 it appears that the only way to combine
static libraries into a monolithic static lib is to first extract the object
files from the static libs you want to link in and then link all the
individual object files together. kind of crazy.

These were extracted via `ar -x libspeex.a`
