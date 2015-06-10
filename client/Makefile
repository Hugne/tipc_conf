CFLAGS= -m64 -std=gnu99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes -Werror

LDFLAGS= -lavahi-core -lavahi-common -lpthread -ljansson -lmnl

ifdef USE_CURL
LDFLAGS += -lcurl
CFLAGS += -DUSE_CURL
endif

.PHONY: all
all: client

client: client.c resolver.c http.c misc.c msg.c
	gcc msg.c misc.c http.c resolver.c client.c $(DFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o client

