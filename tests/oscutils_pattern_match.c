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
#include "../oscutils.h"

#include "oscutils_pattern_match_tests.h"

static int test(FILE *input, FILE *output)
{
	char buf[1024], buf2[1024];

	if (!fgets(buf, sizeof(buf), input))
		return 1;
	if (!fgets(buf2, sizeof(buf2), input))
		return 1;
	fclose(input);

	size_t len = strlen(buf);
	size_t len2 = strlen(buf2);

	if (len && buf[len-1] == '\n')
		buf[len-1] = '\0';
	
	if (len2 && buf2[len2-1] == '\n')
		buf2[len2-1] = '\0';

	fprintf(output, "Read pattern: \"%s\"\n", buf);
	fprintf(output, "Read token: \"%s\"\n", buf2);

	if (osc_pattern_match(buf, buf2)) {
		fprintf(output, "Token matches pattern.\n");
	} else {
		fprintf(output, "Token does NOT match pattern.\n");
	}

	return 0;
}
