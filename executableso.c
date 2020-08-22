/**
 * This library implements the logic required to make a shared object
 * executable.
 *
 * Metadata:
 * - Author:  Eric Pruitt; <https://www.codevat.com>
 * - License: [BSD 2-Clause](http://opensource.org/licenses/BSD-2-Clause)
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

/**
 * Custom ELF interpreter entry that makes a shared object executable.
 */
const char elf_interp[] __attribute__((section(".interp"))) = ELF_INTERP;

/**
 * Read the command line for "/proc/self/cmdline" and generated values suitable
 * for use as "argc" and "argv" in a standard main function.
 *
 * Arguments:
 * - argc: Pointer to the destination where the number of arguments should be
 *   stored.
 * - argv: Pointer to the destination where the arguments should be stored. As
 *   with "argv" used for "main", this array is terminated with a NULL pointer.
 *
 * Returns: 0 if the arguments were successfully extracted or -1 otherwise with
 * errno set accordingly.
 */
static int cmdline(int *argc, char ***argv)
{
    FILE *stream;

    size_t len = 0;
    char *line = NULL;
    void *next = NULL;

    if (!(stream = fopen("/proc/self/cmdline", "r"))) {
        return -1;
    }

    *argc = 0;

    for (errno = 0; getdelim(&line, &len, '\0', stream) != -1; errno = 0) {
        if (!(next = realloc(*argv, (size_t) (*argc + 2) * sizeof(char *)))) {
            goto error;
        }

        *argv = next;
        (*argv)[(*argc)++] = line;
        line = NULL;
        len = 0;
    }

    if (!errno) {
        (*argv)[*argc] = NULL;
        return 0;
    }

error:
    for (; *argc > 0; (*argc)--) {
        free((*argv)[*argc - 1]);
    }

    free(line);
    free(*argv);
    return -1;
}

int main(void)
{
    char exe[PATH_MAX];
    char *path;
    char path_resolved[PATH_MAX];

    int argc = 0;
    char **argv = NULL;
    char *ld_preload = NULL;
    char *paths = NULL;

    if (cmdline(&argc, &argv)) {
        perror("cmdline");
        goto error;
    }

    if (argc < 2) {
        dprintf(STDERR_FILENO, "Usage: %s COMMAND [ARGUMENT]...\n", argv[0]);
    } else if (!realpath("/proc/self/exe", exe)) {
        perror("realpath: /proc/self/exe");
    } else {
        paths = getenv("LD_PRELOAD");

        if (!paths || paths[0] == '\0') {
            paths = NULL;
            ld_preload = strdup(exe);

            if (!ld_preload) {
                perror("strdup");
                goto error;
            }
        } else if (!(paths = strdup(paths))) {
            perror("strdup");
            goto error;
        } else {
            // Since strtok(3) may modify the underlying string, the new
            // LD_PRELOAD value is prepared in advance even though it might not
            // be used.
            ld_preload = malloc(strlen(paths) + /* ":" */ 1 + strlen(exe) + 1);

            if (!ld_preload) {
                perror("malloc");
                goto error;
            }

            stpcpy(stpcpy(stpcpy(ld_preload, paths), ":"), exe);

            for (path = strtok(paths, ":"); path; path = strtok(NULL, ":")) {
                if (!strcmp(exe, path) || (realpath(path, path_resolved) &&
                  !strcmp(exe, path_resolved))) {
                    goto exec;
                }
            }
        }

        if (setenv("LD_PRELOAD", ld_preload, 1)) {
            perror("setenv: LD_PRELOAD");
            goto error;
        }

exec:
        execvp(argv[1], argv + 1);
        perror("execvp");
    }

error:
    free(paths);
    free(ld_preload);
    _exit(255);
}