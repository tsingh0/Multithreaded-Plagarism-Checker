all : compare

compare : compare.c
	gcc -Wall -Werror -fsanitize=address -pthread compare.c -o compare -lm

clean:
	rm -f compare