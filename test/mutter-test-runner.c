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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "mutter-test-runner.h"

/* this struct keeps information about running Mutter.
 * Mutter runs maximally in one instance for each test, so
 * we can keep it global (we can use it in signal handlers
 * cosequently) */
static struct
{
  GPid pid;
  gint pipe;
} mutter = {-1, -1};

/* each test needs environment. Get it once and keep it here */
static gchar **env = NULL;

/* when this macro is defined in the file with tests, the mutter won't
 * be spawned */
#ifdef DONT_RUN_MUTTER

/* set this to TRUE if mutter should not run at all (tester has mutter instance
 * running already */
static gboolean dont_run_mutter = TRUE;
#else
static gboolean dont_run_mutter = FALSE;
#endif /* DONT_SPAWN_MUTTER */

/* run on bare metal */
static gboolean run_native = FALSE;

/* don't fork testcases - usefull when running in debugger */
static gboolean no_fork = FALSE;

/* default timeout */
static gint timeout = 10; /* seconds */

static const gchar *
find_mutter (void)
{
  /* assume we're in test subdirectory (make check goes there) */
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
spawn_mutter ()
{
  int fd[2];
  GError *error = NULL;
  gchar fdstr[4];

  const gchar *path = find_mutter ();
  gchar *plugin = find_plugin ();

  gchar *plugin_arg = g_strdup_printf ("--mutter-plugin=%s", plugin);
  g_free (plugin);

  gchar *args[] =
  {
    (gchar *) path,
    "--wayland",
    plugin_arg,
    run_native ? "--display-server" : NULL,
    NULL
  };

  if (pipe (fd) != 0)
    g_error ("Failed creating pipe");

  mutter.pipe = fd[0];

  g_snprintf (fdstr, sizeof (fdstr), "%d", fd[1]);
  env = g_environ_setenv (env, MUTTER_WAYLAND_READY_PIPE, fdstr, TRUE);

  GSpawnFlags flags = G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD
                      | G_SPAWN_SEARCH_PATH_FROM_ENVP;

  if (!g_spawn_async (NULL, /* use current directory */
                      args,
                      env,
                      flags,
                      NULL, /* child setup */
                      NULL, /* user data */
                      &mutter.pid,
                      &error))
    g_error ("Failed spawning mutter: %s", error->message);

  g_debug ("Spawned mutter [pid %d]: %s --wayland %s %s", mutter.pid,
           path, run_native ? "--display-server" : "",  plugin_arg);

  close (fd[1]);
  g_free (plugin_arg);
}

static void
stop_and_check_mutter ()
{
  gint status;
  gint wstat;

  g_assert (mutter.pid != -1 && "Mutter is not running");

  wstat = waitpid (mutter.pid, &status, WNOHANG);

  /* check if mutter did not crash */
  switch (wstat)
    {
    case -1:
      g_error ("Failed waiting for mutter: %s", strerror (errno));
      break;
    case 0: /* all OK, not exited yet */
      kill (mutter.pid, SIGTERM);

      /* zombies can kill us all! */
      waitpid (mutter.pid, &status, 0);
      break;
    default:
      /* mutter should be still running if everything went ok. The test failed */
      g_warning ("Failed: mutter prematurely exited with status %d",
                 WEXITSTATUS (status));
      g_test_fail ();
    }

    g_spawn_close_pid (mutter.pid);
    mutter.pid = -1;
}

static void
wait_for_mutter_initialization ()
{
  ssize_t n;
  gint val;

  n = read (mutter.pipe, &val, sizeof (int));
  g_assert (n == sizeof (int) && "Failed reading pipe");
  g_assert (val == MUTTER_WAYLAND_READY_SIGNAL);
}

static void
test_setup (gpointer fixture, gconstpointer data)
{
  if (dont_run_mutter)
    {
      g_assert (mutter.pid == -1);
      g_assert (mutter.pipe == -1);
    }

	/* future extensions */
}

static void
test_teardown (gpointer fixture, gconstpointer data)
{
  if (dont_run_mutter)
    {
      g_assert (mutter.pid == -1);
      g_assert (mutter.pipe == -1);
    }
}

static void
check_results (siginfo_t *status, gboolean should_fail)
{
  switch (status->si_code)
  {
  case CLD_EXITED:
    if (status->si_status != EXIT_SUCCESS)
      {
        if (status->si_status == SKIP)
          {
            g_test_skip ("Test skipped");
            break;
          }

        if (should_fail)
            g_test_message ("Test should fail, OK");
        else
            g_test_fail ();
      }
    else
      {
        if (should_fail)
          {
            g_test_message ("Should failed");
            g_test_fail ();
          }
      }
    g_test_message ("Test exited with status: %d", status->si_status);
    break;
  case CLD_KILLED:
  case CLD_DUMPED:
    if (status->si_status == SIGALRM)
      g_test_message ("Test timeouted");
    else
      g_test_message ("Got signal %d", status->si_status);

    if (should_fail)
      g_test_message ("Should fail, OK");
    else
      g_test_fail ();

    break;
  default:
    g_error ("Unhandled return state");
  }
}

static void
timeout_cb (int signum)
{
  g_error ("Timeouted");
}

static void
set_timeout (gint t)
{
  struct sigaction sa;

  if (t)
    {
      g_debug ("Setting timeout to %d seconds", t);
      sa.sa_handler = timeout_cb;
      sigemptyset (&sa.sa_mask);
      sa.sa_flags = 0;

      g_assert (sigaction (SIGALRM, &sa, NULL) == 0);

      alarm (t);
    }
  else
    g_debug ("Timeout turned off");
}

static void
test_run (gpointer fixture, gconstpointer data)
{
  const struct mutter_test *t = data;
  pid_t pid;
  siginfo_t status;
  struct sigaction sa;

  if (!dont_run_mutter && mutter.pid < 2)
    g_error ("Mutter seems not to be spawned although it should be");

  if (no_fork)
    {
      set_timeout (timeout);
      t->func ();
    }
  else
    {
      pid = fork ();
      g_assert (pid != -1 && "Fork failed");

      if (pid == 0)
        {
          /* we set handler for SIGABRT in main,
           * but here we need it to be not set. */
          sa.sa_handler = SIG_DFL;
          sigemptyset (&sa.sa_mask);
          sa.sa_flags = 0;

          g_assert (sigaction (SIGABRT, &sa, NULL) == 0);
          g_assert (sigaction (SIGSEGV, &sa, NULL) == 0);

          set_timeout(timeout);
          t->func ();

          exit (0);
        }
      else
        {
          if (waitid (P_PID, pid, &status, WEXITED))
              g_error ("waitid failed: %s", strerror (errno));

          check_results (&status, t->should_fail);
        }
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
      cs = g_test_create_case (t->name, 0, t,
                               test_setup,
                               test_run,
                               test_teardown);

      g_test_suite_add (suite, cs);
    }
}

static void
cleanup (void)
{
  g_strfreev (env);
}

static void
get_setup_from_env ()
{
  const gchar *envar;

  envar = g_environ_getenv (env, MUTTER_TEST_SPAWN_MUTTER);
  if (envar)
    {
      if (g_strcmp0 (envar, "none") == 0)
          dont_run_mutter = TRUE;
      else if (g_strcmp0 (envar, "native") == 0)
          run_native = TRUE;
    }

  envar = g_environ_getenv (env, MUTTER_TEST_NO_FORK);
  if (envar)
    {
      no_fork = TRUE;
    }

  envar = g_environ_getenv (env, MUTTER_TEST_TIMEOUT);
  if (envar)
    {
      gchar *end;
      timeout = g_ascii_strtoll (envar, &end, 0);
      g_assert (*end == '\0' && "Timeout is not a number");
    }
}

/* when a test fails, then g_test_run () aborts program. That means
 * that stop_and_check_mutter () is not called and mutter keep running
 * until it is manually stopped. Prevent this by adding signal handler */
static void
sig_handler(int s)
{
  if (mutter.pid != -1)
    {
      g_debug ("Got SIGABRT, killing mutter");
      stop_and_check_mutter ();
      g_assert (!dont_run_mutter);
    }
}

int main (int argc, char *argv[])
{
  gint status;
  struct sigaction sa;
  int i;

  /* don't run mutter, when user is only listing the tests */
  for (i = 1; i < argc; ++i)
    if (g_strcmp0(argv[i], "-l") == 0)
      dont_run_mutter = TRUE;

  g_test_init (&argc, &argv, NULL);
  env = g_get_environ ();

  get_setup_from_env ();

  add_tests ();

  if (!dont_run_mutter)
    {
      spawn_mutter ();
      wait_for_mutter_initialization ();

      sa.sa_handler = sig_handler;
      sigemptyset (&sa.sa_mask);
      sa.sa_flags = 0;

      /* kill mutter on signal if it is running */
      g_assert (sigaction (SIGABRT, &sa, NULL) == 0);
      g_assert (sigaction (SIGSEGV, &sa, NULL) == 0);
    }

  status = g_test_run ();

  if (!dont_run_mutter)
    stop_and_check_mutter ();

  cleanup ();

  g_debug ("All done. Exititng...");

  return status;
}
