/*
 * Some utility routines for writing tests.
 *
 * Here are a variety of utility routines for writing tests compatible with
 * the TAP protocol.  All routines of the form ok() or is*() take a test
 * number and some number of appropriate arguments, check to be sure the
 * results match the expected output using the arguments, and print out
 * something appropriate for that test number.  Other utility routines help in
 * constructing more complex tests, skipping tests, reporting errors, setting
 * up the TAP output format, or finding things in the test environment.
 *
 * This file is part of C TAP Harness.  The current version plus supporting
 * documentation is at <http://www.eyrie.org/~eagle/software/c-tap-harness/>.
 *
 * Copyright 2009, 2010, 2011, 2012, 2013 Russ Allbery <eagle@eyrie.org>
 * Copyright 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2011, 2012, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <direct.h>
#else
# include <sys/stat.h>
#endif
#include <sys/types.h>
#include <unistd.h>

#include <tests/tap/basic.h>

/* Windows provides mkdir and rmdir under different names. */
#ifdef _WIN32
# define mkdir(p, m) _mkdir(p)
# define rmdir(p)    _rmdir(p)
#endif

/*
 * The test count.  Always contains the number that will be used for the next
 * test status.  This is exported to callers of the library.
 */
unsigned long testnum = 1;

/*
 * Status information stored so that we can give a test summary at the end of
 * the test case.  We store the planned final test and the count of failures.
 * We can get the highest test count from testnum.
 */
static unsigned long _planned = 0;
static unsigned long _failed  = 0;

/*
 * Store the PID of the process that called plan() and only summarize
 * results when that process exits, so as to not misreport results in forked
 * processes.
 */
static pid_t _process = 0;

/*
 * If true, we're doing lazy planning and will print out the plan based on the
 * last test number at the end of testing.
 */
static int _lazy = 0;

/*
 * If true, the test was aborted by calling bail().  Currently, this is only
 * used to ensure that we pass a false value to any cleanup functions even if
 * all tests to that point have passed.
 */
static int _aborted = 0;

/*
 * Registered cleanup functions.  These are stored as a linked list and run in
 * registered order by finish when the test program exits.  Each function is
 * passed a boolean value indicating whether all tests were successful.
 */
struct cleanup_func {
    test_cleanup_func func;
    struct cleanup_func *next;
};
static struct cleanup_func *cleanup_funcs = NULL;

/*
 * Print a specified prefix and then the test description.  Handles turning
 * the argument list into a va_args structure suitable for passing to
 * print_desc, which has to be done in a macro.  Assumes that format is the
 * argument immediately before the variadic arguments.
 */
#define PRINT_DESC(prefix, format)              \
    do {                                        \
        if (format != NULL) {                   \
            va_list args;                       \
            if (prefix != NULL)                 \
                printf("%s", prefix);           \
            va_start(args, format);             \
            vprintf(format, args);              \
            va_end(args);                       \
        }                                       \
    } while (0)


/*
 * Our exit handler.  Called on completion of the test to report a summary of
 * results provided we're still in the original process.  This also handles
 * printing out the plan if we used plan_lazy(), although that's suppressed if
 * we never ran a test (due to an early bail, for example), and running any
 * registered cleanup functions.
 */
