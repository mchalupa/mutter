/*
 * Copyright (c) 2015 Red Hat, Inc.
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

#include "config.h"

#include <stdlib.h>

#include <meta/main.h>
#include <core/util-private.h>
#include <meta/window.h>
#include <meta/meta-backend.h>

#include "meta-plugin-manager.h"

#include <wayland/meta-wayland.h>
#include <wayland/meta-wayland-private.h>

#include <wayland-server.h>
#include <clutter/clutter.h>
#include <linux/input.h>

#include "protocol/wayland-fits-server-protocol.h"

/* we rely on g_assert */
#ifdef G_DISABLE_ASSERT
#error "g_assert is disabled. This code needs it"
#endif

static ClutterActor *
get_stage (void)
{
  MetaBackend *backend = meta_get_backend ();
  ClutterActor *stage;

  g_assert (backend);
  stage = meta_backend_get_stage (backend);
  g_assert (stage);

  return stage;
}

static void
move_surface (struct wl_client *client,
              struct wl_resource *resource,
              struct wl_resource *surface,
              int32_t x,
              int32_t y)
{
  MetaWaylandSurface *surf = wl_resource_get_user_data (surface);

  g_assert (surf);
  g_assert (surf->window);

  meta_window_move_frame (surf->window, FALSE, x, y);
}

const struct wfits_manip_interface manip_interface =
{
  move_surface
};

static void
surface_geometry (struct wl_client *client, struct wl_resource *resource,
                  struct wl_resource *surface_resource, uint32_t id)
{
  struct wl_resource *result;
  MetaRectangle rect;
  MetaWaylandSurface *surf = wl_resource_get_user_data (surface_resource);

  g_assert (surf);
  g_assert (surf->window);

  result = wl_resource_create (client,
                               &wfits_query_result_interface,
                               1, id);
  g_assert (result);

  meta_window_get_buffer_rect (surf->window, &rect);
  wfits_query_result_send_surface_geometry (result,
                                            wl_fixed_from_int (rect.x),
                                            wl_fixed_from_int (rect.y),
                                            rect.width, rect.height);
}

static void
global_pointer_position (struct wl_client *client, struct wl_resource *resource,
                         uint32_t id)
{
  ClutterPoint p;
  struct wl_resource *result;
  MetaWaylandCompositor *compositor = meta_wayland_compositor_get_default ();
  ClutterInputDevice *dev = compositor->seat->pointer.device;
  g_assert(dev);

  result = wl_resource_create (client, &wfits_query_result_interface, 1, id);
  g_assert (result);

  clutter_input_device_get_coords (dev, NULL, &p);
  wfits_query_result_send_global_pointer_position (result,
                                                   wl_fixed_from_double (p.x),
                                                   wl_fixed_from_double (p.y));
}

const struct wfits_query_interface query_interface =
{
  surface_geometry,
  global_pointer_position
};

static ClutterInputDevice *
clutter_get_device (ClutterInputDeviceType type)
{
  static ClutterDeviceManager *devmng = NULL;

  if (!devmng)
    devmng = clutter_device_manager_get_default ();

  return clutter_device_manager_get_core_device (devmng, type);
}

static void
move_pointer (struct wl_client *client,
              struct wl_resource *resource,
              int32_t x,
              int32_t y)
{
  MetaWaylandCompositor *compositor = meta_wayland_compositor_get_default ();
  ClutterInputDevice *dev = compositor->seat->pointer.device;
  ClutterActor *stage = get_stage ();

  ClutterEvent *ev = clutter_event_new (CLUTTER_MOTION);
  clutter_event_set_device (ev, dev);

  clutter_event_set_time (ev, g_get_monotonic_time ());
  clutter_event_set_coords (ev, x, y);
  clutter_event_set_stage (ev, CLUTTER_STAGE (stage));
  ClutterActor *actor =
    clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                    CLUTTER_PICK_REACTIVE,
                                    x, y);
  clutter_event_set_source (ev, actor);
  /* set synthetic flag, so that we can filter testing events
   * from normal events in mutter */
  clutter_event_set_flags (ev, CLUTTER_EVENT_FLAG_SYNTHETIC);

  clutter_event_put (ev);
  clutter_event_free (ev);
}

