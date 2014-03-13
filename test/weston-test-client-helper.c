/*
 * Copyright (c) 2012 Intel Corporation
 * Copyright (c) 2014 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <errno.h>

#include "weston-test-client-helper.h"
#include "xdg-shell-client-protocol.h"

int
surface_contains (struct surface *surface, int x, int y)
{
  /* test whether a global x,y point is contained in the surface */
  int sx = surface->x;
  int sy = surface->y;
  int sw = surface->width;
  int sh = surface->height;
  return x >= sx && y >= sy && x < sx + sw && y < sy + sh;
}

static void
frame_callback_handler (void *data, struct wl_callback *callback, uint32_t time)
{
  int *done = data;

  *done = 1;

  wl_callback_destroy (callback);
}

static const struct wl_callback_listener frame_listener =
{
  frame_callback_handler
};

struct wl_callback *
frame_callback_set (struct wl_surface *surface, int *done)
{
  struct wl_callback *callback;

  *done = 0;
  callback = wl_surface_frame (surface);
  wl_callback_add_listener (callback, &frame_listener, done);

  return callback;
}

int
frame_callback_wait_nofail(struct client *client, int *done)
{
  while (!*done)
    {
      if (wl_display_dispatch(client->wl_display) < 0)
        return 0;
    }

  return 1;
}

void
move_client (struct client *client, int x, int y)
{
  struct surface *surface = client->surface;
  int done;

  client->surface->x = x;
  client->surface->y = y;
  wl_test_move_surface (client->test->wl_test, surface->wl_surface,
                        surface->x, surface->y);
  /* The attach here is necessary because commit() will call congfigure
   * only on surfaces newly attached, and the one that sets the surface
   * position is the configure. */
  wl_surface_attach (surface->wl_surface, surface->wl_buffer, 0, 0);
  wl_surface_damage (surface->wl_surface, 0, 0, surface->width,
                     surface->height);

  frame_callback_set (surface->wl_surface, &done);

  wl_surface_commit (surface->wl_surface);

  frame_callback_wait (client, &done);
}

static void
xdg_shell_handle_ping (void *data, struct xdg_shell *xdg_shell, uint32_t serial)
{
  struct client *c = data;
  c->last_ping_serial = serial;

  xdg_shell_pong (xdg_shell, serial);

  g_fprintf (stderr, "test-client: got ping with serial %u, sent pong\n", serial);
}

static const struct xdg_shell_listener xdg_shell_listener = {
  xdg_shell_handle_ping
};

static void
xdg_surface_handle_configure (void *data, struct xdg_surface *xdg_surface,
                              int32_t width, int32_t height,
                              struct wl_array *states, uint32_t serial)
{
  struct surface *s = data;
  enum xdg_surface_state *st;

  g_fprintf (stderr, "test-client: xdg_surface.configure:\n");
  g_fprintf (stderr, "\twidth: %d, height: %d\n", width, height);

  s->fullscreen = 0;
  s->maximized = 0;
  s->resizing = 0;
  s->activated = 0;

  wl_array_for_each(st, states)
    {
      switch (*st)
      {
      case XDG_SURFACE_STATE_FULLSCREEN:
        g_fprintf (stderr, "\tfullscreen\n");
        s->fullscreen = 1;
        break;
      case XDG_SURFACE_STATE_MAXIMIZED:
        g_fprintf (stderr, "\tmaximized\n");
        s->maximized = 1;
        break;
      case XDG_SURFACE_STATE_RESIZING:
        g_fprintf (stderr, "\tresizing\n");
        s->resizing = 1;
        break;
      case XDG_SURFACE_STATE_ACTIVATED:
        g_fprintf (stderr, "\tactivated\n");
        s->activated = 1;
        break;
      default:
        g_error ("Unkown state (%d)", *st);
      }
  }

