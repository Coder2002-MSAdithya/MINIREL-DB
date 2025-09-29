#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

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