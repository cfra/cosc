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
#include "oscdispatcher.h"
#include "oscparser.h"
#include "oscutils.h"

enum osc_node_type {
	OSC_CONTAINER,
	OSC_METHOD
};

struct osc_node;

#define OSC_NODE_COMMON \
	enum osc_node_type type; \
	char *name; \
	struct osc_node *next; \

struct osc_node {
	OSC_NODE_COMMON
};

struct osc_container {
	OSC_NODE_COMMON
	struct osc_node *children;
	struct osc_node **endp;
};

struct osc_method {
	OSC_NODE_COMMON
	osc_method callback;
};


struct osc_dispatcher {
	struct osc_container *root;
};

static struct osc_container *osc_container_new(const char *name)
{
	struct osc_container *rv;

	rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_CONTAINER;
	rv->name = strdup(name);
	rv->endp = &rv->children;

	return rv;
};

static struct osc_method *osc_method_new(const char *name, osc_method callback)
{
	struct osc_method *rv;

	rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_METHOD;
	rv->name = strdup(name);
	rv->callback = callback;

	return rv;
}

struct osc_dispatcher *osc_dispatcher_new(void)
{
	struct osc_dispatcher *rv;

	rv = calloc(sizeof(*rv), 1);
	rv->root = osc_container_new("");

	return rv;
}

void osc_dispatcher_add_method(struct osc_dispatcher *d, const char *address,
                               osc_method callback)
{
	size_t slashes = 0;

	for (const char *p = address; *p; p++) {
		if (*p == '/')
			slashes++;
	}

	size_t token_count;
	char **tokens = osc_addr_split(address, &token_count);

	if (token_count < 1)
		return;

	struct osc_container *c = d->root;

	for (size_t i = 0; i < token_count; i++) {
		char *token = tokens[i];
		struct osc_node *n;

		/* Search in container if node exists */
		for (n = c->children; n; n = n->next) {
			if (!strcmp(n->name, token))
				break;
		}

		if (!n) {
			/* Node was not found... */
			if (i < token_count - 1) {
				/* it's not the last token, so create container */
				n = (struct osc_node*)osc_container_new(token);
			} else { /* it is the last token, so create method */
				n = (struct osc_node*)osc_method_new(token, callback);
			}
			/* Insert created node */
			*c->endp = n;
			c->endp = &n->next;
		} else {
			if (i >= token_count - 1 || n->type != OSC_CONTAINER) {
				/* found a node that is not suitable to install the method to */
				goto out;
			}
		}

		if (n->type == OSC_CONTAINER)
			c = (struct osc_container *)n;
	}

out:
	for (size_t i = 0; i < token_count; i++)
		free(tokens[i]);
	free(tokens);
}

static void osc_dispatcher_process_message(struct osc_dispatcher *d, struct osc_message *msg)
{
	size_t token_count;
	char **tokens = osc_addr_split(msg->address->value, &token_count);

	struct osc_container *c = d->root;
	struct osc_method *m = NULL;

	for (size_t i = 0; i < token_count; i++) {
		char *token = tokens[i];
		struct osc_node *n;

		for (n = c->children; n; n = n->next) {
			if (!strcmp(n->name, token))
				break;
		}

		if (!n)
			return;
		if (i < token_count - 1) {
			if (n->type != OSC_CONTAINER)
				goto out;
			c = (struct osc_container *)n;
		} else {
			if (n->type != OSC_METHOD)
				goto out;
			m = (struct osc_method *)n;
		}
	}

	if (!m)
		goto out;

	m->callback(msg->arguments);

out:
	for (size_t i = 0; i < token_count; i++)
		free(tokens[i]);
	free(tokens);
}

void osc_dispatcher_process(struct osc_dispatcher *d, struct osc_element *e)
{
	if (e->type == OSC_MESSAGE) {
		osc_dispatcher_process_message(d, (struct osc_message *)e);
	}
	if (e->type == OSC_BUNDLE) {
		struct osc_bundle *b = (struct osc_bundle*)e;

		if (!b->timetag->immediately)
			return;

		for (struct osc_element *i = b->elements; i; i = i->next)
			osc_dispatcher_process(d, i);
	}
}