  xdg_surface_ack_configure (xdg_surface, serial);
}
static void
xdg_surface_handle_close (void *data, struct xdg_surface *xdg_surface)
{
  struct surface *s = data;

  s->close = 1;

  g_fprintf (stderr, "test-client: xdg_surface.close\n");
}

static const struct xdg_surface_listener xdg_surface_listener = {
  xdg_surface_handle_configure,
  xdg_surface_handle_close
};

int
get_n_egl_buffers (struct client *client)
{
  client->test->n_egl_buffers = -1;

  wl_test_get_n_egl_buffers (client->test->wl_test);
  wl_display_roundtrip (client->wl_display);

  return client->test->n_egl_buffers;
}

static void
pointer_handle_enter (void *data, struct wl_pointer *wl_pointer,
                      uint32_t serial, struct wl_surface *wl_surface,
                      wl_fixed_t x, wl_fixed_t y)
{
  struct pointer *pointer = data;

  pointer->focus = wl_surface_get_user_data (wl_surface);
  pointer->x = wl_fixed_to_int (x);
  pointer->y = wl_fixed_to_int (y);

  g_fprintf (stderr, "test-client: got pointer enter %d %d, surface %p\n",
             pointer->x, pointer->y, pointer->focus);
}

static void
pointer_handle_leave (void *data, struct wl_pointer *wl_pointer,
                      uint32_t serial, struct wl_surface *wl_surface)
{
  struct pointer *pointer = data;

  pointer->focus = NULL;

  g_fprintf (stderr, "test-client: got pointer leave, surface %p\n",
             wl_surface_get_user_data (wl_surface));
}

static void
pointer_handle_motion (void *data, struct wl_pointer *wl_pointer,
                       uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
  struct pointer *pointer = data;

  pointer->x = wl_fixed_to_int (x);
  pointer->y = wl_fixed_to_int (y);

  g_fprintf (stderr, "test-client: got pointer motion %d %d\n",
             pointer->x, pointer->y);
}

static void
pointer_handle_button (void *data, struct wl_pointer *wl_pointer,
                       uint32_t serial, uint32_t time, uint32_t button,
                       uint32_t state)
{
  struct pointer *pointer = data;

  pointer->button = button;
  pointer->state = state;

  g_fprintf (stderr, "test-client: got pointer button %u %u\n",
             button, state);
}

static void
pointer_handle_axis (void *data, struct wl_pointer *wl_pointer,
                     uint32_t time, uint32_t axis, wl_fixed_t value)
{
  g_fprintf (stderr, "test-client: got pointer axis %u %f\n",
             axis, wl_fixed_to_double (value));
}

static const struct wl_pointer_listener pointer_listener =
{
  pointer_handle_enter,
  pointer_handle_leave,
  pointer_handle_motion,
  pointer_handle_button,
  pointer_handle_axis,
};

static void
keyboard_handle_keymap (void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t format, int fd, uint32_t size)
{
  close (fd);

  g_fprintf (stderr, "test-client: got keyboard keymap\n");
}

static void
keyboard_handle_enter (void *data, struct wl_keyboard *wl_keyboard,
                       uint32_t serial, struct wl_surface *wl_surface,
                       struct wl_array *keys)
{
  struct keyboard *keyboard = data;

  keyboard->focus = wl_surface_get_user_data (wl_surface);

  g_fprintf (stderr, "test-client: got keyboard enter, surface %p\n",
             keyboard->focus);
}

static void
keyboard_handle_leave (void *data, struct wl_keyboard *wl_keyboard,
                       uint32_t serial, struct wl_surface *wl_surface)
{
  struct keyboard *keyboard = data;

  keyboard->focus = NULL;

  g_fprintf (stderr, "test-client: got keyboard leave, surface %p\n",
             wl_surface_get_user_data (wl_surface));
}

static void
keyboard_handle_key (void *data, struct wl_keyboard *wl_keyboard,
                     uint32_t serial, uint32_t time, uint32_t key,
                     uint32_t state)
{
  struct keyboard *keyboard = data;

  keyboard->key = key;
  keyboard->state = state;

  g_fprintf (stderr, "test-client: got keyboard key %u %u\n", key, state);
}

