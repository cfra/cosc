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

bool osc_pattern_match(const char *pattern, const char *token)
{
	/* Matching works by examining the pattern and consuming from the
	 * token accordingly. Depending on the pattern, we need to follow
	 * multiple branches.
	 * This recursive implementation is horribly inefficient, but
	 * let's try to get it working first, shall we? */

top:
	/* If pattern is "shorter" than the token, then there is no match */
	if (!*pattern && *token)
		return false;

	/* If both pattern and token are depleted, there is a match */
	if (!*pattern && !*token)
		return true;

	/* 1. '?' matches any single character */
	if (*pattern == '?') {
		if (!*token)
			return false; /* Token is depleted, but pattern expects content -> no match */
		/* Skip one character and reevaluate match */
		pattern++;
		token++;
		goto top;
	}

	/* 2. '*' matches any sequence of zero or more characters */
	if (*pattern == '*') {
		/* Check if the rest of the pattern matches the token with any number of characters skipped
		 * The <= is not an off-by-one - we want to run for empty string too */
		for (size_t i = 0; i <= strlen(token); i++) {
			if (osc_pattern_match(pattern+1,token + i))
				return true;
		}
		return false;
	}

	/* And now for the tricky bit */

	/* 3. A string of characters in square brackets e.g. [string] matches any character in the string.
	 * Inside square brackets, the '-' character and '!' have special meanings:
	 *
	 * Two characters separated by '-' indicate the range of characters between the given two in ASCII
	 * '-' at and of string has no meaning.
	 *
	 * '!' at the begging means the match is inverted, otherwise, it has no meaning.
	 */
	if (*pattern == '[') {
		/* If token is at its end, there can't be a match */
		if (!*token)
			return false;

		const char *end = strchr(pattern, ']');
		if (!end)
			return false; /* The pattern is flawed, return no-match */

		char acceptable[512];
		char *ap = acceptable;

		*ap = '\0';
		pattern++;

		bool invert_match = false;
		if (*pattern == '!') {
			invert_match = true;
			pattern++;
		}

#define append_acceptable(c) \
do { \
	if ((size_t)(ap - acceptable) < sizeof(acceptable)) { \
		*ap = c; \
		ap++; \
		*ap = '\0'; \
	} else { \
		/* If somebody screws with the pattern so badly that it exceeds 512 ascii chars, \
		 * assume it's broken and return no-match */ \
		return false; \
	} \
} while (0)

		bool saw_minus = false;
		for (; *pattern != ']'; pattern++) {
			if (*pattern == '-') {
				saw_minus = true;
				continue;
			}
			if (saw_minus) {
				saw_minus = false;
				if (ap != acceptable) {
					/* If there was something before the '-' */
					char start = *(ap-1) + 1;
					if (start <= *pattern) {
						for (char i = start; i <= *pattern; i++)
							append_acceptable(i);
					} else {
						append_acceptable(*pattern);
					}
				} else {
					/* Else, just treat it as '-' */
					append_acceptable('-');
				}
				continue;
			}
			append_acceptable(*pattern);
		}
		if (saw_minus)
			append_acceptable('-');

#undef append_acceptable

		if (strchr(acceptable, *token)) {
			if (invert_match)
				return false;
		} else {
			if (!invert_match)
				return false;
		}

		pattern++;
		token++;
		goto top;
	}

	/* 4. A comma-separated list of strings enclosed in curly braces matches any of the strings
	 * in the list. */
	if (*pattern == '{') {
		const char *end = strchr(pattern, '}');

		if (!end)
			return false; /* Pattern is broken if there is no closing brace */

		char *commalist = malloc(end - pattern);
		memcpy(commalist, pattern+1, end - pattern - 1);
		commalist[end - pattern - 1] = '\0';
		pattern = end + 1;

		char *saveptr;
		for (char *input = commalist; ; input = NULL) {
			char *word = strtok_r(input, ",", &saveptr);
			if (!word)
				break;

			/* If the word does not match, continue */
			if (strncmp(word, token, strlen(word)))
				continue;

			if (osc_pattern_match(pattern, token + strlen(word))) {
				/* The rest matches, so we have a match */
				free(commalist);
				return true;
			}
			/* The word matches, but the rest did not, continue
			 * testing other words. (They might share the same prefix)
			 */
		}

		free(commalist);
		return false; /* None of the words matches */
	}

	/* 5. Any other character in an OSC Address Pattern can match only the same character */
	if (*pattern != *token)
		return false;
	pattern++;
	token++;
	goto top;
}