static void
finish(void)
{
    int success;
    struct cleanup_func *current;
    unsigned long highest = testnum - 1;

    /*
     * Don't do anything except free the cleanup functions if we aren't the
     * primary process (the process in which plan or plan_lazy was called).
     */
    if (_process != 0 && getpid() != _process) {
        while (cleanup_funcs != NULL) {
            current = cleanup_funcs;
            cleanup_funcs = cleanup_funcs->next;
            free(current);
        }
        return;
    }

    /*
     * Determine whether all tests were successful, which is needed before
     * calling cleanup functions since we pass that fact to the functions.
     */
    if (_planned == 0 && _lazy)
        _planned = highest;
    success = (!_aborted && _planned == highest && _failed == 0);

    /*
     * If there are any registered cleanup functions, we run those first.  We
     * always run them, even if we didn't run a test.
     */
    while (cleanup_funcs != NULL) {
        cleanup_funcs->func(success);
        current = cleanup_funcs;
        cleanup_funcs = cleanup_funcs->next;
        free(current);
    }

    /* Don't do anything further if we never planned a test. */
    if (_planned == 0)
        return;

    /* If we're aborting due to bail, don't print summaries. */
    if (_aborted)
        return;

    /* Print out the lazy plan if needed. */
    fflush(stderr);
    if (_lazy && _planned > 0)
        printf("1..%lu\n", _planned);

    /* Print out a summary of the results. */
    if (_planned > highest)
        diag("Looks like you planned %lu test%s but only ran %lu", _planned,
             (_planned > 1 ? "s" : ""), highest);
    else if (_planned < highest)
        diag("Looks like you planned %lu test%s but ran %lu extra", _planned,
             (_planned > 1 ? "s" : ""), highest - _planned);
    else if (_failed > 0)
        diag("Looks like you failed %lu test%s of %lu", _failed,
             (_failed > 1 ? "s" : ""), _planned);
    else if (_planned != 1)
        diag("All %lu tests successful or skipped", _planned);
    else
        diag("%lu test successful or skipped", _planned);
}


/*
 * Initialize things.  Turns on line buffering on stdout and then prints out
 * the number of tests in the test suite.
 */
void
plan(unsigned long count)
{
    if (setvbuf(stdout, NULL, _IOLBF, BUFSIZ) != 0)
        sysdiag("cannot set stdout to line buffered");
    fflush(stderr);
    printf("1..%lu\n", count);
    testnum = 1;
    _planned = count;
    _process = getpid();
    if (atexit(finish) != 0) {
        sysdiag("cannot register exit handler");
        diag("cleanups will not be run");
    }
}


/*
 * Initialize things for lazy planning, where we'll automatically print out a
 * plan at the end of the program.  Turns on line buffering on stdout as well.
 */
void
plan_lazy(void)
{
    if (setvbuf(stdout, NULL, _IOLBF, BUFSIZ) != 0)
        sysdiag("cannot set stdout to line buffered");
    testnum = 1;
    _process = getpid();
    _lazy = 1;
    if (atexit(finish) != 0)
        sysbail("cannot register exit handler to display plan");
}


/*
 * Skip the entire test suite and exits.  Should be called instead of plan(),
 * not after it, since it prints out a special plan line.
 */
void
skip_all(const char *format, ...)
{
    fflush(stderr);
    printf("1..0 # skip");
    PRINT_DESC(" ", format);
    putchar('\n');
    exit(0);
}


/*
 * Takes a boolean success value and assumes the test passes if that value
 * is true and fails if that value is false.
 */
void
ok(int success, const char *format, ...)
{
    fflush(stderr);
    printf("%sok %lu", success ? "" : "not ", testnum++);
    if (!success)
        _failed++;
    PRINT_DESC(" - ", format);
    putchar('\n');
}


/*
 * Same as ok(), but takes the format arguments as a va_list.
 */
void
okv(int success, const char *format, va_list args)
{
    fflush(stderr);
    printf("%sok %lu", success ? "" : "not ", testnum++);
    if (!success)
        _failed++;
    if (format != NULL) {
        printf(" - ");
        vprintf(format, args);
    }
    putchar('\n');
}


/*
 * Skip a test.
 */
void
skip(const char *reason, ...)
{
    fflush(stderr);
    printf("ok %lu # skip", testnum++);
    PRINT_DESC(" ", reason);
    putchar('\n');
}


/*
 * Report the same status on the next count tests.
 */
void
ok_block(unsigned long count, int status, const char *format, ...)
{
    unsigned long i;

    fflush(stderr);
    for (i = 0; i < count; i++) {
        printf("%sok %lu", status ? "" : "not ", testnum++);
        if (!status)
            _failed++;
        PRINT_DESC(" - ", format);
        putchar('\n');
    }
}


