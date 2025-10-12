shell_wrapper: src/main.c src/shell_wrapper.c
	gcc -Wall -Iinclude -o shell_wrapper src/main.c src/shell_wrapper.c

clean:
	rm -f shell_wrapper

run: shell_wrapper
	./shell_wrapper