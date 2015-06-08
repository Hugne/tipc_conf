CFLAGS= -m64 -std=gnu99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes -Werror

LDFLAGS= -lavahi-core -lavahi-common -lpthread --static

.PHONY: all
all: client

client: client.c resolver.c
	gcc resolver.c client.c $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o client

