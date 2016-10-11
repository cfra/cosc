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
#include "oscparser.h"

struct osc_parser_state {
	const unsigned char *ptr;
	size_t len;
};

static void osc_free_simple(struct osc_element *e)
{
	osc_free(e->next);
	free(e);
}

static void osc_free_message(struct osc_message *m)
{
	osc_free(m->address);
	osc_free(m->arguments);
	m->type = OSC_ELEMENT;
	osc_free(m);
}

static void osc_free_bundle(struct osc_bundle *b)
{
	osc_free(b->timetag);
	osc_free(b->elements);
	b->type = OSC_ELEMENT;
	osc_free(b);
}

static void osc_free_string(struct osc_string *s)
{
	free(s->value);
	s->type = OSC_ELEMENT;
	osc_free(s);
}

static void osc_free_blob(struct osc_blob *b)
{
	free(b->value);
	b->type = OSC_ELEMENT;
	osc_free(b);
}

void osc_free(union osc_element_ptr ptr)
{
	struct osc_element *e = ptr.element;

	if (!e)
		return;

	switch (e->type) {
	case OSC_UNDEFINED:
		assert(0);
		break;
	case OSC_MESSAGE:
		osc_free_message(ptr.message);
		break;
	case OSC_BUNDLE:
		osc_free_bundle(ptr.bundle);
		break;
	case OSC_ELEMENT:
	case OSC_INT32:
	case OSC_TIMETAG:
	case OSC_FLOAT32:
		osc_free_simple(e);
		break;
	case OSC_STRING:
		osc_free_string(ptr.string);
		break;
	case OSC_BLOB:
		osc_free_blob(ptr.blob);
		break;
	}
}

static struct osc_int32 *osc_parse_int32(struct osc_parser_state *s)
{
	struct osc_int32 *rv = NULL;
	int32_t tmp;

	if (s->len < 4)
		return NULL;

	memcpy(&tmp, s->ptr, 4);

	rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_INT32;
	rv->value = htonl(tmp);
	return rv;
}

static struct osc_float32 *osc_parse_float32(struct osc_parser_state *s)
{
	return NULL; /* TODO */
}

static struct osc_string *osc_parse_string(struct osc_parser_state *s)
{
	size_t len = 0;
	struct osc_string *rv = NULL;

	while (len < s->len) {
		if (*(s->ptr + len) == '\0') {
			rv = calloc(sizeof(*rv), 1);
			rv->type = OSC_STRING;
			break;
		}
		len++;
	}

	if (!rv) /* No termination found */
		return NULL;

	rv->value = strdup((char*)s->ptr);

	size_t padded = len + ((4 - (len % 4)) % 4);
	if (padded > s->len)
		padded = s->len; /* Ignore missing padding at end of packet */

	s->ptr += padded;
	s->len -= padded;

	return rv;
}

static struct osc_blob *osc_parse_blob(struct osc_parser_state *s)
{
	return NULL; /* TODO */
}

static struct osc_element *osc_parse_element(struct osc_parser_state *s, char type)
{
	switch (type) {
	case 'i':
		return (struct osc_element*)osc_parse_int32(s);
	case 'f':
		return (struct osc_element*)osc_parse_float32(s);
	case 's':
		return (struct osc_element*)osc_parse_string(s);
	case 'b':
		return (struct osc_element*)osc_parse_blob(s);
	default:
		return NULL;
	}
}

static struct osc_message *osc_parse_message(struct osc_parser_state *s, struct osc_string *addr)
{
	if (!addr) {
		addr = osc_parse_string(s);
		if (!addr)
			return NULL;
	}

	if (addr->value[0] != '/')
		goto out;

	struct osc_message *rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_MESSAGE;
	rv->address = addr;

	if (!s->len) /* Empty Tag string -> no arguments */
		return rv;

	struct osc_string *types = osc_parse_string(s);
	if (!types || types->value[0] != ',')
		goto out;

	struct osc_element **atnext = &rv->arguments;

	for (char *type = &types->value[1]; *type; type++) {
		*atnext = osc_parse_element(s, *type);
		if (!*atnext)
			goto out;
		atnext = &(*atnext)->next;
	}

	osc_free(types);
	return rv;

out:
	if (rv)
		osc_free(rv);
	else
		osc_free(addr);

	osc_free(types);
	return NULL;
}

struct osc_element *osc_parse_packet(const void *data, size_t len)
{
	struct osc_parser_state s = {
		.ptr = data,
		.len = len
	};

	/* Packet is either a bundle or a message, both start with a string */
	struct osc_string *head = osc_parse_string(&s);

	if (!head)
		return NULL;

	if (strcmp(head->value, "#bundle"))
		return (struct osc_element*)osc_parse_bundle(&s);

	return (struct osc_element*)osc_parse_message(&s, head);
}
