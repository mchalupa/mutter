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

#define MUTTER_WAYLAND_READY_PIPE "MUTTER_WAYLAND_READY_PIPE"

/* sizeof signal == sizeof int */
#define MUTTER_WAYLAND_READY_SIGNAL 0xa

/* this environment variable affects behaviour of the test. If it's set to
 *  "none": then tests MUTTER_TEST and MUTTER_FAIL_TEST won't spawn mutter
 *          at all (useful when tester has mutter already running)
 *  "once": spawn mutter only once for all testcases in test
 */
#define MUTTER_TEST_SPAWN_MUTTER "MUTTER_TEST_SPAWN_MUTTER"

struct mutter_test {
  const char *name;
  GTestFunc func;

  gboolean spawn_mutter;
  gboolean should_fail;
} __attribute__ ((aligned (16)));

/* inspired by wayland's test-runner.h, thanks */
#define CREATE_TEST(name, spawn, sf)                  \
  static void name(void);                             \
                                                      \
  const struct mutter_test test##name                 \
    __attribute__ ((section ("test_section"))) = {    \
    #name, name, (spawn), (sf)                        \
  };                                                  \
                                                      \
  static void name(void)

/* TEST is test that don't need mutter running */
#define TEST(name) CREATE_TEST(name, FALSE, FALSE)

/* FAIL_TEST is test that don't need mutter running and should fail */
#define FAIL_TEST(name) CREATE_TEST(name, FALSE, TRUE)

/* MUTTER_TEST needs mutter running */
#define MUTTER_TEST(name) CREATE_TEST(name, TRUE, FALSE)

/* MUTTER_FAIL_TEST needs mutter running and should fail */
#define MUTTER_FAIL_TEST(name) CREATE_TEST(name, TRUE, TRUE)
