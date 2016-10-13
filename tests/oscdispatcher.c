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
#include "../cosc.h"
#include "../oscdispatcher.h"
#include "../oscparser.h"

static void callback_foo_bar(struct osc_element *e) {
	printf("foo_bar called.\n");
}

static void callback_foo_baz(struct osc_element *e) {
	printf("foo_baz called.\n");
}
static void callback_foo2_bar(struct osc_element *e) {
	printf("foo2_bar called.\n");
}

int main(int argc, char **argv)
{
	struct osc_dispatcher *d = osc_dispatcher_new();

	osc_dispatcher_add_method(d, "/foo/bar", callback_foo_bar);
	osc_dispatcher_add_method(d, "/foo/baz", callback_foo_baz);
	osc_dispatcher_add_method(d, "/foo2/bar", callback_foo2_bar);

	struct osc_string addr = {
		.type = OSC_STRING,
		.value = "/foo/bar"
	};

	struct osc_message m = {
		.type = OSC_MESSAGE,
		.address = &addr
	};

	printf("Calling /foo/bar\n");
	osc_dispatcher_process(d, (struct osc_element*)&m);

	addr.value = "/foo/baz";
	printf("Calling /foo/baz\n");
	osc_dispatcher_process(d, (struct osc_element*)&m);

	addr.value = "/foo/qux";
	printf("Calling /foo/qux\n");
	osc_dispatcher_process(d, (struct osc_element*)&m);

	addr.value = "/foo2/bar";
	printf("Calling /foo2/bar\n");
	osc_dispatcher_process(d, (struct osc_element*)&m);

	struct osc_timetag t = {
		.immediately = true
	};

	struct osc_bundle b = {
		.type = OSC_BUNDLE,
		.timetag = &t,
		.elements = (struct osc_element*)&m
	};

	printf("Sending message in bundle\n");
	osc_dispatcher_process(d, (struct osc_element*)&b);

	t.immediately = false;
	printf("Trying bundle w/o immediate delivery\n");
	osc_dispatcher_process(d, (struct osc_element*)&b);

	printf("Done.\n");

	return 0;
}
