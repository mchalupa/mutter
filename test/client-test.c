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
#include "xdg-shell-client-protocol.h"

MUTTER_TEST (maximize_client_tst)
{
  struct client *c = client_create (100, 100, 400, 200);
  int i;

  for (i = 0; i < 10; ++i)
    {
      maximize_client (c);
      g_assert_cmpint (c->surface->width, ==, c->output->width);
      g_assert_cmpint (c->surface->height, ==, c->output->height);

      unmaximize_client (c);
      g_assert_cmpint (c->surface->width, ==, 400);
      g_assert_cmpint (c->surface->height, ==, 200);
    }

  /* set maximized few times and then unmaximize few times */
  for (i = 0; i < 50; ++i)
    {
        xdg_surface_set_maximized (c->surface->xdg_surface);
        client_roundtrip (c);
    }

  g_assert (c->surface->maximized);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  for (i = 0; i < 50; ++i)
    {
      xdg_surface_unset_maximized (c->surface->xdg_surface);
      client_roundtrip (c);
    }

  g_assert (!c->surface->maximized);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  /* try once more the geometry */
  maximize_client (c);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  unmaximize_client (c);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  client_destroy (c);
}

MUTTER_TEST (maximize_flood_tst)
{
  struct client *c = client_create (100, 100, 400, 200);
  int i;

  for (i = 0; i < 100; ++i)
    {
      maximize_client (c);
      g_assert_cmpint (c->surface->width, ==, c->output->width);
      g_assert_cmpint (c->surface->height, ==, c->output->height);

      unmaximize_client (c);
      g_assert_cmpint (c->surface->width, ==, 400);
      g_assert_cmpint (c->surface->height, ==, 200);
    }

   /* flood the display with request, what it will do? */
   for (i = 0; i < 100; ++i)
    {
        xdg_surface_set_maximized (c->surface->xdg_surface);
        xdg_surface_unset_maximized (c->surface->xdg_surface);
        client_roundtrip (c);
    }

  g_assert (!c->surface->maximized);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  for (i = 0; i < 100; ++i)
    {
        xdg_surface_set_maximized (c->surface->xdg_surface);
        client_roundtrip (c);
    }

  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  g_assert (c->surface->maximized);

  for (i = 0; i < 100; ++i)
    {
      xdg_surface_unset_maximized (c->surface->xdg_surface);
      client_roundtrip (c);
    }

  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  g_assert (!c->surface->maximized);

  /* try once more the geometry */
  maximize_client (c);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  unmaximize_client (c);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  client_destroy (c);
}

MUTTER_TEST (fullscreen_client_tst)
{
  struct client *c = client_create (100, 100, 400, 200);
  int i;

  for (i = 0; i < 10; ++i)
    {
      fullscreen_client (c);
      g_assert_cmpint (c->surface->width, ==, c->output->width);
      g_assert_cmpint (c->surface->height, ==, c->output->height);

      unfullscreen_client (c);
      g_assert_cmpint (c->surface->width, ==, 400);
      g_assert_cmpint (c->surface->height, ==, 200);
    }

  for (i = 0; i < 50; ++i)
    {
        xdg_surface_set_fullscreen (c->surface->xdg_surface,
                                    c->output->wl_output);
        client_roundtrip (c);
    }

  g_assert (c->surface->fullscreen);

  for (i = 0; i < 50; ++i)
    {
      xdg_surface_unset_fullscreen (c->surface->xdg_surface);
      client_roundtrip (c);
    }

  g_assert (!c->surface->fullscreen);

  fullscreen_client (c);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  unfullscreen_client (c);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  client_destroy (c);
}

MUTTER_TEST (fullscreen_flood_tst)
{
  struct client *c = client_create (100, 100, 400, 200);
  int i;

  for (i = 0; i < 100; ++i)
    {
      fullscreen_client (c);
      g_assert_cmpint (c->surface->width, ==, c->output->width);
      g_assert_cmpint (c->surface->height, ==, c->output->height);

      unfullscreen_client (c);
      g_assert_cmpint (c->surface->width, ==, 400);
      g_assert_cmpint (c->surface->height, ==, 200);
    }

   /* flood the display with request, what it will do? */
   for (i = 0; i < 100; ++i)
    {
        xdg_surface_set_fullscreen (c->surface->xdg_surface,
                                    c->output->wl_output);
        xdg_surface_unset_fullscreen (c->surface->xdg_surface);
        client_roundtrip (c);
    }

  g_assert (!c->surface->fullscreen);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  for (i = 0; i < 100; ++i)
    {
        xdg_surface_set_fullscreen (c->surface->xdg_surface,
                                    c->output->wl_output);
        client_roundtrip (c);
    }

  g_assert (c->surface->fullscreen);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  for (i = 0; i < 100; ++i)
    {
      xdg_surface_unset_fullscreen (c->surface->xdg_surface);
      client_roundtrip (c);
    }

  g_assert (!c->surface->fullscreen);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  /* try once more the geometry */
  fullscreen_client (c);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);

  unfullscreen_client (c);
  g_assert_cmpint (c->surface->width, ==, 400);
  g_assert_cmpint (c->surface->height, ==, 200);

  client_destroy (c);
}

MUTTER_TEST(unmaximize_not_maximized_surface_tst)
{
  struct client *c = client_create (100, 50, 200, 200);

  xdg_surface_unset_maximized (c->surface->xdg_surface);
  client_roundtrip (c);

  g_assert_cmpint (c->surface->width, ==, 200);
  g_assert_cmpint (c->surface->height, ==, 200);
  g_assert (!c->surface->maximized);

  client_destroy (c);
}

MUTTER_TEST(maximize_fullscreen_mixed_tst)
{
  struct client *c = client_create (0, 0, 200, 200);

  maximize_client (c);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);
  g_assert (c->surface->maximized);

  fullscreen_client (c);
  g_assert_cmpint (c->surface->width, ==, c->output->width);
  g_assert_cmpint (c->surface->height, ==, c->output->height);
  g_assert (c->surface->fullscreen);
  g_assert (!c->surface->maximized);

  /* the client should be intert to resizing while fullscreened */
  xdg_surface_set_maximized (c->surface->xdg_surface);
  client_roundtrip (c);

  g_assert (c->surface->fullscreen);
  g_assert (!c->surface->maximized);

  client_destroy (c);
}