static void
send_key (struct wl_client *client,
          struct wl_resource *resource,
          uint32_t key,
          uint32_t state)
{
  ClutterEventType type;
  ClutterEvent *ev;
  ClutterPoint p;
  MetaWaylandCompositor *compositor = meta_wayland_compositor_get_default ();
  ClutterInputDevice *dev = compositor->seat->pointer.device;
  ClutterActor *stage = get_stage ();

  if (key == BTN_MIDDLE || key == BTN_RIGHT || key == BTN_LEFT)
    {
      if (state == WL_POINTER_BUTTON_STATE_PRESSED)
        type = CLUTTER_BUTTON_PRESS;
      else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
        type = CLUTTER_BUTTON_RELEASE;
      else
        g_error ("Unknown button state");

      /* set right button relative to evdev */
      /* following 'magic numbers' are from meta-wayland-pointer.c
       * from default_grab_button () */
      switch (key)
        {
        case BTN_MIDDLE:
          key = CLUTTER_BUTTON_MIDDLE;
          break;
        case BTN_RIGHT:
          key = CLUTTER_BUTTON_SECONDARY;
          break;
        case BTN_LEFT:
          key = CLUTTER_BUTTON_PRIMARY;
          break;
        default:
          g_error ("Unsupported button");
        };

      ev = clutter_event_new (type);
      clutter_event_set_device (ev, clutter_get_device (CLUTTER_POINTER_DEVICE));
      clutter_event_set_button (ev, key);
    }
  else
    {
      g_assert (key < 255);

      if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
        type = CLUTTER_KEY_PRESS;
      else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        type = CLUTTER_KEY_RELEASE;
      else
        g_error ("Unknown key state");

      ev = clutter_event_new (type);
      clutter_event_set_device (ev, clutter_get_device (CLUTTER_KEYBOARD_DEVICE));
      /* XXX all the keycodes from wayland differs by 8
       * see src/wayland/meta-wayland-keyboard.c:431 */
      clutter_event_set_key_code (ev, key + 8);
    }

  clutter_event_set_stage (ev, CLUTTER_STAGE (stage));

  clutter_input_device_get_coords (dev, NULL, &p);
  clutter_event_set_coords (ev, p.x, p.y);
  ClutterActor *actor =
    clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                    CLUTTER_PICK_REACTIVE,
                                    p.x, p.y);
  clutter_event_set_source (ev, actor);
  clutter_event_set_time (ev, g_get_monotonic_time ());
  clutter_event_set_flags (ev, CLUTTER_EVENT_FLAG_SYNTHETIC);

  clutter_event_put (ev);
  clutter_event_free (ev);
}

const struct wfits_input_interface input_interface =
{
  move_pointer,
  send_key
};

static void
bind_manip (struct wl_client *client, void *data,
            uint32_t version, uint32_t id)
{
  struct wl_resource *res = wl_resource_create (client, &wfits_manip_interface,
                                                version, id);

  if (!res)
    {
      wl_client_post_no_memory (client);
      return;
    }

  wl_resource_set_implementation (res, &manip_interface, data, NULL);
}

static void
bind_query (struct wl_client *client, void *data,
            uint32_t version, uint32_t id)
{
  struct wl_resource *res = wl_resource_create (client, &wfits_query_interface,
                                                version, id);
  if (!res)
    {
      wl_client_post_no_memory (client);
      return;
    }

  wl_resource_set_implementation (res, &query_interface, data, NULL);
}

static void
bind_input (struct wl_client *client, void *data,
            uint32_t version, uint32_t id)
{
  struct wl_resource *res = wl_resource_create (client, &wfits_input_interface,
                                                version, id);
  if (!res)
    {
      wl_client_post_no_memory (client);
      return;
    }

  wl_resource_set_implementation (res, &input_interface, data, NULL);
}

static gboolean
wfits_init (void)
{
  MetaWaylandCompositor *compositor = meta_wayland_compositor_get_default ();
  struct wl_display *display = compositor->wayland_display;
  struct wl_global *global;

  global = wl_global_create (display, &wfits_manip_interface,
                             wfits_manip_interface.version, NULL, bind_manip);
  if (!global)
    g_error ("Failed creating global for manip interface");

  global = wl_global_create (display, &wfits_query_interface,
                             wfits_query_interface.version, NULL, bind_query);
  if (!global)
    g_error ("Failed creating global for query interface");

  global = wl_global_create (display, &wfits_input_interface,
                             wfits_input_interface.version, NULL, bind_input);
  if (!global)
    g_error ("Failed creating global for input interface");

  g_debug("Initialized wayland-fits interfaces");

  return FALSE;
}

/* this part is taken from test-runner.c */
int
main (int argc, char **argv)
{
  GOptionContext *ctx;
  GError *error = NULL;

  char *fake_args[] = { NULL, "--wayland" };
  fake_args[0] = argv[0];
  char **fake_argv = fake_args;
  int fake_argc = 2;

  ctx = meta_get_option_context ();
  if (!g_option_context_parse (ctx, &fake_argc, &fake_argv, &error))
    {
      g_printerr ("mutter: %s\n", error->message);
      exit (1);
    }
  g_option_context_free (ctx);

  meta_plugin_manager_load ("default");

  meta_init ();
  meta_register_with_session ();

  meta_later_add (META_LATER_BEFORE_REDRAW,
                  (GSourceFunc) wfits_init,
                  NULL, NULL);

  meta_set_testing ();

  return meta_run ();
}
