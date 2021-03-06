/*
 * Wayland Support
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2012 Collabora, Ltd.
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

#ifndef META_WAYLAND_KEYBOARD_H
#define META_WAYLAND_KEYBOARD_H

#include <clutter/clutter.h>
#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>

typedef struct
{
  struct xkb_keymap *keymap;
  struct xkb_state *state;
  int keymap_fd;
  size_t keymap_size;
  char *keymap_area;
} MetaWaylandXkbInfo;

struct _MetaWaylandKeyboard
{
  struct wl_display *display;

  struct wl_list resource_list;
  struct wl_list focus_resource_list;

  MetaWaylandSurface *focus_surface;
  struct wl_listener focus_surface_listener;
  uint32_t focus_serial;

  MetaWaylandXkbInfo xkb_info;
  enum xkb_state_component mods_changed;

  GSettings *settings;
};

void meta_wayland_keyboard_init (MetaWaylandKeyboard *keyboard,
                                 struct wl_display   *display);

void meta_wayland_keyboard_release (MetaWaylandKeyboard *keyboard);

void meta_wayland_keyboard_update (MetaWaylandKeyboard *keyboard,
                                   const ClutterKeyEvent *event);

gboolean meta_wayland_keyboard_handle_event (MetaWaylandKeyboard *keyboard,
                                             const ClutterKeyEvent *event);

void meta_wayland_keyboard_set_focus (MetaWaylandKeyboard *keyboard,
                                      MetaWaylandSurface *surface);

struct wl_client * meta_wayland_keyboard_get_focus_client (MetaWaylandKeyboard *keyboard);

void meta_wayland_keyboard_create_new_resource (MetaWaylandKeyboard *keyboard,
                                                struct wl_client    *client,
                                                struct wl_resource  *seat_resource,
                                                uint32_t id);

#endif /* META_WAYLAND_KEYBOARD_H */
