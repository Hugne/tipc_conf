#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <curl/curl.h>

#include "http.h"

#define BUFFER_SIZE  (256 * 1024)  /* 256 KB */

struct write_result
{
	char *data;
	int pos;
};

char *build_url(struct sockaddr_storage *ss, const char *cmd,
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


char *curlreq(const char *url) {
	CURL *curl = NULL;
	CURLcode status;
	struct curl_slist *headers = NULL;
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


