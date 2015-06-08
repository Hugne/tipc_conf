#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include <avahi-core/core.h>
#include <avahi-core/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>


#include "resolver.h"

struct entry {
	struct AvahiServer *srv;
	struct AvahiSimplePoll *poll;
	const char *name;
	struct sockaddr_storage *ss;
	int resolved;
};


static void avahi_address_to_sockaddr(const AvahiAddress *address,
				      const uint16_t port,
				      const AvahiIfIndex index_,
				      struct sockaddr *sa) {
	switch (address->proto) {
	case AVAHI_PROTO_INET: {
		struct sockaddr_in *s4 = (struct sockaddr_in *) sa;
		s4->sin_family = AF_INET;
		s4->sin_addr.s_addr = address->data.ipv4.address;
		s4->sin_port = htons(port);
		break;
		}
	case AVAHI_PROTO_INET6: {
		struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) sa;
		s6->sin6_family = AF_INET6;
		memcpy(s6->sin6_addr.s6_addr, address->data.ipv6.address, 16);
		s6->sin6_flowinfo = 0;
		s6->sin6_scope_id = index_;
		s6->sin6_port = htons(port);
		break;
		}
	}
}

static void resolve_callback(AvahiSServiceResolver *r,
		AVAHI_GCC_UNUSED AvahiIfIndex interface,
		AVAHI_GCC_UNUSED AvahiProtocol protocol,
		AvahiResolverEvent event, const char *name,
		const char *type, const char *domain,
		const char *host_name, const AvahiAddress *address,
		uint16_t port, AvahiStringList *txt,
		AvahiLookupResultFlags flags,
		AVAHI_GCC_UNUSED void* userdata) {


	struct entry *e = userdata;

	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", 
			name, type, domain,
			avahi_strerror(avahi_server_errno(e->srv)));
		break;
	case AVAHI_RESOLVER_FOUND: {
		char a[AVAHI_ADDRESS_STR_MAX], *t;
		fprintf(stderr, "(Resolver) Service '%s' of type '%s' in domain '%s':\n",
			name, type, domain);
		avahi_address_snprint(a, sizeof(a), address);
		t = avahi_string_list_to_string(txt);
		fprintf(stderr, "\t%s:%u (%s)\n"
				"\tTXT=%s\n"
				"\tcookie is %u\n"
				"\tis_local: %i\n"
				"\twide_area: %i\n"
				"\tmulticast: %i\n"
				"\tcached: %i\n",
				host_name, port, a,
				t,
				avahi_string_list_get_service_cookie(txt),
				!!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
				!!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
				!!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
				!!(flags & AVAHI_LOOKUP_RESULT_CACHED));
				avahi_free(t);
		avahi_address_to_sockaddr(address, port, interface,
					  (struct sockaddr*)e->ss);
		e->resolved = 1;
		avahi_simple_poll_quit(e->poll);
		}
	}
}

static void resolve_timeout(AVAHI_GCC_UNUSED AvahiTimeout *timeout,
			    AVAHI_GCC_UNUSED void *userdata)
{
	struct entry *e = userdata;

	fprintf(stderr, "Timed out waiting to resolve %s\n",
		e->name);
	avahi_simple_poll_quit(e->poll);
}

int resolve(const char *name, struct sockaddr_storage *ss)
{
	int err;
	AvahiServerConfig sc;
	AvahiServer *server;
	AvahiSimplePoll *poll;
	struct timeval tv;
	AvahiSServiceResolver *resolver;
	struct entry e = {
		.name = name,
		.ss = ss
	};

	avahi_server_config_init(&sc);
	sc.publish_hinfo = 0;
	sc.publish_addresses = 0;
	sc.publish_workstation = 0;
	sc.publish_domain = 0;

	poll = avahi_simple_poll_new();
	e.poll = poll;
	avahi_elapse_time(&tv, 5000, 0);
	avahi_simple_poll_get(poll)->timeout_new(avahi_simple_poll_get(poll),
						 &tv, resolve_timeout, &e);

	server = avahi_server_new(avahi_simple_poll_get(poll), NULL, NULL, NULL , &err);
	if (!server) {
		fprintf(stderr, "Failed to create server: %s\n", avahi_strerror(err));
		return -1;
	}
	e.srv = server;
	resolver = avahi_s_service_resolver_new(e.srv, AVAHI_IF_UNSPEC,
					   AVAHI_PROTO_INET, e.name,
					   "_http._tcp", NULL, AVAHI_PROTO_INET,
					   0, resolve_callback, &e);
	if (!resolver) {
		fprintf(stderr, "Failed to resolve service '%s': %s\n",
			e.name, avahi_strerror(avahi_server_errno(e.srv)));
		goto out_err;
	}
	/*Wait until resolved*/
	avahi_simple_poll_loop(poll);
out_err:
	avahi_server_config_free(&sc);
	avahi_s_service_resolver_free(resolver);
	avahi_server_free(server);
	avahi_simple_poll_free(poll);
	return e.resolved;

}
