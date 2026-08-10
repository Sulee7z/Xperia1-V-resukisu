CFLAGS	= -Wextra -O2 -g -DPC_BUILD -D_GNU_SOURCE  ${GCOV} -I../protobuf-c -I../api -ggdb -pthread
LDFLAGS	= -lz -lssl -lcrypto

CFILES	= counters.c events.c sdata.c serializer.c util.c compactor.c list.c debug_if.c configuration.c configuration.pb-c.c common.pb-c.c filter.c iddd.pb-c.c settings.c crypto.c file_utils.c idd_system_logging.pb-c.c iddd_error_probe.c framework.pb-c.c iddd.c plugins.c ../common/envelope.c
HFILES	= counters.h events.h sdata.h serializer.h util.h compactor.h list.h debug_if.h configuration.h common.pb-c.h configuration.pb-c.h filter.h idd/iddd.pb-c.h settings.h crypto.h file_utils.h idd/idd_system_logging.pb-c.h iddd_error_probe.h framework.pb-c.h config.h ../idd_config.h plugins.h ../common/envelope.h
LIBS	= ../protobuf-c/protobuf-c.a ../api/libidd.a

all: iddd

iddd: ${CFILES} ${HFILES} ${LIBS}
	gcc ${CFLAGS} -o iddd ${CFILES} ${LIBS} ${LDFLAGS}

