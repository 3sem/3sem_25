FLAGS = -Wextra -Wall

B_PREFIX = bin/
O_PREFIX = objects/
S_PREFIX = sources/
H_PREFIX = headers/
T_PREFIX = text/

SOURCES_SHM = main_shm shared_mem_send common
SOURCES_FIFO = main_fifo fifo_send common
SOURCES_MSG = main_msg msg_send common
OBJECTS_SHM := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES_SHM))
OBJECTS_FIFO := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES_FIFO))
OBJECTS_MSG := $(patsubst %,$(O_PREFIX)%.o,$(SOURCES_MSG))

TEXTES_ := small big average
TEXTES := $(patsubst %,$(T_PREFIX)%,$(TEXTES_))
TEXTES_SHM := $(patsubst %, $(T_PREFIX)%_recieved_shm,$(TEXTES_))
TEXTES_FIFO := $(patsubst %, $(T_PREFIX)%_recieved_fifo,$(TEXTES_))
TEXTES_MSG := $(patsubst %, $(T_PREFIX)%_recieved_msg,$(TEXTES_))

HEADER_LIST = $(H_PREFIX)*.h

run_all: run_shm run_fifo run_msg compare_all

all: main_shm main_fifo main_msg

run_shm: main_shm $(TEXTES);
	@./$(B_PREFIX)main_shm $(T_PREFIX)small $(T_PREFIX)small_recieved_shm
	@./$(B_PREFIX)main_shm $(T_PREFIX)average $(T_PREFIX)average_recieved_shm
	@./$(B_PREFIX)main_shm $(T_PREFIX)big $(T_PREFIX)big_recieved_shm

run_fifo: main_fifo $(TEXTES);
	@./$(B_PREFIX)main_fifo $(T_PREFIX)small $(T_PREFIX)small_recieved_fifo
	@./$(B_PREFIX)main_fifo $(T_PREFIX)average $(T_PREFIX)average_recieved_fifo
	@./$(B_PREFIX)main_fifo $(T_PREFIX)big $(T_PREFIX)big_recieved_fifo

run_msg: main_msg $(TEXTES);
	@./$(B_PREFIX)main_msg $(T_PREFIX)small $(T_PREFIX)small_recieved_msg
	@./$(B_PREFIX)main_msg $(T_PREFIX)average $(T_PREFIX)average_recieved_msg
	@./$(B_PREFIX)main_msg $(T_PREFIX)big $(T_PREFIX)big_recieved_msg


compare_all: $(TEXTES) $(TEXTES_FIFO) $(TEXTES_MSG) $(TEXTES_SHM);
	@md5sum $(T_PREFIX)small
	@md5sum $(T_PREFIX)small_recieved_msg
	@md5sum $(T_PREFIX)small_recieved_fifo
	@md5sum $(T_PREFIX)small_recieved_shm
	@md5sum $(T_PREFIX)average
	@md5sum $(T_PREFIX)average_recieved_shm
	@md5sum $(T_PREFIX)average_recieved_fifo
	@md5sum $(T_PREFIX)average_recieved_msg
	@md5sum $(T_PREFIX)big
	@md5sum $(T_PREFIX)big_recieved_shm
	@md5sum $(T_PREFIX)big_recieved_fifo
	@md5sum $(T_PREFIX)big_recieved_msg

compare_shm: $(TEXTES_SHM) $(TEXTES);
	@md5sum $(T_PREFIX)small
	@md5sum $(T_PREFIX)small_recieved_shm
	@md5sum $(T_PREFIX)average
	@md5sum $(T_PREFIX)average_recieved_shm
	@md5sum $(T_PREFIX)big
	@md5sum $(T_PREFIX)big_recieved_shm

compare_msg: $(TEXTES_MSG) $(TEXTES);
	@md5sum $(T_PREFIX)small
	@md5sum $(T_PREFIX)small_recieved_msg
	@md5sum $(T_PREFIX)average
	@md5sum $(T_PREFIX)average_recieved_msg
	@md5sum $(T_PREFIX)big
	@md5sum $(T_PREFIX)big_recieved_msg

compare_fifo: $(TEXTES_FIFO) $(TEXTES);
	@md5sum $(T_PREFIX)small
	@md5sum $(T_PREFIX)small_recieved_fifo
	@md5sum $(T_PREFIX)average
	@md5sum $(T_PREFIX)average_recieved_fifo
	@md5sum $(T_PREFIX)big
	@md5sum $(T_PREFIX)big_recieved_fifo


main_shm: $(OBJECTS_SHM)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@

main_fifo: $(OBJECTS_FIFO)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@

main_msg: $(OBJECTS_MSG)
	@mkdir -p $(B_PREFIX)
	@gcc $(FLAGS) $^ -o $(B_PREFIX)$@


$(T_PREFIX)small:
	@mkdir -p $(T_PREFIX)
	time dd if=/dev/urandom bs=1024b count=8 | tr -d '\377' | head -c 8192 > $@

$(T_PREFIX)average:
	@mkdir -p $(T_PREFIX)
	time dd if=/dev/urandom bs=1024b count=65 | tr -d '\377' | head -c 66560 > $@

$(T_PREFIX)big:
	@mkdir -p $(T_PREFIX)
	time dd if=/dev/urandom bs=1MB count=128 | tr -d '\377' | head -c 128MB > $@

$(O_PREFIX)%.o: $(S_PREFIX)%.c $(HEADER_LIST)
	@mkdir -p $(O_PREFIX)
	@gcc $(FLAGS) -I $(H_PREFIX) $< -c -o $@

clean:
	rm -rf $(O_PREFIX)* $(B_PREFIX)* $(T_PREFIX)*
