CFLAGS=-Wextra -O2 -I../protobuf-c -DPC_BUILD
LDFLAGS=-lssl
all: iddc

iddc: iddc.c iddconfigparser.c protos/agent.pb-c.c ../idd_config.h ../api/idd_api.c ../iddd/common.pb-c.c ../iddd/iddd.pb-c.c ../iddd/configuration.pb-c.c ../common/envelope.c ../protobuf-c/protobuf-c.a
	gcc ${CFLAGS} -o iddc iddc.c iddconfigparser.c protos/agent.pb-c.c ../api/idd_api.c ../iddd/common.pb-c.c ../iddd/iddd.pb-c.c ../iddd/configuration.pb-c.c ../common/envelope.c ../protobuf-c/protobuf-c.a ${LDFLAGS}

