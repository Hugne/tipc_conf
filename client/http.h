#ifndef __HTTP_H__
#define __HTTP_H__
char *build_url(struct sockaddr_storage *ss, const char *cmd, char *buf,
		int len);
char *curlreq(const char *url);

#endif
