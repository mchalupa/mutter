/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/input.h>
#include "weston-test-client-helper.h"
#include "mutter-test-runner.h"

MUTTER_TEST(simple_button_test)
{
	struct client *client;
	struct pointer *pointer;

	client = client_create(100, 100, 100, 100);
	g_assert(client);

	pointer = client->input->pointer;

	g_assert(pointer->button == 0);
	g_assert(pointer->state == 0);

	move_pointer(client, 150, 150);
	g_assert(pointer->x == 50);
	g_assert(pointer->y == 50);

	send_button (client, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
	g_assert(pointer->button == BTN_LEFT);
	g_assert(pointer->state == WL_POINTER_BUTTON_STATE_PRESSED);

	send_button (client, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
	g_assert(pointer->button == BTN_LEFT);
	g_assert(pointer->state == WL_POINTER_BUTTON_STATE_RELEASED);
}
