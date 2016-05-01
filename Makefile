all:
	gcc -m32 xc.c -o xc
	gcc -m32 xci.c -o xci
clean:
	rm -f xc xci