/*
 * Skip the next count tests.
 */
void
skip_block(unsigned long count, const char *reason, ...)
{
    unsigned long i;

    fflush(stderr);
    for (i = 0; i < count; i++) {
        printf("ok %lu # skip", testnum++);
        PRINT_DESC(" ", reason);
        putchar('\n');
    }
}


/*
 * Takes an expected integer and a seen integer and assumes the test passes
 * if those two numbers match.
 */
void
is_int(long wanted, long seen, const char *format, ...)
{
    fflush(stderr);
    if (wanted == seen)
        printf("ok %lu", testnum++);
    else {
        diag("wanted: %ld", wanted);
        diag("  seen: %ld", seen);
        printf("not ok %lu", testnum++);
        _failed++;
    }
    PRINT_DESC(" - ", format);
    putchar('\n');
}


/*
 * Takes a string and what the string should be, and assumes the test passes
 * if those strings match (using strcmp).
 */
void
is_string(const char *wanted, const char *seen, const char *format, ...)
{
    if (wanted == NULL)
        wanted = "(null)";
    if (seen == NULL)
        seen = "(null)";
    fflush(stderr);
    if (strcmp(wanted, seen) == 0)
        printf("ok %lu", testnum++);
    else {
        diag("wanted: %s", wanted);
        diag("  seen: %s", seen);
        printf("not ok %lu", testnum++);
        _failed++;
    }
    PRINT_DESC(" - ", format);
    putchar('\n');
}


/*
 * Takes an expected unsigned long and a seen unsigned long and assumes the
 * test passes if the two numbers match.  Otherwise, reports them in hex.
 */
void
is_hex(unsigned long wanted, unsigned long seen, const char *format, ...)
{
    fflush(stderr);
    if (wanted == seen)
        printf("ok %lu", testnum++);
    else {
        diag("wanted: %lx", (unsigned long) wanted);
        diag("  seen: %lx", (unsigned long) seen);
        printf("not ok %lu", testnum++);
        _failed++;
    }
    PRINT_DESC(" - ", format);
    putchar('\n');
}


/*
 * Bail out with an error.
 */
