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

struct osc_formatter_state {
	char *pos;
	char *end;
};

struct osc_parser_state {
	const unsigned char *ptr;
	size_t len;
	struct osc_formatter_state f;
};

static void osc_format_print(struct osc_formatter_state *s, unsigned indent, const char *fmt, ...)
{
	va_list ap;
	char buf[4096];

	if (!s->pos)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	snprintf(s->pos, s->end - s->pos, "%*s%s", indent, "", buf);
	s->pos += strlen(s->pos);
}


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

	osc_format_print(&s->f, 0, "Parsing int32...\n");
	if (s->len < 4) {
		osc_format_print(&s->f, 0, "Not enough data available.\n");
		return NULL;
	}

	memcpy(&tmp, s->ptr, 4);
	s->ptr += 4;
	s->len -= 4;

	rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_INT32;
	rv->value = ntohl(tmp);
	return rv;
}

static struct osc_float32 *osc_parse_float32(struct osc_parser_state *s)
{
	osc_format_print(&s->f, 0, "Parsing float32 not implemented.\n");
	return NULL; /* TODO */
}

static struct osc_string *osc_parse_string(struct osc_parser_state *s)
{
	size_t len = 0;
	struct osc_string *rv = NULL;

	osc_format_print(&s->f, 0, "Parsing string...\n");

	while (len < s->len) {
		if (*(s->ptr + len) == '\0') {
			rv = calloc(sizeof(*rv), 1);
			rv->type = OSC_STRING;
			break;
		}
		len++;
	}

	if (!rv) { /* No termination found */
		osc_format_print(&s->f, 0, "Could not find string terminator.\n");
		return NULL;
	}

	rv->value = strdup((char*)s->ptr);

	len += 1; /* Account for terminator byte. */
	size_t padded = len + ((4 - (len % 4)) % 4);
	if (padded > s->len)
		padded = s->len; /* Ignore missing padding at end of packet */

	s->ptr += padded;
	s->len -= padded;

	return rv;
}

static struct osc_blob *osc_parse_blob(struct osc_parser_state *s)
{
	osc_format_print(&s->f, 0, "Parsing blob not implemented.\n");
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
		osc_format_print(&s->f, 0, "Don't know about type '%c'\n", type);
		return NULL;
	}
}

static struct osc_message *osc_parse_message(struct osc_parser_state *s, struct osc_string *addr)
{
	struct osc_string *types = NULL;
	struct osc_message *rv = NULL;

	if (!addr) {
		osc_format_print(&s->f, 0, "Parsing address...!\n");
		addr = osc_parse_string(s);
		if (!addr)
			return NULL;
	}

	if (addr->value[0] != '/') {
		osc_format_print(&s->f, 0, "Message does not start with valid address!\n");
		goto out;
	}

	rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_MESSAGE;
	rv->address = addr;

	if (!s->len) /* Empty Tag string -> no arguments */
		return rv;

	types = osc_parse_string(s);
	if (!types || types->value[0] != ',') {
		osc_format_print(&s->f, 0, "Message does not contain correct type tag string!\n");
		if (types)
			osc_format_print(&s->f, 2, "Tag types string: \"%s\"\n", types->value);
		else
			osc_format_print(&s->f, 2, "Tag types string could not be parsed.\n");

		goto out;
	}

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

static struct osc_timetag *osc_parse_timetag(struct osc_parser_state *s)
{
	struct osc_timetag *rv = NULL;
	uint64_t seconds, fraction;
	uint32_t tmp;

	osc_format_print(&s->f, 0, "Parsing timetag...\n");

	if (s->len < 8) {
		osc_format_print(&s->f, 0, "Not enough data available.\n");
		return NULL;
	}

	rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_TIMETAG;

	memcpy(&tmp, s->ptr, 4);
	seconds = ntohl(tmp);
	s->ptr += 4;
	memcpy(&tmp, s->ptr, 4);
	fraction = ntohl(tmp);
	s->ptr += 4;
	s->len -= 8;

	tmp = be64toh(tmp);

	if (seconds == 0 && fraction == 1) {
		rv->immediately = true;
	} else {
		rv->immediately = false;
		seconds -= 2208988800ULL;
		fraction *= 1000000000ULL;
		fraction >>= 32;

		rv->value.tv_sec = seconds;
		rv->value.tv_nsec = fraction;
	}

	return rv;
}

static struct osc_bundle *osc_parse_bundle(struct osc_parser_state *s)
{
	struct osc_timetag *tag = osc_parse_timetag(s);
	if (!tag)
		return NULL;

	struct osc_bundle *rv = calloc(sizeof(*rv), 1);
	rv->type = OSC_BUNDLE;
	rv->timetag = tag;

	struct osc_element **atnext = &rv->elements;

