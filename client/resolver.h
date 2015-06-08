#ifndef __RESOLVER_H__
#define __RESOLVER_H__

struct sockaddr_storage;

int resolve(const char *name, struct sockaddr_storage *ss);

#endif
