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

#include <wayland-server.h>

#include "wayland-test-server-protocol.h"
#include "meta-wayland-private.h"
#include "meta-background-actor.h"
#include "meta-background-group.h"
#include "meta-backend.h"
#include "../mutter-test-runner.h"

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

  gint filter_id;
  gint idle_id;
  gint pipe_fd;

  ClutterActor *background_group;
  ClutterActor *stage;
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
  MetaWaylandTestPluginPrivate *priv = META_WAYLAND_TEST_PLUGIN (object)->priv;

  clutter_event_remove_filter (priv->filter_id);

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

static gboolean
notify_parent (gpointer data);

static gint
get_ready_pipe (void);

static void
bind_test_object (struct wl_client *client,
                  void *data, uint32_t version, uint32_t id);

static gboolean
event_cb (const ClutterEvent *event, gpointer data);

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
  priv->stage = meta_backend_get_stage (meta_get_backend ());

  if (wl_global_create (priv->compositor->wayland_display,
                        &wl_test_interface, 1, self, bind_test_object) == NULL)
    g_error ("Global (wl_test) initialization failed");

  priv->filter_id
        = clutter_event_add_filter((ClutterStage *) priv->stage,
                                   event_cb, NULL, self);

  /* get filedescriptor of pipe from parent (from the test that runs mutter)
   * so that mutter can notify parent that it's ready */
  priv->pipe_fd = get_ready_pipe ();
  if (priv->pipe_fd == -1)
    {
      /* keep mutter running in the case that user want do manual testing */
      g_warning ("Mutter didn't find any pipe to connect on. Keep running.");
      return;
    }

  priv->idle_id = meta_later_add (META_LATER_IDLE, notify_parent, self, NULL);
}

static void
set_background (MetaBackground *background, ClutterActor *background_actor)
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

  meta_background_set_color (background, &color);
  meta_background_actor_set_background (META_BACKGROUND_ACTOR (background_actor),
                                        background);

  g_rand_free (rand);
}

/* taken from default plugin, slightly changed */
static void
on_monitors_changed (MetaScreen *screen,
                     MetaPlugin *plugin)
{
  MetaWaylandTestPlugin *self = META_WAYLAND_TEST_PLUGIN (plugin);
  MetaRectangle rect;
  MetaBackground *background;
  ClutterActor *background_actor;
  int i, n;

  clutter_actor_destroy_all_children (self->priv->background_group);

  n = meta_screen_get_n_monitors (screen);
  for (i = 0; i < n; i++)
    {
      meta_screen_get_monitor_geometry (screen, i, &rect);

      background_actor = meta_background_actor_new (screen, i);

      clutter_actor_set_position (background_actor, rect.x, rect.y);
      clutter_actor_set_size (background_actor, rect.width, rect.height);

      background = meta_background_new (screen);
      set_background (background, background_actor);

      g_object_unref (background);

      clutter_actor_add_child (self->priv->background_group, background_actor);
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

static gint
get_ready_pipe (void)
{
  const gchar *pipe;
  gchar *end;
  gint fd = -1;

  pipe = g_getenv (MUTTER_WAYLAND_READY_PIPE);
  if (pipe)
    {
      fd = g_ascii_strtoll (pipe, &end, 0);
      g_assert (*end == '\0');
    }

  return fd;
}

static gboolean
notify_parent (gpointer data)
{
  MetaWaylandTestPlugin *plugin = data;
  MetaWaylandTestPluginPrivate *priv = plugin->priv;
  ssize_t n;
  int val = MUTTER_WAYLAND_READY_SIGNAL;

  g_assert (priv->pipe_fd >= 0);

  n = write (priv->pipe_fd, &val, sizeof (val));
  g_assert (n == sizeof (val) && "Writing to socket failed");

  /* clean up */
  close (priv->pipe_fd);
  priv->pipe_fd = -1;

  meta_later_remove (priv->idle_id);
  priv->idle_id = 0;

  return TRUE;
}

static gboolean
event_cb (const ClutterEvent *event, gpointer data)
{
  ClutterPoint p;
  MetaWaylandTestPlugin *plugin = data;
  ClutterInputDevice *dev;

  /* The filter is added before binding to wl_test, because we need to add
   * it before any meta filter. That means that the resource does not need
   * to exist yet */
  if (plugin->priv->test_object_resource == NULL)
    return CLUTTER_EVENT_PROPAGATE;

  if (clutter_event_type (event) == CLUTTER_MOTION)
    {
      dev = plugin->priv->compositor->seat->pointer.device;
      g_assert(dev);

      clutter_input_device_get_coords (dev, NULL, &p);

      wl_test_send_pointer_position (plugin->priv->test_object_resource,
                                      wl_fixed_from_double (p.x),
                                      wl_fixed_from_double (p.y));
    }

  /* we always must return this */
  return CLUTTER_EVENT_PROPAGATE;
}

static void
destroy_test_object_resource (struct wl_resource *resource)
{
  MetaWaylandTestPlugin *plugin = wl_resource_get_user_data (resource);

  plugin->priv->test_object_resource = NULL;
}

extern const struct wl_test_interface wl_test_implementation;

static void
bind_test_object (struct wl_client *client,
                  void *data, uint32_t version, uint32_t id)
{
  MetaWaylandTestPlugin *plugin = data;
  struct wl_resource *res;

  res = wl_resource_create (client, &wl_test_interface, 1, id);
  if (!res)
    {
      wl_client_post_no_memory (client);
      return;
    }

  wl_resource_set_implementation (res, &wl_test_implementation, plugin,
                                  destroy_test_object_resource);

  plugin->priv->test_object_resource = res;
}