static void
keyboard_handle_modifiers (void *data, struct wl_keyboard *wl_keyboard,
                           uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked,
                           uint32_t group)
{
  struct keyboard *keyboard = data;

  keyboard->mods_depressed = mods_depressed;
  keyboard->mods_latched = mods_latched;
  keyboard->mods_locked = mods_locked;
  keyboard->group = group;

  g_fprintf (stderr, "test-client: got keyboard modifiers %u %u %u %u\n",
             mods_depressed, mods_latched, mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener =
{
  keyboard_handle_keymap,
  keyboard_handle_enter,
  keyboard_handle_leave,
  keyboard_handle_key,
  keyboard_handle_modifiers,
};

static void
surface_enter (void *data,
               struct wl_surface *wl_surface, struct wl_output *output)
{
  struct surface *surface = data;

  surface->output = wl_output_get_user_data (output);

  g_fprintf (stderr, "test-client: got surface enter output %p\n",
             surface->output);
}

static void
surface_leave (void *data,
               struct wl_surface *wl_surface, struct wl_output *output)
{
  struct surface *surface = data;

  surface->output = NULL;

  g_fprintf (stderr, "test-client: got surface leave output %p\n",
             wl_output_get_user_data (output));
}

static const struct wl_surface_listener surface_listener =
{
  surface_enter,
  surface_leave
};

struct wl_buffer *
create_shm_buffer (struct client *client, int width, int height, void **pixels)
{
  char filename[] = "/tmp/wayland-shm-XXXXXX";
  struct wl_shm *shm = client->wl_shm;
  struct wl_shm_pool *pool;
  int fd, size, stride;
  void *data;
  struct wl_buffer *buffer;

  fd = mkstemp (filename);
  if (fd < 0)
    {
      g_critical (G_STRLOC ": Unable to create temporary file (%s): %s",
                  filename, g_strerror (errno));
      return NULL;
    }

  stride = width * 4;
  size = stride * height;
  if (ftruncate (fd, size) < 0)
    {
      g_critical (G_STRLOC ": Truncating temporary file failed: %s",
                  g_strerror (errno));
      close (fd);
      return NULL;
    }

  data = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  unlink (filename);

  if (data == MAP_FAILED)
    {
      g_critical (G_STRLOC ": mmap'ping temporary file failed: %s",
                  g_strerror (errno));
      close (fd);
      return NULL;
    }

  pool = wl_shm_create_pool (shm, fd, size);
  buffer = wl_shm_pool_create_buffer (pool, 0, width, height, stride,
                                      WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy (pool);

  close (fd);

  if (pixels)
    *pixels = data;

  return buffer;
}

static void
shm_format (void *data, struct wl_shm *wl_shm, uint32_t format)
{
  struct client *client = data;

  if (format == WL_SHM_FORMAT_ARGB8888)
    client->has_argb = 1;
}

struct wl_shm_listener shm_listener =
{
  shm_format
};

static void
test_handle_pointer_position (void *data, struct wl_test *wl_test,
                              wl_fixed_t x, wl_fixed_t y)
{
  struct test *test = data;
  test->pointer_x = wl_fixed_to_int (x);
  test->pointer_y = wl_fixed_to_int (y);

  g_fprintf (stderr, "test-client: got global pointer %d %d\n",
             test->pointer_x, test->pointer_y);
}

static void
test_handle_n_egl_buffers (void *data, struct wl_test *wl_test, uint32_t n)
{
  struct test *test = data;

  test->n_egl_buffers = n;
}

static const struct wl_test_listener test_listener =
{
  test_handle_pointer_position,
  test_handle_n_egl_buffers,
};

static void
seat_handle_capabilities (void *data, struct wl_seat *seat,
                          enum wl_seat_capability caps)
{
  struct input *input = data;
  struct pointer *pointer;
  struct keyboard *keyboard;

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !input->pointer)
    {
      pointer = g_new0 (struct pointer, 1);
      pointer->wl_pointer = wl_seat_get_pointer (seat);
      wl_pointer_set_user_data (pointer->wl_pointer, pointer);
      wl_pointer_add_listener (pointer->wl_pointer, &pointer_listener,
                               pointer);
      input->pointer = pointer;
    }
  else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && input->pointer)
    {
      wl_pointer_destroy (input->pointer->wl_pointer);
      g_free (input->pointer);
      input->pointer = NULL;
    }

  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !input->keyboard)
    {
      keyboard = g_new0 (struct keyboard, 1);
      keyboard->wl_keyboard = wl_seat_get_keyboard (seat);
      wl_keyboard_set_user_data (keyboard->wl_keyboard, keyboard);
      wl_keyboard_add_listener (keyboard->wl_keyboard, &keyboard_listener,
                                keyboard);
      input->keyboard = keyboard;
    }
  else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && input->keyboard)
    {
      wl_keyboard_destroy (input->keyboard->wl_keyboard);
      g_free (input->keyboard);
      input->keyboard = NULL;
    }
}

