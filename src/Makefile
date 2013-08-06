C_FILES := $(wildcard *.c)
OBJ_FILES := $(addprefix ./,$(C_FILES:.c=.o))


all: rsp

clean:
	rm $(OBJ_FILES)
	rm rsp

%.o: %.c
	$(CC) -c $<

rsp: $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES)
