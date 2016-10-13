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
 *
 * This code is based on the code in cpython which is originally distributed under
 * the following terms:
 *
 * Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 * 2011, 2012, 2013, 2014, 2015, 2016 Python Software Foundation; All Rights
 * Reserved
 *
 * PYTHON SOFTWARE FOUNDATION LICENSE VERSION 2
 * --------------------------------------------
 *
 * 1. This LICENSE AGREEMENT is between the Python Software Foundation
 * ("PSF"), and the Individual or Organization ("Licensee") accessing and
 * otherwise using this software ("Python") in source or binary form and
 * its associated documentation.
 *
 * 2. Subject to the terms and conditions of this License Agreement, PSF hereby
 * grants Licensee a nonexclusive, royalty-free, world-wide license to reproduce,
 * analyze, test, perform and/or display publicly, prepare derivative works,
 * distribute, and otherwise use Python alone or in any derivative version,
 * provided, however, that PSF's License Agreement and PSF's notice of copyright,
 * i.e., "Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 * 2011, 2012, 2013, 2014, 2015, 2016 Python Software Foundation; All Rights
 * Reserved" are retained in Python alone or in any derivative version prepared by
 * Licensee.
 *
 * 3. In the event Licensee prepares a derivative work that is based on
 * or incorporates Python or any part thereof, and wants to make
 * the derivative work available to others as provided herein, then
 * Licensee hereby agrees to include in any such work a brief summary of
 * the changes made to Python.
 *
 * 4. PSF is making Python available to Licensee on an "AS IS"
 * basis.  PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
 * IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND
 * DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
 * FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON WILL NOT
 * INFRINGE ANY THIRD PARTY RIGHTS.
 *
 * 5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON
 * FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS
 * A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON,
 * OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
 *
 * 6. This License Agreement will automatically terminate upon a material
 * breach of its terms and conditions.
 *
 * 7. Nothing in this License Agreement shall be deemed to create any
 * relationship of agency, partnership, or joint venture between PSF and
 * Licensee.  This License Agreement does not grant permission to use PSF
 * trademarks or trade name in a trademark sense to endorse or promote
 * products or services of Licensee, or any third party.
 *
 * 8. By copying, installing or otherwise using Python, Licensee
 * agrees to be bound by the terms and conditions of this License
 * Agreement.
 */

#include "cosc.h"
#include "oscfloat.h"

#include <math.h>

enum float_format_type {
	FLOAT_UNINITIALIZED,
	FLOAT_UNKNOWN,
	FLOAT_IEEE_BIG_ENDIAN,
	FLOAT_IEEE_LITTLE_ENDIAN
};

static enum float_format_type float_format = FLOAT_UNINITIALIZED;

static void get_float_format(void)
{
	float y = 16711938.0;

	if (float_format != FLOAT_UNINITIALIZED)
		return;

	if (sizeof(y) != 4)
		float_format = FLOAT_UNKNOWN;

	if (!memcmp(&y, "\x4b\x7f\x01\x02", 4))
		float_format = FLOAT_IEEE_BIG_ENDIAN;
	else if (!memcmp(&y, "\x02\x01\x7f\x4b", 4))
		float_format = FLOAT_IEEE_LITTLE_ENDIAN;
	else
		float_format = FLOAT_UNKNOWN;
}

float osc_unpack_float(const unsigned char *p)
{
	get_float_format();

	if (float_format == FLOAT_UNKNOWN) {
		unsigned char sign;
		int e;
		unsigned int f;
		double x;

		sign = (*p >> 7) & 1;
		e = (*p & 0x7F) << 1;
		p++;

		e |= (*p >> 7) & 1;
		f = (*p & 0x7F) << 16;
		p++;

		if (e == 255) {
			return NAN;
		}

		f |= *p << 8;
		p++;
		f |= *p;

		x = (double)f / 8388608.0;

		if (e == 0) {
			e = -126;
		} else {
			x += 1.0;
			e -= 127;
		}
		x = ldexp(x, e);

		if (sign)
			x = -x;

		return x;
	} else if (float_format == FLOAT_IEEE_BIG_ENDIAN) {
		float x;

		memcpy(&x, p, 4);
		return x;
	} else {
		float x;
		char buf[4];

		buf[0] = p[3];
		buf[1] = p[2];
		buf[2] = p[1];
		buf[3] = p[0];

		memcpy(&x, buf, 4);
		return x;
	}

	return NAN;
}
