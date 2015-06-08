CFLAGS= -m64 -std=gnu99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes -Werror

DFLAGS= -DCURL_STATICLIB

LDFLAGS= -lavahi-core -lavahi-common -lpthread -lcurl #--static

.PHONY: all
all: client

client: client.c resolver.c
	gcc http.c resolver.c client.c $(DFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o client

