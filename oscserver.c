/*
 * Copyright (c) 2016 Christian Franke <nobody@nowhere.ws>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "cosc.h"
#include "oscserver.h"
#include "oscdispatcher.h"
#include "oscparser.h"
#include "oscutils.h"

struct osc_server {
	int fd;
	bool blocking;
	struct osc_dispatcher *dispatcher;
};

struct osc_server *osc_server_new(const char *node, const char *service,
                                  const struct addrinfo *hints)
{
	struct addrinfo ai = {
		.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG,
		.ai_family = AF_UNSPEC,
	};

	if (hints)
		memcpy(&ai, hints, sizeof(ai));

	ai.ai_flags |= AI_PASSIVE;
	ai.ai_socktype = SOCK_DGRAM;

	struct addrinfo *res, *rp;
	if (getaddrinfo(node, service, &ai, &res))
		return NULL;

	int fd;
	for (rp = res; rp; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (fd < 0)
			continue;

		int on = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		if (bind(fd, rp->ai_addr, rp->ai_addrlen) != 0) {
			close(fd);
			continue;
		}

		break;
	}

	freeaddrinfo(res);

	if (!rp)
		return NULL;

	struct osc_server *rv = calloc(sizeof(*rv), 1);

	rv->fd = fd;
	rv->blocking = true;
	rv->dispatcher = osc_dispatcher_new();
	return rv;
}

void osc_server_add_method(struct osc_server *server, const char *address,
                           osc_method callback, void *arg)
{
	osc_dispatcher_add_method(server->dispatcher, address, callback, arg);
}

int osc_server_set_blocking(struct osc_server *server, bool blocking)
{
	int flags = fcntl(server->fd, F_GETFL);

	if (flags < 0)
		return -1;

	if (blocking)
		flags &= ~(O_NONBLOCK);
	else
		flags |= O_NONBLOCK;

	if (fcntl(server->fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

int osc_server_run(struct osc_server *server)
{
	while (1) {
		char buf[8192];
		ssize_t bytes = recvfrom(server->fd, buf, sizeof(buf), 0, NULL, NULL);
		if (bytes < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				return 0;
			return 1;
		}

		char *log;
		struct osc_element *e = osc_parse_packet(buf, bytes, &log);
		if (!e) {
			fprintf(stderr, "Could not parse packet:<parser>\n%s<endparser>\n", log);
			free(log);
			continue;
		}

		free(log);
		osc_dispatcher_process(server->dispatcher, e);
		osc_free(e);
	}
}
