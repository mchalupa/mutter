/*
 * Copyright (c) 2014 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

#include <glib.h>
#include <wayland-client.h>
#include <linux/input.h>

#include "mutter-test-runner.h"
#include "weston-test-client-helper.h"

MUTTER_TEST (connect_tst)
{
  struct wl_display *d = wl_display_connect (NULL);
  g_assert (d);

  wl_display_disconnect (d);
}

MUTTER_TEST (get_registry)
{
  struct wl_display *d = wl_display_connect (NULL);
  g_assert (d);
  struct wl_registry *r = wl_display_get_registry (d);
  g_assert (r);

  wl_display_roundtrip (d);
  g_assert (wl_display_get_error (d) == 0);

  wl_display_disconnect (d);
}

static void
handle_global (void *data, struct wl_registry *registry,
               uint32_t id, const char *interface, uint32_t version)
{
  struct wl_test **test = data;

  if (g_strcmp0 (interface, "wl_test") == 0)
    (*test) = wl_registry_bind (registry, id, &wl_test_interface, 1);
}

static const struct wl_registry_listener registry_listener = {
  handle_global, NULL
};

MUTTER_TEST (bind_test_object)
{
  struct wl_test *test = NULL;

  struct wl_display *d = wl_display_connect (NULL);
  g_assert (d);
  struct wl_registry *r = wl_display_get_registry (d);
  g_assert (r);

  wl_registry_add_listener (r, &registry_listener, &test);
  wl_display_roundtrip (d);
  g_assert (wl_display_get_error (d) == 0);

  g_assert (test != NULL);

  wl_display_disconnect (d);
}

MUTTER_TEST (create_client_tst)
{
  struct client *c = client_create (0, 0, 100, 100);
  g_assert (c);
}

/**
 * Following tests are tests for wl_test requests and events. However, there's
 * no direct way how to test them --> if some test fail, it doesn't
 * necessarily means that the plugin doesn't work. It might be mutter or clutter
 * bug.
 */


MUTTER_TEST (move_surface_tst)
{
  struct client *c = client_create (0, 0, 10, 10);

  g_assert (c->input->pointer->focus == NULL);
  move_client (c, 10, 10);
  move_pointer (c, 11, 11);

  /* wait for movement => focus */
  do
  {
    client_roundtrip (c);
  } while (c->input->pointer->x != 1 || c->input->pointer->y != 1);

  /* if the surface is on the right place, we should get enter with [1, 1] */
  g_assert (c->input->pointer->focus != NULL);
  g_assert_cmpint (c->input->pointer->x, ==, 1);
  g_assert_cmpint (c->input->pointer->y, ==, 1);
}

MUTTER_TEST (move_pointer_tst)
{
  int i, r1, r2;
  struct client *c = client_create (10, 10, 10, 10);

  /* check output */
  g_assert_cmpint(c->output->width, >, 0);
  g_assert_cmpint(c->output->height, >, 0);

  for (i = 0; i < 10; ++i)
    {
        r1 = g_test_rand_int_range(0, c->output->width);
        r2 = g_test_rand_int_range(0, c->output->height);
        move_pointer (c, r1, r2);

        g_assert_cmpint (c->test->pointer_x, ==, r1);
        g_assert_cmpint (c->test->pointer_y, ==, r2);
    }
}

MUTTER_TEST (send_button_tst)
{
  struct client *c = client_create (0, 0, 100, 100);
  struct wl_test *t = c->test->wl_test;

  /* get focus on client so that we will get events */
  move_client (c, 10, 10);
  wl_test_move_pointer (t, 11, 11);

  /* no event sent yet */
  send_button (c, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
  send_button (c, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
}

MUTTER_TEST (activate_surface_tst)
{
  struct client *c = client_create (0, 0, 100, 100);
  struct wl_test *t = c->test->wl_test;

  /* try deactivate surface */
  wl_test_activate_surface (t, NULL);

  /* try to remove focus from surface, if the above didn't work */
  move_pointer (c, 150, 150);
  wl_test_send_button (t, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
  wl_test_send_button (t, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
  client_roundtrip (c);

  wl_test_activate_surface (c->test->wl_test, c->surface->wl_surface);
  client_roundtrip (c);

  g_assert (c->surface->activated);

  /* client now should have focus */
  send_key (c, 111, WL_KEYBOARD_KEY_STATE_PRESSED);
}

MUTTER_TEST (send_key_tst)
{
  struct client *c = client_create (0, 0, 100, 100);

  /* just test if we'll get answer from plugin */
  /* new client should have a focus */
  wl_test_activate_surface (c->test->wl_test, c->surface->wl_surface);
  send_key (c, 37, WL_KEYBOARD_KEY_STATE_PRESSED);
  send_key (c, 37, WL_KEYBOARD_KEY_STATE_RELEASED);
  client_roundtrip (c);
}