static const struct wl_seat_listener seat_listener =
{
  seat_handle_capabilities,
};

static void
output_handle_geometry (void *data,
                        struct wl_output *wl_output,
                        int x, int y,
                        int physical_width,
                        int physical_height,
                        int subpixel,
                        const char *make,
                        const char *model,
                        int32_t transform)
{
  struct output *output = data;

  output->x = x;
  output->y = y;
}

static void
output_handle_mode (void *data,
                    struct wl_output *wl_output,
                    uint32_t flags,
                    int width,
                    int height,
                    int refresh)
{
  struct output *output = data;

  if (flags & WL_OUTPUT_MODE_CURRENT)
    {
      output->width = width;
      output->height = height;
    }
}

static const struct wl_output_listener output_listener =
{
  output_handle_geometry,
  output_handle_mode
};

static void
handle_global (void *data, struct wl_registry *registry,
               uint32_t id, const char *interface, uint32_t version)
{
  struct client *client = data;
  struct input *input;
  struct output *output;
  struct test *test;
  struct global *global;

  global = g_new0 (struct global, 1);
  global->name = id;
  global->interface = g_strdup (interface);
  g_assert (interface);
  global->version = version;
  wl_list_insert (client->global_list.prev, &global->link);

  if (g_strcmp0 (interface, "wl_compositor") == 0)
    {
      client->wl_compositor = wl_registry_bind (registry, id,
                              &wl_compositor_interface, 1);
    }
  else if (g_strcmp0 (interface, "wl_seat") == 0)
    {
      input = g_new0 (struct input, 1);
      input->wl_seat = wl_registry_bind (registry, id,
                                         &wl_seat_interface, 1);
      wl_seat_add_listener (input->wl_seat, &seat_listener, input);
      client->input = input;
    }
  else if (g_strcmp0 (interface, "wl_shm") == 0)
    {
      client->wl_shm = wl_registry_bind (registry, id, &wl_shm_interface, 1);
      wl_shm_add_listener (client->wl_shm, &shm_listener, client);
    }
  else if (g_strcmp0 (interface, "wl_output") == 0)
    {
      output = g_new0 (struct output, 1);
      output->wl_output = wl_registry_bind (registry, id,
                                            &wl_output_interface, 1);
      wl_output_add_listener (output->wl_output,
                              &output_listener, output);
      client->output = output;
    }
  else if (g_strcmp0 (interface, "wl_test") == 0)
    {
      test = g_new0 (struct test, 1);
      test->wl_test = wl_registry_bind (registry, id,
                                        &wl_test_interface, 1);
      wl_test_add_listener (test->wl_test, &test_listener, test);
      client->test = test;
    }
  else if (g_strcmp0 (interface, "xdg_shell") == 0)
    {
      client->xdg_shell = wl_registry_bind (registry, id,
                                            &xdg_shell_interface, 1);
      xdg_shell_add_listener (client->xdg_shell, &xdg_shell_listener, client);
    }

}