void
bail(const char *format, ...)
{
    va_list args;

    _aborted = 1;
    fflush(stderr);
    fflush(stdout);
    printf("Bail out! ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    exit(255);
}


/*
 * Bail out with an error, appending strerror(errno).
 */
void
sysbail(const char *format, ...)
{
    va_list args;
    int oerrno = errno;

    _aborted = 1;
    fflush(stderr);
    fflush(stdout);
    printf("Bail out! ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf(": %s\n", strerror(oerrno));
    exit(255);
}


/*
 * Report a diagnostic to stderr.
 */
void
diag(const char *format, ...)
{
    va_list args;

    fflush(stderr);
    fflush(stdout);
    printf("# ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}


/*
 * Report a diagnostic to stderr, appending strerror(errno).
 */
void
sysdiag(const char *format, ...)
{
    va_list args;
    int oerrno = errno;

    fflush(stderr);
    fflush(stdout);
    printf("# ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf(": %s\n", strerror(oerrno));
}


/*
 * Allocate cleared memory, reporting a fatal error with bail on failure.
 */
void *
bcalloc(size_t n, size_t size)
{
    void *p;

    p = calloc(n, size);
    if (p == NULL)
        sysbail("failed to calloc %lu", (unsigned long)(n * size));
    return p;
}


/*
 * Allocate memory, reporting a fatal error with bail on failure.
 */
void *
bmalloc(size_t size)
{
    void *p;

    p = malloc(size);
    if (p == NULL)
        sysbail("failed to malloc %lu", (unsigned long) size);
    return p;
}


/*
 * Reallocate memory, reporting a fatal error with bail on failure.
 */
void *
brealloc(void *p, size_t size)
{
    p = realloc(p, size);
    if (p == NULL)
        sysbail("failed to realloc %lu bytes", (unsigned long) size);
    return p;
}


/*
 * Copy a string, reporting a fatal error with bail on failure.
 */
char *
bstrdup(const char *s)
{
    char *p;
    size_t len;

    len = strlen(s) + 1;
    p = malloc(len);
    if (p == NULL)
        sysbail("failed to strdup %lu bytes", (unsigned long) len);
    memcpy(p, s, len);
    return p;
}


/*
 * Copy up to n characters of a string, reporting a fatal error with bail on
 * failure.  Don't use the system strndup function, since it may not exist and
 * the TAP library doesn't assume any portability support.
 */
char *
bstrndup(const char *s, size_t n)
{
    const char *p;
    char *copy;
    size_t length;

    /* Don't assume that the source string is nul-terminated. */
    for (p = s; (size_t) (p - s) < n && *p != '\0'; p++)
        ;
    length = p - s;
    copy = malloc(length + 1);
    if (p == NULL)
        sysbail("failed to strndup %lu bytes", (unsigned long) length);
    memcpy(copy, s, length);
    copy[length] = '\0';
    return copy;
}


/*
 * Locate a test file.  Given the partial path to a file, look under BUILD and
 * then SOURCE for the file and return the full path to the file.  Returns
 * NULL if the file doesn't exist.  A non-NULL return should be freed with
 * test_file_path_free().
 *
 * This function uses sprintf because it attempts to be independent of all
 * other portability layers.  The use immediately after a memory allocation
 * should be safe without using snprintf or strlcpy/strlcat.
 */
char *
test_file_path(const char *file)
{
    char *base;
    char *path = NULL;
    size_t length;
    const char *envs[] = { "BUILD", "SOURCE", NULL };
    int i;

    for (i = 0; envs[i] != NULL; i++) {
        base = getenv(envs[i]);
        if (base == NULL)
            continue;
        length = strlen(base) + 1 + strlen(file) + 1;
        path = bmalloc(length);
        sprintf(path, "%s/%s", base, file);
        if (access(path, R_OK) == 0)
            break;
        free(path);
        path = NULL;
    }
    return path;
}


/*
 * Free a path returned from test_file_path().  This function exists primarily
 * for Windows, where memory must be freed from the same library domain that
 * it was allocated from.
 */
void
test_file_path_free(char *path)
{
    if (path != NULL)
        free(path);
}


/*
 * Create a temporary directory, tmp, under BUILD if set and the current
 * directory if it does not.  Returns the path to the temporary directory in
 * newly allocated memory, and calls bail on any failure.  The return value
 * should be freed with test_tmpdir_free.
 *
 * This function uses sprintf because it attempts to be independent of all
 * other portability layers.  The use immediately after a memory allocation
 * should be safe without using snprintf or strlcpy/strlcat.
 */
char *
test_tmpdir(void)
{
    const char *build;
    char *path = NULL;
    size_t length;

    build = getenv("BUILD");
    if (build == NULL)
        build = ".";
    length = strlen(build) + strlen("/tmp") + 1;
    path = bmalloc(length);
    sprintf(path, "%s/tmp", build);
    if (access(path, X_OK) < 0)
        if (mkdir(path, 0777) < 0)
            sysbail("error creating temporary directory %s", path);
    return path;
}


/*
 * Free a path returned from test_tmpdir() and attempt to remove the
 * directory.  If we can't delete the directory, don't worry; something else
 * that hasn't yet cleaned up may still be using it.
 */
void
test_tmpdir_free(char *path)
{
    rmdir(path);
    if (path != NULL)
        free(path);
}


/*
 * Register a cleanup function that is called when testing ends.  All such
 * registered functions will be run by finish.
 */
void
test_cleanup_register(test_cleanup_func func)
{
    struct cleanup_func *cleanup, **last;

    cleanup = bmalloc(sizeof(struct cleanup_func));
    cleanup->func = func;
    cleanup->next = NULL;
    last = &cleanup_funcs;
    while (*last != NULL)
        last = &(*last)->next;
    *last = cleanup;
}
