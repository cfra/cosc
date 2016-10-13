/*
 * Copyright (c) 2016 Christian Franke <nobody@nowhere.ws>
 *
 * This is a small example how cosc can be used. It runs an OSC server
 * which controls a moodlamp running http://git.sublab.org/remoodlamp/
 * via serial.
 *
 * The mapping of osc paths to the moodlamp is such that the Lemur
 * "Nexus 7 - Studio Combo" example can be used to control it in a very
 * basic fashion.
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
#define _GNU_SOURCE 1

#include <termios.h>
#include <unistd.h>

#include <cosc/cosc.h>
#include <cosc/oscserver.h>
#include <cosc/oscparser.h>

struct moodlamp {
	int fd;
	int color[3];
};

struct moodlamp_setter {
	struct moodlamp *m;
	int which;
};

static void moodlamp_update(struct moodlamp *m)
{
	char buf[512];

	snprintf(buf, sizeof(buf), "i%02x%02x%02x\r\n",
	         m->color[0], m->color[1], m->color[2]);

	for (char *c = buf; *c; c++) {
		write(m->fd, c, 1);
		/* This delay is to reduce bit-errors due to baud rate
		 * mismatch - stupid 16MHz crystal ;/ */
		usleep(1111);
	}
}

static struct moodlamp *moodlamp_new(const char *path)
{
	int fd = open(path, O_NOCTTY|O_RDWR);
	if (fd < 0)
		return NULL;

	struct termios tio;

	if (tcgetattr(fd, &tio))
		goto out;

	cfmakeraw(&tio);
	if (cfsetspeed(&tio, B115200))
		goto out;

	if (tcsetattr(fd, TCSANOW, &tio))
		goto out;

	struct moodlamp *rv = calloc(sizeof(*rv), 1);
	rv->fd = fd;
	rv->color[0] = 10;
	rv->color[1] = 0;
	rv->color[2] = 0;
	moodlamp_update(rv);
	return rv;

out:
	close(fd);
	return NULL;
}

static void moodlamp_set(void *arg, struct osc_element *e)
{
	struct moodlamp_setter *ms = arg;

	if (!e || e->type != OSC_FLOAT32)
		return;

	int color = 255.0 * ((struct osc_float32*)e)->value;
	if (color < 0)
		color = 0;
	if (color > 255)
		color = 255;

	ms->m->color[ms->which] = color;
	moodlamp_update(ms->m);
}


int main(int argc, char **argv)
{
	struct osc_server *s = osc_server_new(NULL, "4223", NULL);
	if (!s) {
		fprintf(stderr, "Could not create osc server.\n");
		return 1;
	}

	struct moodlamp *m = moodlamp_new("/dev/ttyUSB0");
	if (!m) {
		fprintf(stderr, "Could not open moodlamp.\n");
		return 1;
	}

	struct moodlamp_setter ms_r = { .m = m, .which = 0 };
	struct moodlamp_setter ms_g = { .m = m, .which = 1 };
	struct moodlamp_setter ms_b = { .m = m, .which = 2 };

	osc_server_add_method(s, "/Fader1/x", moodlamp_set, &ms_r);
	osc_server_add_method(s, "/Fader2/x", moodlamp_set, &ms_g);
	osc_server_add_method(s, "/Fader3/x", moodlamp_set, &ms_b);

	osc_server_run(s);
	return 1;
}
