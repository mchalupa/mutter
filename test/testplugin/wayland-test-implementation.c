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

#include <wayland-server.h>
#include <clutter/clutter.h>
#include <linux/input.h>

#include "wayland-test-server-protocol.h"
#include "meta-wayland-private.h"


static void
move_surface (struct wl_client *client,
              struct wl_resource *resource,
              struct wl_resource *surface,
              int32_t x,
              int32_t y);

static void
move_pointer (struct wl_client *client,
              struct wl_resource *resource,
              int32_t x,
              int32_t y);

static void
send_button (struct wl_client *client,
             struct wl_resource *resource,
             int32_t button,
             uint32_t state);

static void
activate_surface (struct wl_client *client,
                  struct wl_resource *resource,
                  struct wl_resource *surface);

static void
send_key (struct wl_client *client,
          struct wl_resource *resource,
          uint32_t key,
          uint32_t state);

static void
get_n_egl_buffers (struct wl_client *client,
                   struct wl_resource *resource);


const struct wl_test_interface wl_test_implementation = {
  move_surface,
  move_pointer,
  send_button,
  activate_surface,
  send_key,
  get_n_egl_buffers
};

static ClutterInputDevice *
clutter_get_device (ClutterInputDeviceType type)
{
  static ClutterDeviceManager *devmng = NULL;

  if (!devmng)
    devmng = clutter_device_manager_get_default ();

  return clutter_device_manager_get_core_device (devmng, type);
}

static ClutterActor *
get_stage (void)
{
	return CLUTTER_ACTOR (meta_backend_get_stage (meta_get_backend ()));
}

inline static gint64
get_time (void)
{
  return meta_display_get_current_time_roundtrip (meta_get_display ());
}

static void
move_surface (struct wl_client *client,
              struct wl_resource *resource,
              struct wl_resource *surface,
              int32_t x,
              int32_t y)
{
  MetaWaylandSurface *surf = wl_resource_get_user_data (surface);
  MetaRectangle rect;

  g_assert (surf);
  g_assert (surf->window);

  g_debug ("Moving surface %p to %d, %d\n", surf, x, y);
  meta_window_move_frame (surf->window, TRUE, x, y);

  /* check the result */
  meta_window_get_frame_rect (surf->window, &rect);
  g_assert_cmpint (x, ==, rect.x);
  g_assert_cmpint (y, ==, rect.y);
}

static void
move_pointer (struct wl_client *client,
              struct wl_resource *resource,
              int32_t x,
              int32_t y)
{
  ClutterActor *stage = get_stage ();
  MetaWaylandCompositor *compositor = meta_wayland_compositor_get_default ();
  ClutterInputDevice *dev = compositor->seat->pointer.device;
  g_assert (dev);

  ClutterEvent *ev = clutter_event_new (CLUTTER_MOTION);
  clutter_event_set_device (ev, dev);

  clutter_event_set_time (ev, get_time ());
  clutter_event_set_coords (ev, x, y);
  clutter_event_set_stage (ev, CLUTTER_STAGE (stage));
  ClutterActor *actor =
            clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                            CLUTTER_PICK_REACTIVE,
                                            (double) x, (double) y);
  clutter_event_set_source (ev, actor);

  g_debug ("Moving pointer to %d, %d", x, y);
  clutter_do_event (ev);
  clutter_event_free (ev);
}

static void
send_button (struct wl_client *client,
             struct wl_resource *resource,
             int32_t button,
             uint32_t state)
{
  ClutterEventType type;
  ClutterPoint pos;
  MetaWaylandCompositor *compositor = meta_wayland_compositor_get_default ();
  ClutterActor *stage = get_stage ();

  ClutterInputDevice *dev = compositor->seat->pointer.device;
  g_assert (dev);

  if (state == WL_POINTER_BUTTON_STATE_PRESSED)
    {
      type = CLUTTER_BUTTON_PRESS;
      g_debug("Sending button press: %d", button);
    }
  else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
    {
      type = CLUTTER_BUTTON_RELEASE;
      g_debug("Sending button release: %d", button);
    }
  else
    g_error ("Unknown button state");

  /* set right button relative to evdev */
  /* following 'magic numbers' are from meta-wayland-pointer.c
   * from default_grab_button () */
  switch (button)
    {
    case BTN_MIDDLE:
      button = 2;
      break;
    case BTN_RIGHT:
      button = 3;
      break;
    case BTN_LEFT:
      button = 1;
      break;
    default:
      g_error ("Unsupported button");
    };

  ClutterEvent *ev = clutter_event_new (type);
  clutter_event_set_device (ev, clutter_get_device (CLUTTER_POINTER_DEVICE));

  clutter_event_set_stage (ev, CLUTTER_STAGE (stage));
  clutter_input_device_get_coords(dev, NULL, &pos);
  clutter_event_set_coords (ev, pos.x, pos.y);
  ClutterActor *actor =
            clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                            CLUTTER_PICK_REACTIVE,
                                            pos.x, pos.y);
  clutter_event_set_source (ev, actor);

  clutter_event_set_time (ev, get_time ());
  clutter_event_set_button (ev, button);

  clutter_do_event (ev);
  clutter_event_free (ev);
}

static void
activate_surface (struct wl_client *client,
                  struct wl_resource *resource,
                  struct wl_resource *surface)
{
  MetaDisplay *display = meta_get_display ();

  /* unset focus */
  if (surface == NULL)
    {
      meta_display_focus_the_no_focus_window (display,
                                              display->screen,
                                              get_time ());
    return;
    }

  MetaWaylandSurface *surf = wl_resource_get_user_data (surface);
  g_assert (surf);

  g_debug("Activating surface %p", surf);
  meta_window_activate (surf->window, get_time ());
}

static void
send_key (struct wl_client *client,
          struct wl_resource *resource,
          uint32_t key,
          uint32_t state)
{
  ClutterEventType type;
  ClutterActor *stage = get_stage ();

  if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
      type = CLUTTER_KEY_PRESS;
      g_debug("Sending key press: %u", key);
    }
  else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
      type = CLUTTER_KEY_RELEASE;
      g_debug("Sending key release: %u", key);
    }
  else
    g_error ("Unknown key state");

  ClutterEvent *ev = clutter_event_new (type);
  clutter_event_set_device (ev, clutter_get_device (CLUTTER_KEYBOARD_DEVICE));

  clutter_event_set_stage (ev, CLUTTER_STAGE (stage));
  clutter_event_set_time (ev, get_time ());
  /* XXX all the keycodes from wayland differs by 8
   * see src/wayland/meta-wayland-keyboard.c:431 */
  clutter_event_set_key_code (ev, key + 8);

  clutter_do_event (ev);
  clutter_event_free (ev);
}

static void
get_n_egl_buffers (struct wl_client *client,
                   struct wl_resource *resource)
{
  wl_test_send_n_egl_buffers (resource, 0);
}
