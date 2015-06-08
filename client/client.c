#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <curl/curl.h>
#include <jansson.h>

#include "resolver.h"
#include "http.h"

#define SERVICE_NAME "TipcConfigService"

int main(int argc, char *argv[])
{
	struct sockaddr_storage ss;
	char url[255];
	char *response;

	if (!resolve(SERVICE_NAME, &ss)) {
		fprintf(stderr, "Failed to resolve %s\n", SERVICE_NAME);
		return -1;
	}
	build_url(&ss, "?request_config", url, sizeof(url));
	printf("%s\n",url);
	response = curlreq(url);
	if (!response) {
		fprintf(stderr, "No response received from server (%s)\n", url);
		return -1;
	}
	printf("%s", response);

	return 0;
}
