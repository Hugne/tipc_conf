#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <curl/curl.h>
#include <jansson.h>
#include <linux/genetlink.h>
#include <libmnl/libmnl.h>

#include "resolver.h"
#include "http.h"
#include "misc.h"
#include "msg.h"
#include "tipc.h"
#include "tipc_netlink.h"

#define SERVICE_NAME "TipcConfigService"


static int set_media_prop(const char *media, const char *key,
			  const unsigned int val)
{
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlattr *props;
	struct nlattr *attrs;
	int prop;

	if (strcmp(key, "priority") == 0)
		prop = TIPC_NLA_PROP_PRIO;
	else if ((strcmp(key, "tolerance") == 0))
		prop = TIPC_NLA_PROP_TOL;
	else if ((strcmp(key, "window") == 0))
		prop = TIPC_NLA_PROP_WIN;
	else
		return -EINVAL;

	if (!(nlh = msg_init(buf, TIPC_NL_MEDIA_SET))) {
		fprintf(stderr, "error, message initialisation failed\n");
		return -1;
	}
	attrs = mnl_attr_nest_start(nlh, TIPC_NLA_MEDIA);
	mnl_attr_put_strz(nlh, TIPC_NLA_MEDIA_NAME, media);
	props = mnl_attr_nest_start(nlh, TIPC_NLA_MEDIA_PROP);
	mnl_attr_put_u32(nlh, prop, val);
	mnl_attr_nest_end(nlh, props);
	return msg_doit(nlh, NULL, NULL);
}

static int get_netid_cb(const struct nlmsghdr *nlh, void *data)
{
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
	struct nlattr *info[TIPC_NLA_MAX + 1] = { 0 };
	struct nlattr *attrs[TIPC_NLA_NET_MAX + 1] = { 0 };
	int *netid = (int*)data;

	mnl_attr_parse(nlh, sizeof(*genl), parse_attrs, info);
	if (!info[TIPC_NLA_NET])
		return MNL_CB_ERROR;
	mnl_attr_parse_nested(info[TIPC_NLA_NET], parse_attrs, attrs);
	if (!attrs[TIPC_NLA_NET_ID])
		return MNL_CB_ERROR;
	*netid = mnl_attr_get_u32(attrs[TIPC_NLA_NET_ID]);

	return MNL_CB_OK;
}

