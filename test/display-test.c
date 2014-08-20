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

#include "config.h"

#include <glib.h>

#include "mutter-test-runner.h"

#ifndef SN_API_NOT_YET_FROZEN
#define SN_API_NOT_YET_FROZEN
#endif

#include "main.h"
#include "display-private.h"
#include "util-private.h"
#include "meta-plugin-manager.h"

static void
init_meta_display(int argc, char *argv[])
{
  GOptionContext *ctx;
  GError *error = NULL;
  const gchar *plugin = "default";

  /* this code is mostly copied from mutter.c from main function */
  ctx = meta_get_option_context ();
  if (!g_option_context_parse (ctx, &argc, &argv,  &error))
    {
      g_error ("mutter: %s\n", error->message);
    }
  g_option_context_free (ctx);

  if (plugin)
    meta_plugin_manager_load (plugin);

  meta_init ();
  meta_prefs_init ();
  meta_display_open ();

  g_assert (meta_get_display ()
            && "Initializing meta display failed");
}

MUTTER_TEST(test1)
{
  int argc = 2;
  char *argv[3] =
  {
    "mutter",
    "--wayland",
    NULL
  };

  init_meta_display (argc, argv);
}
