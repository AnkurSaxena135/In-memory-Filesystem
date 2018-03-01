ramdisk: ramdisk.c 
	gcc -Wall ramdisk.c `pkg-config fuse --cflags --libs` -o ramdisk

debug: 	
	./ramdisk ./fusetest -f 12
unmount:
	fusermount -u ./fusetest
