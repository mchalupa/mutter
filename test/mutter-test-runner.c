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
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "mutter-test-runner.h"

/* keeps information about running mutter */
struct mutter_info
{
  GPid pid;
  gint pipe;
};

/* each test needs environment. Get it once and keep it here */
static gchar **environ = NULL;

/* set this to 1 if the mutter-wayland should be run only once for all tests */
static gint run_once = 0;
/* set this to 1 if mutter should not run at all (tester has mtter instance
 * running already */
static gint dont_run_mutter = 0;

/* default timeout */
static gint timeout = 10; /* seconds */

static const gchar *
find_mutter (void)
{
  const gchar *local = "../src/.libs/mutter";
  if (g_file_test (local, G_FILE_TEST_EXISTS))
    {
      return local;
    }
  else
    {
      /* let g_spawn_async find mutter in the PATH */
      return "mutter";
    }
}

static gchar *
find_plugin (void)
{
  const gchar *local = "testplugin/.libs/mutter-wayland-test.so";
  gchar *current_dir;
  gchar *ret;

  if (g_file_test (local, G_FILE_TEST_EXISTS))
    {
      current_dir = g_get_current_dir ();
      ret = g_build_filename (current_dir, local, NULL);
      g_free (current_dir);
    }
  else
    {
      /* let mutter search in default directories */
      ret = g_strdup ("mutter-wayland-test");
    }

  return ret;
}

static void
spawn_mutter (struct mutter_info *m)
{
  int fd[2];
  GError *error = NULL;
  gchar fdstr[4];

  const gchar *path = find_mutter ();
  gchar *plugin = find_plugin ();
  gchar *plugin_arg = g_strdup_printf ("--mutter-plugin=%s", plugin);

  gchar *args[] =
  {
    (gchar *) path,
    "--wayland",
    plugin_arg,
    NULL
  };

  if (pipe (fd) != 0)
    g_error ("Failed creating pipe");

  g_snprintf (fdstr, sizeof (fdstr), "%d", fd[1]);
  m->pipe = fd[0];

  environ = g_environ_setenv (environ, MUTTER_WAYLAND_READY_PIPE, fdstr, TRUE);
  GSpawnFlags flags = G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD
                      | G_SPAWN_SEARCH_PATH_FROM_ENVP;

  if (!g_spawn_async (NULL, /* use current directory */
                      args,
                      environ,
                      flags,
                      NULL, /* child setup */
                      NULL, /* user data */
                      &m->pid,
                      &error))
    g_error ("Failed spawning mutter: %s", error->message);

  g_debug ("Spawned mutter [pid %d]: %s --wayland %s", m->pid, path, plugin_arg);

  close (fd[1]);
  g_free (plugin);
  g_free (plugin_arg);
}

static void
stop_and_check_mutter (struct mutter_info *m)
{
  gint status;
  gint wstat;

  wstat = waitpid (m->pid, &status, WNOHANG);

  /* check if mutter did not crash */
  switch (wstat)
    {
    case -1:
      g_error ("Failed waiting for mutter");
      break;
    case 0: /* all OK, not exited yet */
      kill (m->pid, SIGTERM);

      /* zombies can kill us all! */
      waitpid (m->pid, &status, 0);
      break;
    default:
      /* mutter should be still running if everything went ok. The test failed */
      g_test_message ("failed: mutter prematurely exited with status %d",
                      WEXITSTATUS (status));
      g_test_fail ();
    }

    g_spawn_close_pid (m->pid);
}

static void
wait_for_mutter_initialization (struct mutter_info *m)
{
  ssize_t n;
  gint val;

  n = read (m->pipe, &val, sizeof (int));
  g_assert (n == sizeof (int) && "Failed reading pipe");
  g_assert (val == MUTTER_WAYLAND_READY_SIGNAL);
}

static void
timeout_cb (int signum)
{
  g_assert (signum == SIGALRM);
  g_error ("Timeout");
}

