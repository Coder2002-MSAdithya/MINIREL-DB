#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CreateFromAttrList(const char *dstRelName, AttrDesc *head)
{
    if (!dstRelName || !head)
        return NOTOK;

    // Count number of attributes in the linked list
    int numAttrs = 0;
    for (AttrDesc *p = head; p; p = p->next)
        numAttrs++;

    // argc = 2 (for "create" and dstRelName) + 2 * numAttrs (name + format for each attribute)
    int createArgc = 2 + 2 * numAttrs;

    // Allocate argv array (+1 for NULL terminator)
    char **createArgv = (char **)malloc((createArgc + 1) * sizeof(char *));
    if (!createArgv)
    {
        db_err_code = MEM_ALLOC_ERROR;
        return NOTOK;
    }

    createArgv[0] = strdup("_create");
    createArgv[1] = strdup(dstRelName);

    int i = 2;
    for (AttrDesc *p = head; p; p = p->next)
    {
        // Attribute name
        createArgv[i++] = strdup(p->attr.attrName);

        // Build attribute format string
        char format[5];
        switch (p->attr.type[0])
        {
            case 'i':
                strcpy(format, "i");
                break;
            case 'f':
                strcpy(format, "f");
                break;
            case 's':
                snprintf(format, sizeof(format), "s%d", p->attr.length);
                break;
            default:
                format[0] = '\0';
                break;
        }

        createArgv[i++] = strdup(format);
    }

    // NIL terminator
    createArgv[i] = NULL;

    // ---- Call Create() ----
    int status = Create(createArgc, createArgv);

    // ---- Free all allocated memory ----
    for (int j = 0; j < createArgc; j++)
        free(createArgv[j]);

    free(createArgv);

    return status;
}