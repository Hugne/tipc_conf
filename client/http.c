#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "http.h"

#define BUFFER_SIZE  (256 * 1024)  /* 256 KB */

#ifndef USE_CURL
char *httpget(struct sockaddr_storage *server, const char *page) {
	int sock;
	int res;
	int sent = 0;
	char get[128] = { 0 };
	char *response, *json;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("Could not create TCP socket\n");
		return NULL;
	}

	if (connect(sock, (struct sockaddr*)server, sizeof(*server)) < 0) {
		perror("Could not connect to server\n");
		return NULL;
	}
	if (!(response = malloc(BUFFER_SIZE))) {
		fprintf(stderr, "Failed to allocate response buffer\n");
		return NULL;
	}

	snprintf(get, 128, "GET /%s HTTP/1.1\r\n User-Agent: TIPC\r\n\r\n",
		 page);
	do {
		res = send(sock, get+sent, strlen(get) - sent, 0);
		if (res < 0) {
			perror("Failed to send query\n");
			return NULL;
		}
		sent += res;
	} while (sent < strlen(get));

	if (recv(sock, response, BUFFER_SIZE, MSG_WAITALL) < 0) {
		perror("recv");
		return NULL;
	}
	/*Strip HTML header and return pure json*/
	json = strdup(strstr(response, "\r\n\r\n"));
	free(response);
	return json;
}

#else
#include <curl/curl.h>

struct write_result
{
	char *data;
	int pos;
};

static char *build_url(struct sockaddr_storage *ss, const char *cmd,
		char *buf, int len)
{
	char h[NI_MAXHOST];
	char p[NI_MAXSERV];
	socklen_t slen = sizeof(*ss);

	if (getnameinfo((struct sockaddr *)ss, slen, h,
			sizeof(h), p, sizeof(p),
			NI_NUMERICHOST | NI_NUMERICSERV)) {
		fprintf(stderr, "Failed to build URL\n");
		return NULL;
	}

	snprintf(buf, len, "http://%s:%s/%s", h, p, cmd);
	return buf;
}

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct write_result *result = (struct write_result *)stream;

	if(result->pos + size * nmemb >= BUFFER_SIZE - 1) {
		fprintf(stderr, "error: too small buffer\n");
		return 0;
	}
	memcpy(result->data + result->pos, ptr, size * nmemb);
	result->pos += size * nmemb;
	return size * nmemb;
}

char *httpget(struct sockaddr_storage *server, const char *page) {
	CURL *curl = NULL;
	CURLcode status;
	struct curl_slist *headers = NULL;
	char url[255];
	char *data = NULL;
	long code;
	struct write_result write_result;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(!curl)
        	goto error;
	data = malloc(BUFFER_SIZE);
	if(!data)
	        goto error;

	build_url(server, page, url, sizeof(url));
	write_result.data = data;
	write_result.pos = 0;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	headers = curl_slist_append(headers, "User-Agent: Dickbutt");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	status = curl_easy_perform(curl);
	if(status != 0) {
		fprintf(stderr, "error: unable to request data from %s:\n", url);
		fprintf(stderr, "%s\n", curl_easy_strerror(status));
		goto error;
	}
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if(code != 200) {
		fprintf(stderr, "error: server responded with code %ld\n", code);
		goto error;
	}
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	curl_global_cleanup();
	data[write_result.pos] = '\0';
	return data;
error:
	if(data)
		free(data);
	if(curl)
		curl_easy_cleanup(curl);
	if(headers)
		curl_slist_free_all(headers);
	curl_global_cleanup(); 
	return NULL;
}

#endif


