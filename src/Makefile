CC := gcc
C_FILES := $(wildcard *.c)
HEADER_FILES := $(wildcard *.h)
OBJ_FILES := $(patsubst %.c,%.o,$(C_FILES))
INCLUDE_DIRS := -I/usr/local/include/luajit-2.0/
LDFLAGS := -Wl,-rpath,/usr/local/lib
LIBS := -lluajit-5.1

all: rsp foo

install: rsp
	cp rsp ..

clean:
	rm $(OBJ_FILES)
	rm rsp

%.o: %.c $(HEADER_FILES)
	$(CC) -c -Wall -std=gnu99 $(INCLUDE_DIRS) $<

rsp: $(OBJ_FILES) 
	$(CC) -o $@ $(OBJ_FILES) $(LDFLAGS) $(LIBS)

foo: foo.o epollinterface.o
	$(CC) -o $@ foo.o epollinterface.o $(LDFLAGS) $(LIBS)
