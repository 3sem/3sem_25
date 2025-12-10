FLAGS = -Wextra -Wall

B_PREFIX = bin/
O_PREFIX = objects/
S_PREFIX = sources/
H_PREFIX = headers/
T_PREFIX = files/

SOURCES_SND = sender barterer
SOURCES_RCV = reciever barterer

OBJECTS_SND := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES_SND))
OBJECTS_RCV := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES_RCV))

HEADER_LIST = $(H_PREFIX)*.h

all: sender reciever generate

run_snd: sender generate
	@./$(B_PREFIX)sender files/text_snd
run_rcv: reciever
	@./$(B_PREFIX)reciever files/text_rcv

sender: $(OBJECTS_SND)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@

reciever: $(OBJECTS_RCV)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@

generate: $(T_PREFIX)text_snd

check: $(T_PREFIX)text_snd $(T_PREFIX)text_rcv
	md5sum $(T_PREFIX)text_snd
	md5sum $(T_PREFIX)text_rcv

$(T_PREFIX)text_snd:
	@mkdir -p $(T_PREFIX)
	time dd if=/dev/urandom bs=1MB count=1 > $@

$(O_PREFIX)%.o: $(S_PREFIX)%.c $(HEADER_LIST)
	@mkdir -p $(O_PREFIX)
	@gcc $(FLAGS) -I $(H_PREFIX) $< -c -o $@

clean:
	rm -rf $(O_PREFIX)*.o $(B_PREFIX)*
