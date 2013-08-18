C_FILES := $(wildcard *.c)
HEADER_FILES := $(wildcard *.h)
OBJ_FILES := $(patsubst %.c,%.o,$(C_FILES))


all: rsp

install: rsp
	cp rsp ..

clean:
	rm $(OBJ_FILES)
	rm rsp

%.o: %.c $(HEADER_FILES)
	$(CC) -c $<

rsp: $(OBJ_FILES) 
	$(CC) -o $@ $(OBJ_FILES)