static const struct wl_registry_listener registry_listener =
{
  handle_global
};

void
skip (const char *fmt, ...)
{
  va_list argp;

  va_start (argp, fmt);
  g_vfprintf (stderr, fmt, argp);
  va_end (argp);

  /* automake tests uses exit code 77. weston-test-runner will see
   * this and use it, and then weston-test's sigchld handler (in the
   * weston process) will use that as an exit status, which is what
   * automake will see in the end. */
  exit(77);
}

void
expect_protocol_error(struct client *client,
                      const struct wl_interface *intf,
                      uint32_t code)
{
  int err;
  uint32_t errcode;
  gboolean failed = FALSE;
  const struct wl_interface *interface;
  unsigned int id;

  /* if the error has not come yet, make it happen */
  wl_display_roundtrip(client->wl_display);

  err = wl_display_get_error(client->wl_display);

  g_assert(err && "Expected protocol error but nothing came");
  g_assert(err == EPROTO && "Expected protocol error but got local error");

  errcode = wl_display_get_protocol_error(client->wl_display,
                                          &interface, &id);

  /* check error */
  if (errcode != code)
    {
      g_fprintf(stderr, "Should get error code %d but got %d\n",
                code, errcode);
      failed = TRUE;
    }

  /* this should be definitely set */
  g_assert(interface);

  if (g_strcmp0(intf->name, interface->name) != 0)
    {
      g_fprintf(stderr, "Should get interface '%s' but got '%s'\n",
                intf->name, interface->name);
      failed = TRUE;
    }

  if (failed)
    {
      g_fprintf(stderr, "Expected other protocol error\n");
      abort();
    }

  /* all OK */
  g_fprintf(stderr, "Got expected protocol error on '%s' (object id: %d) "
            "with code %d\n", interface->name, id, errcode);
}

static void
log_handler (const char *fmt, va_list args)
{
  g_fprintf (stderr, "libwayland: ");
  g_vfprintf (stderr, fmt, args);
}

struct client *
client_create (int x, int y, int width, int height)
{
  struct client *client;
  struct surface *surface;

  wl_log_set_handler_client (log_handler);

  /* connect to display */
  client = g_new0 (struct client, 1);
  client->wl_display = wl_display_connect (NULL);
  g_assert (client->wl_display);
  wl_list_init (&client->global_list);

  /* setup registry so we can bind to interfaces */
  client->wl_registry = wl_display_get_registry (client->wl_display);
  wl_registry_add_listener (client->wl_registry, &registry_listener, client);

  /* trigger global listener */
  wl_display_dispatch (client->wl_display);
  wl_display_roundtrip (client->wl_display);

  /* must have WL_SHM_FORMAT_ARGB32 */
  g_assert (client->has_argb);

  /* must have wl_test interface */
  g_assert (client->test);

  /* must have an output */
  g_assert (client->output);

  /* initialize the client surface */
  surface = g_new0 (struct surface, 1);
  surface->wl_surface = wl_compositor_create_surface (client->wl_compositor);
  g_assert (surface->wl_surface);

  wl_surface_add_listener (surface->wl_surface, &surface_listener, surface);

  client->surface = surface;
  wl_surface_set_user_data (surface->wl_surface, surface);

  surface->width = width;
  surface->height = height;
  surface->wl_buffer = create_shm_buffer (client, width, height,
                                          &surface->data);

  memset (surface->data, 0xaf, width * height * 4);

  surface->xdg_surface = xdg_shell_get_xdg_surface (client->xdg_shell,
                         surface->wl_surface);
  g_assert (surface->xdg_surface);
  xdg_surface_add_listener (surface->xdg_surface, &xdg_surface_listener, surface);

  move_client(client, x, y);

  return client;
}
