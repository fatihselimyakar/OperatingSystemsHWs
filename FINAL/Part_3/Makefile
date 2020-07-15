all : part3

part3 : part3.o
	gcc -o part3 part3.o -lm -pthread 

part3.o : part3.c
	gcc -c part3.c -Wall -Wextra -pedantic

clean:
	rm -f part3 *.o
