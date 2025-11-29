#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/closerel.h"
#include "../include/deleterec.h"
#include "../include/findrec.h"
#include "../include/freemap.h"   // for build_fmap_filename

int Destroy(int argc, char *argv[])
{
    if (!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (argc < 2)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (argc > 2)
    {
        db_err_code = TOO_MANY_ARGS;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    void *relCatRecPtr  = malloc(sizeof(RelCatRec));
    void *attrCatRecPtr = malloc(sizeof(AttrCatRec));
    Rid startRid = INVALID_RID;

    if (!relCatRecPtr || !attrCatRecPtr)
    {
        free(relCatRecPtr);
        free(attrCatRecPtr);
        db_err_code = MEM_ALLOC_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* Protect system catalogs */
    if (strncmp(relName, RELCAT, RELNAME) == OK ||
        strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        db_err_code = METADATA_SECURITY;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* Find relation in RelCat */
    int status = FindRec(RELCAT_CACHE, startRid, &startRid,
                         relCatRecPtr, 's', RELNAME,
                         offsetof(RelCatRec, relName),
                         relName, CMP_EQ);

    if (status == NOTOK)
    {
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (!isValidRid(startRid))
    {
        db_err_code = RELNOEXIST;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* Close relation if open */
    int r = FindRelNum(relName);
    if (r != NOTOK)
    {
        CloseRel(r);
    }

    /* ---------- 1. Remove the heap file ---------- */
    if (remove(relName) != 0)
    {
        db_err_code = FILESYSTEM_ERROR;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* ---------- 2. Remove the freemap file ---------- */
    char freeMapName[RELNAME + 6];
    build_fmap_filename(relName, freeMapName, sizeof(freeMapName));

    if (remove(freeMapName) != 0 && errno != ENOENT)
    {
        db_err_code = FILESYSTEM_ERROR;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("Relation %s destroyed successfully.\n", relName);

    /* ---------- 3. Now update catalogs ---------- */

    /* Delete from RelCat */
    DeleteRec(RELCAT_CACHE, startRid);
    startRid = INVALID_RID;

    /* Delete all AttrCat entries for this relation */
    do
    {
        status = FindRec(ATTRCAT_CACHE, startRid, &startRid,
                         attrCatRecPtr, 's', RELNAME,
                         offsetof(AttrCatRec, relName),
                         relName, CMP_EQ);

        if (status == NOTOK)
        {
            db_err_code = UNKNOWN_ERROR;
            free(relCatRecPtr);
            free(attrCatRecPtr);
            return ErrorMsgs(db_err_code, print_flag);
        }

        if (isValidRid(startRid))
        {
            DeleteRec(ATTRCAT_CACHE, startRid);
        }
        else
        {
            break;
        }
    } while (1);

    free(relCatRecPtr);
    free(attrCatRecPtr);

    return OK;
}