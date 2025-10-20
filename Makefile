FLAGS = -Wextra -Wall

B_PREFIX = bin/
O_PREFIX = objects/
S_PREFIX = sources/
H_PREFIX = headers/
T_PREFIX = text/

SOURCES = main duplex_pipe

OBJECTS := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES))

HEADER_LIST = $(H_PREFIX)*.h

all: main

run: main $(T_PREFIX)text
	@./$(B_PREFIX)main
	@make compare

compare: $(T_PREFIX)text $(T_PREFIX)text_dup $(T_PREFIX)text_tmp
	md5sum $(T_PREFIX)text
	md5sum $(T_PREFIX)text_tmp
	md5sum $(T_PREFIX)text_dup
	echo compare results

main: $(OBJECTS)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@

$(O_PREFIX)%.o: $(S_PREFIX)%.c $(HEADER_LIST)
	@mkdir -p $(O_PREFIX)
	@gcc $(FLAGS) -I $(H_PREFIX) $< -c -o $@

generate_text:
	rm -f $(T_PREFIX)text
	time dd if=/dev/urandom bs=1048576 count=4096 | tr -d '\377' | head -c 4G >$(T_PREFIX)text

clean:
	rm -rf $(O_PREFIX)*.o $(B_PREFIX)* $(T_PREFIX)*