static void
test_setup (gpointer fixture, gconstpointer data)
{
  struct mutter_info *m = fixture;
  const struct mutter_test *t = data;

  if (!t->spawn_mutter || run_once || dont_run_mutter)
    return;

  spawn_mutter (m);
  wait_for_mutter_initialization (m);
}

static void
test_teardown (gpointer fixture, gconstpointer data)
{
  struct mutter_info *m = fixture;
  const struct mutter_test *t = data;

  if (!t->spawn_mutter || run_once || dont_run_mutter)
    return;

  g_assert (m->pid > 1);
  stop_and_check_mutter (m);
}

static void
test_run (gpointer fixture, gconstpointer data)
{
  struct mutter_info *m = fixture;
  const struct mutter_test *t = data;
  pid_t pid;
  int status;

  if (t->spawn_mutter && m->pid < 2 && !(run_once || dont_run_mutter))
    g_error ("Mutter seems not to be spawned although it should be");

  /* I considered using g_test_trap_subprocess, but when I use it, then
   * I have no other way how to check exit status than to use
   * g_trap_assert_passed (failed) and that will abort the test before calling
   * test_teardown so the mutter won't be killed and blocks other tests */
  if ((pid = fork ()) == -1)
    g_error ("Fork failed");

  if (pid == 0)
    {
      if (timeout)
        {
          /* ther's no main loop for using g_timeout_add_seconds */
          signal (SIGALRM, timeout_cb);
          alarm (timeout);
        }

      t->func ();
      exit (0);
    }

  waitpid (pid, &status, 0);

  /* check results */
  if (!WIFEXITED (status) || WEXITSTATUS (status) != EXIT_SUCCESS)
    {
      if (t->should_fail)
        {
          g_test_message ("This test should fail, all OK");
        }
      else
        {
          g_test_message ("Test did not exit normally. Exit status: %d",
                          WEXITSTATUS (status));
          /* must not abort, or test_teardown wont be called */
          g_test_fail ();
        }
    }
  else if (t->should_fail)
    {
      g_test_message ("This test should have failed. Not OK");
      g_test_fail ();
    }
}

extern const struct mutter_test __start_test_section, __stop_test_section;

static void
add_tests (void)
{
  GTestCase *cs;
  const struct mutter_test *t;

  GTestSuite *suite = g_test_get_root ();

  for (t = &__start_test_section; t < &__stop_test_section; ++t)
    {
      cs = g_test_create_case (t->name, sizeof (struct mutter_info), t,
                               test_setup,
                               test_run,
                               test_teardown);

      g_test_suite_add (suite, cs);
    }
}

static void
cleanup (void)
{
  g_strfreev (environ);
}

int main (int argc, char *argv[])
{
  gint status;
  const gchar *spawn_no;
  const gchar *timeout_env;
  struct mutter_info m = {0};

  g_test_init (&argc, &argv, NULL);
  environ = g_get_environ ();

  timeout_env = g_environ_getenv (environ, MUTTER_TEST_TIMEOUT);
  if (timeout_env)
    {
      gchar *end;
      timeout = g_ascii_strtoll (timeout_env, &end, 0);
      g_assert (*end == '\0' && "Timeout is not a number");
    }

  spawn_no = g_environ_getenv (environ, MUTTER_TEST_SPAWN_MUTTER);
  if (spawn_no)
    {
      if (g_strcmp0 (spawn_no, "once") == 0 || g_strcmp0 (spawn_no, "1") == 0)
        {
          run_once = 1;

          spawn_mutter (&m);
          wait_for_mutter_initialization (&m);
        }
      else if (g_strcmp0 (spawn_no, "none") == 0 || g_strcmp0 (spawn_no, "0") == 0)
        {
          /* ok, here it would be easier to suppress running mutter and set
           * run_once = 1, but it would not be so clear, so do it explicitely */
          dont_run_mutter = 1;
        }
    }

  add_tests ();
  status = g_test_run ();

  if (run_once)
    stop_and_check_mutter (&m);

  cleanup ();

  return status;
}
