bin_PROGRAMS=client
client_SOURCES=client.c http.c misc.c  msg.c resolver.c
client_LDADD=$(LIBMNL_LIBS) $(LIBJANSSON_LIBS) $(LIBAVAHICORE_LIBS)
if STATIC
client_LDADD+=-lpthread
client_LDFLAGS=--static
endif
