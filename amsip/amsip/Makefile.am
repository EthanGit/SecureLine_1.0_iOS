
EXTRA_DIST = README autogen.sh

SUBDIRS = src include scripts help

doxygen:
	doxygen help/doxygen/DoxyFile

indent:
	indent src/*.h
	indent src/*.c
	indent include/amsip/*.h
	rm -rf */*.h~
	rm -rf */*.c~
	rm -rf */*/*.c~
	rm -rf */*/*/*.h~


