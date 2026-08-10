PROGRAM=rd_extract

INSTALL_DIR=/usr/bin/

TLCORE_CFLAGS= -g -Wall -Wformat -Wextra

OBJECT_FILES= rd_extract.o snappy_wrap.o zlib_wrap.o

all: ${OBJECT_FILES}
	${CC} -o ${PROGRAM} ${OBJECT_FILES} -ltlcore -lsnappy -lz

rd_extract.o: rd_extract.c
	${CC} -c ${TLCORE_CFLAGS} rd_extract.c

snappy_wrap.o: snappy_wrap.c
	${CC} -c ${TLCORE_CFLAGS} snappy_wrap.c

zlib_wrap.o: zlib_wrap.c
	${CC} -c ${TLCORE_CFLAGS} zlib_wrap.c

clean:
	-rm -rf ${PROGRAM} *.o
install:
	sudo /usr/bin/install ${PROGRAM} ${INSTALL_DIR}
