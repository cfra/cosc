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
#include "oscutils.h"

char **osc_addr_split(const char *address, size_t *count)
{
	*count = 0;

	for (const char *i = address + 1; *i; i++) {
		if (*i == '/')
			(*count)++;
	}

	char **rv = calloc((*count) + 2, sizeof(*rv));

	const char *start = address + 1;
	const char *i = start;
	*count = 0;

	while (1) {
		if (*i != '/' && *i != '\0') {
			i++;
			continue;
		}

		rv[*count] = malloc(i - start + 1);
		memcpy(rv[*count], start, i-start);
		rv[*count][i-start] = '\0';
		(*count)++;
		if (*i == '\0')
			break;
		i++;
		start = i;
	}

	return rv;
}
