CFLAGS= -m64 -std=gnu99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes -Werror

DFLAGS= -DCURL_STATICLIB

LDFLAGS= -lavahi-core -lavahi-common -lpthread -lcurl -ljansson -lmnl

.PHONY: all
all: client

client: client.c resolver.c http.c misc.c msg.c
	gcc msg.c misc.c http.c resolver.c client.c $(DFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o client

