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
#include "../oscserver.h"
#include "../oscparser.h"

void fader_callback(void *arg, struct osc_element *arguments)
{
	const char *name = arg;
	if (!arguments || arguments->type != OSC_FLOAT32) {
		printf("%s called with invalid argument:\n%s", name, osc_format(arguments));
		return;
	}

	struct osc_float32 *f = (struct osc_float32*)arguments;

	printf("%s set to %f\n", name, f->value);
}

int main(int argc, char **argv)
{
	struct osc_server *server = osc_server_new(NULL, "4223", NULL);
	if (!server) {
		fprintf(stderr, "Could not create server.\n");
		return 1;
	}

	osc_server_add_method(server, "/Fader1/x", fader_callback, "Fader1");
	osc_server_run(server);
	return 1;
}