static int generate_multicast(short af, char *buf, int bufsize)
{
	int netid;
	char mnl_msg[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;

	if (!(nlh = msg_init(mnl_msg, TIPC_NL_NET_GET))) {
		fprintf(stderr, "error, message initialization failed\n");
		return -1;
	}
	if (msg_dumpit(nlh, get_netid_cb, &netid)) {
		fprintf(stderr, "error, failed to fetch TIPC network id from kernel\n");
		return -EINVAL;
	}
	if (af == AF_INET)
		snprintf(buf, bufsize, "228.0.%u.%u", (netid>>8) & 0xFF, netid & 0xFF);
	else
		snprintf(buf, bufsize, "ff02::%u", netid);

	return 0;
}

static int configure_tipc_address(json_t *addrconf)
{
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlattr *nest;
	struct nlmsghdr *nlh;
	json_t *tipcaddr;
	uint32_t addr;

	if (!addrconf) {
		fprintf(stderr, "No address configuration data present\n");
		return -1;
	}
	tipcaddr = json_object_get(addrconf, "tipcaddress");
	if (json_is_string(tipcaddr))
		printf("TIPC address: %s\n", json_string_value(tipcaddr));
	addr = str2addr(json_string_value(tipcaddr));
	
	if (!(nlh = msg_init(buf, TIPC_NL_NET_SET))) {
		fprintf(stderr, "error, message initialisation failed\n");
		return -1;
	}
	nest = mnl_attr_nest_start(nlh, TIPC_NLA_NET);
	mnl_attr_put_u32(nlh, TIPC_NLA_NET_ADDR, addr);
	mnl_attr_nest_end(nlh, nest);
	return msg_doit(nlh, NULL, NULL);
}

static int do_one_bearer(json_t *b)
{
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlattr *nest, *props;
	struct nlmsghdr *nlh;
	json_t *data;
	struct bearer_config {
		/*Common for all bearer types*/
		const char *media;
		const char *win;
		const char *tol;
		const char *pri;
		const char *dom;
		/*Eth/IB only*/
		const char *device;
		/*UDP only*/
		const char *name;
		const char *localip;
		const char *localport;
		const char *remoteip;
		const char *remoteport;

	} bc = {0};


	if (!(nlh = msg_init(buf, TIPC_NL_BEARER_ENABLE))) {
		fprintf(stderr, "error: message initialisation failed\n");
		return -1;
	}
	nest = mnl_attr_nest_start(nlh, TIPC_NLA_BEARER);

	/*TODO: This could use some cleaning up.. make it
	 * look more like iproute2/tipc/bearer.c code..*/

	/*Mandatory: Bearer media type*/
	data = json_object_get(b, "media");
	if (data)
		bc.media = json_string_value(data);
	else
		return -1;

	/**
	 * We change the media defaults for tol/win here.. not good but the NL
	 * api does not support setting these attributes at bearer creation time
	 */
	data = json_object_get(b, "tolerance");
	if (data) {
		bc.tol = json_string_value(data);
		if (set_media_prop(media, "tolerance", atoi(bc.tol)))
			fprintf(stderr, "Failed to set tolerance\n");
	}
	data = json_object_get(b, "window");
	if (data) {
		bc.win = json_string_value(data);
		if (set_media_prop(media, "window", atoi(bc.win)))
			fprintf(stderr, "Failed to set window\n");
	}
	/*Optional: Bearer domain*/
	data = json_object_get(b, "domain");
	if (data) {
		bc.dom = json_string_value(data);
		mnl_attr_put_u32(nlh, TIPC_NLA_BEARER_DOMAIN, str2addr(bc.dom));
	}
	/*Optional: Bearer priority*/
	data = json_object_get(b, "priority");
	if (data) {
		bc.pri = json_string_value(data);
		props = mnl_attr_nest_start(nlh, TIPC_NLA_BEARER_PROP);
		mnl_attr_put_u32(nlh, TIPC_NLA_PROP_PRIO, atoi(bc.pri));
		mnl_attr_nest_end(nlh, props);
	}
	if (!strcmp(bc.media, "eth") || !strcmp(bc.media, "ib")) {
		char id[TIPC_MAX_BEARER_NAME];
		/*Mandatory: Bearer interface name for l2 media types*/
		data = json_object_get(b, "interface");
		if (!data)
			return -1;
		bc.device = json_string_value(data);
		snprintf(id, sizeof(id), "%s:%s", bc.media, bc.device);
		mnl_attr_put_strz(nlh, TIPC_NLA_BEARER_NAME, id);
	} else if (!strcmp(bc.media, "udp")) {
		struct nlattr *nest_udp;
		int err;
		char addrbuf[INET6_ADDRSTRLEN];
		char id[TIPC_MAX_BEARER_NAME];
		struct addrinfo *loc = NULL;
		struct addrinfo *rem = NULL;
		struct addrinfo hints = {
			.ai_family = AF_UNSPEC,
			.ai_socktype = SOCK_DGRAM
		};
		/*Mandatory: Bearer logical name for UDP media*/
		data = json_object_get(b, "name");
		if (!data)
			return -1;
		bc.name = json_string_value(data);
		snprintf(id, sizeof(id), "%s:%s", bc.media, bc.name);
		mnl_attr_put_strz(nlh, TIPC_NLA_BEARER_NAME, id);

		/*Mandatory: Bearer local ip for UDP media*/
		data = json_object_get(b, "localip");
		if (!data)
			return -1;
		bc.localip = json_string_value(data);
		/*Optional: Local port for UDP media*/
		data = json_object_get(b, "localport");
		if (data)
			bc.localport = json_string_value(data);
		else
			bc.localport = "6118";

		if ((err = getaddrinfo(bc.localip, bc.localport, &hints, &loc))) {
			fprintf(stderr, "UDP local address error %s\n",
				gai_strerror(err));
			return -1;
		}

		/*Optional: Remote IP for UDP media*/
		data = json_object_get(b, "remoteip");
		if (data) {
			bc.remoteip = json_string_value(data);
		} else {
			if (generate_multicast(loc->ai_family, addrbuf, sizeof(addrbuf))) {
				fprintf(stderr, "Failed to generate multicast address\n");
				return -1;
			}
			bc.remoteip = addrbuf;
		}
		/*Optional: Remote port for UDP media*/
		data = json_object_get(b, "remoteport");
		if (data)
			bc.remoteport = json_string_value(data);
		else
			bc.remoteport = "6118";
		if ((err = getaddrinfo(bc.remoteip, bc.remoteport, &hints, &rem))) {
			fprintf(stderr, "UDP remote address error %s\n",
				gai_strerror(err));
			return -1;
		}
		if (rem->ai_family != loc->ai_family) {
			fprintf(stderr, "UDP local and remote AF mismatch\n");
			return -1;
		}
		nest_udp = mnl_attr_nest_start(nlh, TIPC_NLA_BEARER_UDP_OPTS);
		mnl_attr_put(nlh, TIPC_NLA_UDP_LOCAL, loc->ai_addrlen, loc->ai_addr);
		mnl_attr_put(nlh, TIPC_NLA_UDP_REMOTE, rem->ai_addrlen, rem->ai_addr);
		mnl_attr_nest_end(nlh, nest_udp);
		freeaddrinfo(rem);
		freeaddrinfo(loc);
	}
	mnl_attr_nest_end(nlh, nest);
	return msg_doit(nlh, NULL, NULL);
}

static int configure_tipc_bearers(json_t *bearers)
{
	int i;
	int res = 0;

	if (!json_is_array(bearers))
		return -1;
	for (i = 0; i < json_array_size(bearers); i++)
	{
		if (do_one_bearer(json_array_get(bearers, i))) {
			fprintf(stderr, "Failed to configure bearer\n");
			res = -1;
		}
	}
	return res;
}

static int parse_config(char *txt)
{
	int res = 0;
	json_t *root;
	json_error_t error;

	root = json_loads(txt, 0, &error);
	if (!root) {
		fprintf(stderr, "JSON error on line %d: %s\n",
			error.line, error.text);
		res = -1;
		goto err;
	}
	if (configure_tipc_address(json_object_get(root, "address"))) {
		res = -1;
		goto err;
	}
	if (configure_tipc_bearers(json_object_get(root, "bearers"))) {
		res = -1;
		goto err;
	}

err:
	json_decref(root);
	return res;
}

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
	if (parse_config(response))
		fprintf(stderr, "Failed to parse one or more config items\n");
	free(response);
	return 0;
}
