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

    bool flag = strcmp(argv[0], "destroy") == OK;
    char *relName = argv[1];
    void *relCatRecPtr  = malloc(sizeof(RelCatRec));
    void *attrCatRecPtr = malloc(sizeof(AttrCatRec));
    Rid startRid = INVALID_RID;

    if (!relCatRecPtr || !attrCatRecPtr)
    {
        free(relCatRecPtr);
        free(attrCatRecPtr);
        db_err_code = MEM_ALLOC_ERROR;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    /* Protect system catalogs */
    if (strncmp(relName, RELCAT, RELNAME) == OK ||
        strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        printf("CANNOT destroy catalog relation %s.\n", relName);
        db_err_code = METADATA_SECURITY;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag && flag);
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
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    if (!isValidRid(startRid))
    {
        printf("Relation '%s' does NOT exist in the DB.\n", relName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), relName, NULL);
        db_err_code = RELNOEXIST;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag && flag);
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
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    /* ---------- 2. Remove the freemap file ---------- */
    char freeMapName[RELNAME + 6];
    build_fmap_filename(relName, freeMapName, sizeof(freeMapName));

    if (remove(freeMapName) != 0 && errno != ENOENT)
    {
        db_err_code = FILESYSTEM_ERROR;
        free(relCatRecPtr);
        free(attrCatRecPtr);
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    printf("Relation %s destroyed successfully.\n", relName);

    /* ---------- 3. Now update catalogs ---------- */

    /* Delete from RelCat */
    if(DeleteRec(RELCAT_CACHE, startRid) == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

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
            return ErrorMsgs(db_err_code, print_flag && flag);
        }

        if (isValidRid(startRid))
        {
            if(DeleteRec(ATTRCAT_CACHE, startRid) == NOTOK)
            {
                return ErrorMsgs(db_err_code, print_flag && flag);
            }
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