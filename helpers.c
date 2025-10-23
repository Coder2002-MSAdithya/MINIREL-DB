#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "include/defs.h"
#include "include/helpers.h"
#define BYTES_PER_LINE 16

int ceil_div(int a, int b)
{
    return (a+b-1)/b;
}

bool isValidPath(const char *path)
{
    // Empty path is invalid
    if (path == NULL || *path == '\0')
        return false;

    const char *p = path;

    while (*p)
    {
        // Skip leading slashes (allow multiple consecutive /)
        while (*p == '/')
            p++;

        if (*p == '\0')
            break; // trailing slashes are fine

        // First character of component must be alphabet
        if (!isalpha((unsigned char)*p))
            return false;

        p++;

        // Rest of the component: only alphanumeric
        while (*p && *p != '/')
        {
            if (!isalnum((unsigned char)*p))
                return false;
            p++;
        }
    }

    return true;
}

int remove_all_entry(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == -1)
    {
        /* If the file doesn't exist any more, treat as success for this entry. */
        if (errno == ENOENT)
        {
            return 0;
        }
        fprintf(stderr, "lstat(%s) failed: %s\n", path, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode))
    {
        /* Directory: recurse into it, remove its contents, then remove the directory itself. */
        DIR *d = opendir(path);
        if (d == NULL)
        {
            fprintf(stderr, "opendir(%s) failed: %s\n", path, strerror(errno));
            return -1;
        }

        struct dirent *entry;
        int ret = 0;

        while ((entry = readdir(d)) != NULL)
        {
            /* Skip "." and ".." */
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            /* Build full child path */
            char child[MAX_PATH_LENGTH];
            int n = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
            if (n < 0 || (size_t)n >= sizeof(child))
            {
                fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
                ret = -1;
                continue;
            }

            if (remove_all_entry(child) != 0)
            {
                /* keep going but record failure */
                ret = -1;
            }
        }

        closedir(d);

        /* After removing contents, remove the directory itself. */
        if (rmdir(path) == -1)
        {
            fprintf(stderr, "rmdir(%s) failed: %s\n", path, strerror(errno));
            return -1;
        }

        return ret;
    }
    else
    {
        /* Not a directory (regular file, symlink, socket, fifo, device): unlink it. */
        if (unlink(path) == -1)
        {
            fprintf(stderr, "unlink(%s) failed: %s\n", path, strerror(errno));
            return -1;
        }
        return 0;
    }
}

/*
 * remove_contents(dirpath)
 *   Removes all entries inside dirpath but does not remove dirpath itself.
 *   Returns 0 if everything removed successfully, non-zero if any error occurred.
 */
int remove_contents(const char *dirpath)
{
    struct stat st;
    if (lstat(dirpath, &st) == -1)
    {
        fprintf(stderr, "lstat(%s) failed: %s\n", dirpath, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "%s is not a directory\n", dirpath);
        return -1;
    }

    DIR *d = opendir(dirpath);
    if (d == NULL)
    {
        fprintf(stderr, "opendir(%s) failed: %s\n", dirpath, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    int overall_ret = 0;

    while ((entry = readdir(d)) != NULL)
    {
        /* Skip "." and ".." */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[MAX_PATH_LENGTH];
        int n = snprintf(child, sizeof(child), "%s/%s", dirpath, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(child))
        {
            fprintf(stderr, "Path too long: %s/%s\n", dirpath, entry->d_name);
            overall_ret = -1;
            continue;
        }

        if (remove_all_entry(child) != 0)
        {
            overall_ret = -1;
        }
    }

    closedir(d);
    return overall_ret;
}


void print_page_hex(const char *buf) 
{
    for (int line = 0; line < PAGESIZE / BYTES_PER_LINE; ++line) {
        int base = line * BYTES_PER_LINE;
        /* print line number (decimal, zero-padded to 2 digits) */
        printf("%02X: ", line);
        /* print 16 bytes in hex */
        for (int i = 0; i < BYTES_PER_LINE; ++i) {
            printf("%02X", (unsigned char) buf[base + i]);
            if (i + 1 < BYTES_PER_LINE) putchar(' ');
        }
        putchar('\n');
    }
}