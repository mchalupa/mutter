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

#include <stdlib.h>
#include <unistd.h>

#include "mutter-test-runner.h"

MUTTER_TEST (empty)
{
}

MUTTER_TEST (just_exit)
{
  exit (0);
}

MUTTER_FAIL_TEST (g_assert_tst)
{
  g_assert (0);
}

MUTTER_FAIL_TEST (exit_1_tst)
{
  exit (1);
}

MUTTER_FAIL_TEST (kill_test)
{
  kill(getpid(), SIGKILL);
}

MUTTER_FAIL_TEST (sigterm_test)
{
  kill(getpid(), SIGTERM);
}

MUTTER_FAIL_TEST (sigsegv_tst)
{
  volatile char **ptr = NULL;

  *ptr = "Goodbye, cruel world!";
}

MUTTER_FAIL_TEST (abort_tst)
{
  abort ();
}

MUTTER_FAIL_TEST (timeout_tst)
{
  sleep (15);
}
