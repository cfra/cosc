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
#ifndef OSCPARSER_H
#define OSCPARSER_H

enum osc_type {
	OSC_UNDEFINED = 0,
	OSC_ELEMENT,

	OSC_MESSAGE,
	OSC_BUNDLE,

	OSC_INT32,
	OSC_TIMETAG,
	OSC_FLOAT32,
	OSC_STRING,
	OSC_BLOB,
};

struct osc_element;

#define OSC_ELEMENT_COMMON \
	enum osc_type type; \
	struct osc_element *next;

struct osc_element {
	OSC_ELEMENT_COMMON
};

struct osc_message {
	OSC_ELEMENT_COMMON

	struct osc_string *address;
	struct osc_element *arguments;
};

struct osc_bundle {
	OSC_ELEMENT_COMMON

	struct osc_timetag *timetag;
	struct osc_element *elements;
};

struct osc_int32 {
	OSC_ELEMENT_COMMON

	int32_t value;
};

struct osc_timetag {
	OSC_ELEMENT_COMMON

	struct timespec value;
};

struct osc_float32 {
	OSC_ELEMENT_COMMON

	float value;
};

struct osc_string {
	OSC_ELEMENT_COMMON

	char *value;
};

struct osc_blob {
	OSC_ELEMENT_COMMON

	unsigned char *value;
	size_t size;
};

union osc_element_ptr {
	struct osc_element* element;
	struct osc_message* message;
	struct osc_bundle* bundle;
	struct osc_int32* int32;
	struct osc_timetag* timetag;
	struct osc_float32* float32;
	struct osc_string* string;
	struct osc_blob* blob;
} __attribute__((__transparent_union__));

void osc_free(union osc_element_ptr ptr);

#endif
