GCC_ARGS = -Wall -std=gnu99 -g
#GCC_ARGS = -Wall -std=gnu99 -O2 -DNDEBUG
LINKER_ARGS = 

%.o: %.c %.h
	gcc $(GCC_ARGS) -c $<

clean:
	rm -f *.o
	cd tests; make clean