#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "resolver.h"

#define SERVICE_NAME "TipcConfigService"

static void print_ss(struct sockaddr_storage *ss){
	char hoststr[NI_MAXHOST];
	char portstr[NI_MAXSERV];
	socklen_t cl = sizeof(*ss);
	int rc = getnameinfo((struct sockaddr *)ss, cl, hoststr,
			     sizeof(hoststr), portstr, sizeof(portstr),
			     NI_NUMERICHOST | NI_NUMERICSERV);
	if (rc == 0) printf("Resolved %s to: %s %s\n",
			    SERVICE_NAME, hoststr, portstr);

}

int main(int argc, char *argv[])
{
	struct sockaddr_storage ss;

	if (!resolve(SERVICE_NAME, &ss)) {
		printf("Failed to resolve %s\n", SERVICE_NAME);
		return -1;
	}
	print_ss(&ss);



	return 0;
}
