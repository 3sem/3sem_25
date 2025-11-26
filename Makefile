FLAGS = -Wextra -Wall

B_PREFIX = bin/
O_PREFIX = objects/
S_PREFIX = sources/
H_PREFIX = headers/
T_PREFIX = text_file/

SOURCES = main

OBJECTS := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES))

HEADER_LIST = $(H_PREFIX)*.h

all: main

run: main
	@./$(B_PREFIX)main

main: $(OBJECTS)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@

$(O_PREFIX)%.o: $(S_PREFIX)%.c $(HEADER_LIST)
	@mkdir -p $(O_PREFIX)
	@gcc $(FLAGS) -I $(H_PREFIX) $< -c -o $@

clean:
	rm -rf $(O_PREFIX)*.o $(B_PREFIX)*
