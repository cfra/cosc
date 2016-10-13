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
#include "../oscparser.h"

#include "oscparser_tests.h"

static int test(FILE *input, FILE *output)
{
	unsigned char buf[1024];
	size_t pos = 0;
	size_t bytes;

	while (!feof(input)) {
		bytes = fread(&buf[pos], 1, sizeof(buf) - pos, input);
		if (!bytes)
			break;
		pos += bytes;
		if (pos == sizeof(buf)) { /* Overflow */
			fclose(input);
			return 1;
		}
	}

	fclose(input);

	char *log;
	struct osc_element *e = osc_parse_packet(buf, pos, &log);
	const char *formatted = osc_format(e);
	osc_free(e);

	fprintf(output, "Parser log:\n%s", log);
	fprintf(output, "Parser out:\n%s", formatted);

	return 0;
}
