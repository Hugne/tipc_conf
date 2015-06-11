bin_PROGRAMS=client
client_SOURCES=client.c http.c misc.c  msg.c resolver.c
include_HEADERS=http.h misc.h msg.h resolver.h tipc.h tipc_netlink.h

client_LDADD=$(LIBMNL_LIBS) $(LIBJANSSON_LIBS) $(LIBAVAHICORE_LIBS)
if STATIC
client_LDADD+=-lpthread
client_LDFLAGS=--static
endif