	while (s->len) {
		osc_format_print(&s->f, 0, "Parsing bundle element...\n");
		struct osc_int32 *i = osc_parse_int32(s);
		if (!i)
			goto out;

		if ((size_t)i->value > s->len) {
			osc_format_print(&s->f, 0, "Bundle element size too large.\n");
			osc_free(i);
			goto out;
		}

		char *log;
		*atnext = osc_parse_packet(s->ptr, i->value, &log);
		s->ptr += i->value;
		s->len -= i->value;

		osc_format_print(&s->f, 0, "<subparser>\n%s</subparser>\n", log);
		free(log);

		if (!*atnext)
			goto out;
		atnext = &(*atnext)->next;
		osc_free(i);
	}

	return rv;
out:
	if (rv)
		osc_free(rv);
	else
		osc_free(tag);

	return NULL;
}

struct osc_element *osc_parse_packet(const void *data, size_t len, char **log)
{
	char *logbuf;
	struct osc_parser_state s = {
		.ptr = data,
		.len = len,
	};

	if (log) {
		logbuf = malloc(8192);
		logbuf[0] = '\0';
		*log = logbuf;
		s.f.pos = logbuf;
		s.f.end = logbuf + 8192;
	}

	/* Packet is either a bundle or a message, both start with a string */
	struct osc_string *head = osc_parse_string(&s);

	if (!head) {
		osc_format_print(&s.f, 0, "Could not find start string of message or bundle.\n");
		return NULL;
	}

	if (!strcmp(head->value, "#bundle")) {
		osc_free(head);
		osc_format_print(&s.f, 0, "Found bundle, parsing...\n");
		return (struct osc_element*)osc_parse_bundle(&s);
	}

	osc_format_print(&s.f, 0, "Found messsage, parsing...\n");
	return (struct osc_element*)osc_parse_message(&s, head);
}

static void _osc_format(struct osc_formatter_state *s, unsigned indent,
                        union osc_element_ptr ptr);

static void osc_format_blob(struct osc_formatter_state *s, unsigned indent,
                            struct osc_blob *b)
{
	osc_format_print(s, indent, "OSC_BLOB: ...\n"); /* TODO */
}

static void osc_format_timetag(struct osc_formatter_state *s, unsigned indent,
                               struct osc_timetag *t)
{
	if (t->immediately) {
		osc_format_print(s, indent, "OSC_TIMETAG: immediately\n");
	} else {
		osc_format_print(s, indent, "OSC_TIMETAG: %jd sec %jd nsec\n",
		                 (intmax_t)t->value.tv_sec, (intmax_t)t->value.tv_nsec);
	}
}

static void osc_format_message(struct osc_formatter_state *s, unsigned indent,
                               struct osc_message *msg)
{
	osc_format_print(s, indent, "OSC_MESSAGE:\n");
	osc_format_print(s, indent, "  Address: \"%s\"\n", msg->address->value);
	osc_format_print(s, indent, "  Arguments:\n");

	for (struct osc_element *e = msg->arguments; e; e = e->next)
		_osc_format(s, indent + 4, e);
}

static void osc_format_bundle(struct osc_formatter_state *s, unsigned indent,
                              struct osc_bundle *b)
{
	osc_format_print(s, indent, "OSC_BUNDLE:\n");
	osc_format_timetag(s, indent + 2, b->timetag);
	osc_format_print(s, indent, "  Elements:\n");

	for (struct osc_element *e = b->elements; e; e = e->next)
		_osc_format(s, indent + 4, e);
}

static void _osc_format(struct osc_formatter_state *s, unsigned indent,
                        union osc_element_ptr ptr)
{
	struct osc_element *e = ptr.element;

	if (!e)
		return;

	switch (e->type) {
	case OSC_UNDEFINED:
		osc_format_print(s, indent, "OSC_UNDEFINED\n");
		break;
	case OSC_MESSAGE:
		osc_format_message(s, indent, ptr.message);
		break;
	case OSC_BUNDLE:
		osc_format_bundle(s, indent, ptr.bundle);
		break;
	case OSC_ELEMENT:
		osc_format_print(s, indent, "OSC_ELEMENT\n");
		break;
	case OSC_INT32:
		osc_format_print(s, indent, "OSC_INT32: %d\n", ptr.int32->value);
		break;
	case OSC_TIMETAG:
		osc_format_timetag(s, indent, ptr.timetag);
		break;
	case OSC_FLOAT32:
		osc_format_print(s, indent, "OSC_FLOAT32: %f\n", ptr.float32->value);
		break;
	case OSC_STRING:
		osc_format_print(s, indent, "OSC_STRING: \"%s\"\n", ptr.string->value);
		break;
	case OSC_BLOB:
		osc_format_blob(s, indent, ptr.blob);
		break;
	}
}

const char *osc_format(union osc_element_ptr ptr)
{
	static char buf[8192];

	struct osc_formatter_state s = {
		.pos = buf,
		.end = buf + sizeof(buf)
	};
	buf[0] = '\0';

	_osc_format(&s, 0, ptr);
	return buf;
}
