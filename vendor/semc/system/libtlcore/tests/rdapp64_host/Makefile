PROGRAM=rdapp64_host

INSTALL_DIR=/usr/bin/

TLCORE_CFLAGS= -g -Wall -Wformat -Wextra

OBJECT_FILES= rdapp.o snappy_wrap.o

all: ${OBJECT_FILES}
	${CC} -o ${PROGRAM} ${OBJECT_FILES} -ltlcore -lsnappy

rdapp.o: rdapp.c
	${CC} -c ${TLCORE_CFLAGS} rdapp.c

snappy_wrap.o: snappy_wrap.c
	${CC} -c ${TLCORE_CFLAGS} snappy_wrap.c

clean:
	-rm -rf ${PROGRAM} *.o
install:
	sudo /usr/bin/install ${PROGRAM} ${INSTALL_DIR}
