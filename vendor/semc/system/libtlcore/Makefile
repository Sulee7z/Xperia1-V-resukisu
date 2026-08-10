#This Make file is used to build the library for Linux environment
PROGRAM=tlcore

TLCORE_CFLAGS := -fPIC -g -Wall -Wformat -Wextra
TLCORE_CFLAGS += -DDEBUG_LEVEL=0

SHARED_FLAGS= -shared

INCLUDES= -I ./ -I ./src/
FILES= tlcore.o \
	tlcore_header.o \
	section.o \
	section_header.o \
	section_data.o \
	sha1.o \
	tlcore_utils.o

OBJECT_FILES= $(addprefix src/,$(FILES))

.PHONY: all clean install
all: lib${PROGRAM}.so

lib${PROGRAM}.so: ${OBJECT_FILES}
	${CC} ${SHARED_FLAGS} -o lib${PROGRAM}.so ${OBJECT_FILES}

#Rule for creating a static library. Run "make libtlcore.a"
lib${PROGRAM}.a: ${OBJECT_FILES}
	ar -rs lib${PROGRAM}.a ${OBJECT_FILES}

%.o: %.c
	$(CC) -c -o $@ $< $(TLCORE_CFLAGS) $(INCLUDES)

install:
	/usr/bin/install lib${PROGRAM}.so /usr/lib
	/usr/bin/install tlcore.h /usr/include/

clean:
	-rm -rf $(OBJECT_FILES) lib${PROGRAM}.so lib${PROGRAM}.a
