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

#include <config.h>

#include <meta/meta-plugin.h>
#include <clutter/clutter.h>

#include "meta-wayland-private.h"
#include "meta-background-actor.h"
#include "meta-background-group.h"

/* === MetaWaylandPlugin declarations === */
#define META_TYPE_WAYLAND_TEST_PLUGIN (meta_wayland_test_plugin_get_type ())

#define META_WAYLAND_TEST_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), META_TYPE_WAYLAND_TEST_PLUGIN, MetaWaylandTestPlugin))

#define META_WAYLAND_TEST_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST  \
    ((klass), META_TYPE_WAYLAND_TEST_PLUGIN, MetaWaylandTestPluginClass))

#define META_IS_WAYLAND_TEST_PLUGIN(obj)  \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), META_WAYLAND_TEST_PLUGIN_TYPE))

#define META_IS_WAYLAND_TEST_PLUGIN_CLASS(klass)  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), META_TYPE_WAYLAND_TEST_PLUGIN))

#define META_WAYLAND_TEST_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS  \
    ((obj), META_TYPE_WAYLAND_TEST_PLUGIN, MetaWaylandTestPluginClass))

#define META_WAYLAND_TEST_PLUGIN_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE  \
  ((obj), META_TYPE_WAYLAND_TEST_PLUGIN, MetaWaylandTestPluginPrivate))

typedef struct _MetaWaylandTestPlugin        MetaWaylandTestPlugin;
typedef struct _MetaWaylandTestPluginClass   MetaWaylandTestPluginClass;
typedef struct _MetaWaylandTestPluginPrivate MetaWaylandTestPluginPrivate;

struct _MetaWaylandTestPlugin
{
  MetaPlugin parent;

  MetaWaylandTestPluginPrivate *priv;
};

struct _MetaWaylandTestPluginClass
{
  MetaPluginClass parent_class;
};

struct _MetaWaylandTestPluginPrivate
{
  MetaPluginInfo info;

  MetaWaylandCompositor *compositor;
  struct wl_resource *test_object_resource;

  ClutterActor *background_group;
};
/* === END MetaWaylandPlugin declarations === */


META_PLUGIN_DECLARE (MetaWaylandTestPlugin, meta_wayland_test_plugin);

static const MetaPluginInfo *
plugin_info (MetaPlugin *plugin)
{
  MetaWaylandTestPluginPrivate *priv = META_WAYLAND_TEST_PLUGIN (plugin)->priv;

  return &priv->info;
}

static void
meta_wayland_test_plugin_dispose (GObject *object)
{
  G_OBJECT_CLASS (meta_wayland_test_plugin_parent_class)->dispose (object);
}

static void
meta_wayland_test_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (meta_wayland_test_plugin_parent_class)->finalize (object);
}

static void
start (MetaPlugin *plugin);

static void
meta_wayland_test_plugin_class_init (MetaWaylandTestPluginClass *klass)
{
  GObjectClass    *gobject_class = G_OBJECT_CLASS (klass);
  MetaPluginClass *plugin_class  = META_PLUGIN_CLASS (klass);

  gobject_class->finalize        = meta_wayland_test_plugin_finalize;
  gobject_class->dispose         = meta_wayland_test_plugin_dispose;

  plugin_class->plugin_info      = plugin_info;
  plugin_class->start            = start;

  g_type_class_add_private (gobject_class, sizeof (MetaWaylandTestPluginPrivate));
}

static void
meta_wayland_test_plugin_init (MetaWaylandTestPlugin *self)
{
  MetaWaylandTestPluginPrivate *priv;

  priv = META_WAYLAND_TEST_PLUGIN_GET_PRIVATE (self);
  self->priv = priv;

  priv->info.name        = "Mutter wayland tests";
  priv->info.version     = "1";
  priv->info.author      = "Red Hat, Inc.";
  priv->info.license     = "GPL";
  priv->info.description = "This plugin inserts wl_test object into mutter";

  priv->compositor = meta_wayland_compositor_get_default ();
}

static void
set_background (ClutterActor *actor)
{
  GRand *rand = g_rand_new_with_seed (g_get_monotonic_time ());

  ClutterColor color;
  /* use range (0, 200) so that the color will never be white, because client
   * is white */
  clutter_color_init (&color,
                      g_rand_int_range (rand, 0, 200),
                      g_rand_int_range (rand, 0, 200),
                      g_rand_int_range (rand, 0, 200),
                      255);

  clutter_actor_set_background_color (actor, &color);

  g_rand_free (rand);
}

/* taken from default plugin, slightly changed */
static void
on_monitors_changed (MetaScreen *screen,
                     MetaPlugin *plugin)
{
  MetaWaylandTestPlugin *self = META_WAYLAND_TEST_PLUGIN (plugin);
  MetaRectangle rect;
  ClutterActor *background;
  int i, n;

  clutter_actor_destroy_all_children (self->priv->background_group);

  n = meta_screen_get_n_monitors (screen);
  for (i = 0; i < n; i++)
    {
      meta_screen_get_monitor_geometry (screen, i, &rect);

      background = meta_background_actor_new ();
      set_background(background);

      clutter_actor_set_position (background, rect.x, rect.y);
      clutter_actor_set_size (background, rect.width, rect.height);
      clutter_actor_add_child (self->priv->background_group, background);
    }
}

static void
start (MetaPlugin *plugin)
{
  MetaWaylandTestPlugin *self = META_WAYLAND_TEST_PLUGIN (plugin);
  MetaScreen *screen = meta_plugin_get_screen (plugin);

  self->priv->background_group = meta_background_group_new ();
  clutter_actor_insert_child_below (meta_get_window_group_for_screen (screen),
                                    self->priv->background_group, NULL);

  g_signal_connect (screen, "monitors-changed",
                    G_CALLBACK (on_monitors_changed), plugin);
  on_monitors_changed (screen, plugin);

  clutter_actor_show (meta_get_stage_for_screen (screen));
}
