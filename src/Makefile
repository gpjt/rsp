CC := gcc
C_FILES := $(wildcard *.c)
HEADER_FILES := $(wildcard *.h)
OBJ_FILES := $(patsubst %.c,%.o,$(C_FILES))
INCLUDE_DIRS := -I/usr/local/include/luajit-2.0/
LDFLAGS := -Wl,-rpath,/usr/local/lib
LIBS := -lluajit-5.1

all: rsp

install: rsp
	cp rsp ..

clean:
	rm -f $(OBJ_FILES)
	rm -f rsp

%.o: %.c $(HEADER_FILES)
	$(CC) -c -Wall -g -std=gnu99 $(INCLUDE_DIRS) $<

rsp: $(OBJ_FILES) 
	$(CC) -o $@ $(OBJ_FILES) $(LDFLAGS) $(LIBS)
