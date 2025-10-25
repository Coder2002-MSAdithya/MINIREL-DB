#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/closerel.h"


int Destroy(int argc, char *argv[])
{
    if (!db_open)
    {
        return ErrorMsgs(DBNOTOPEN, print_flag);
    }

    if (argc < 2)
    {
        return ErrorMsgs(ARGC_INSUFFICIENT, print_flag);
    }

    char *relName = argv[1];

    if (strcmp(relName, RELCAT) == 0 || strcmp(relName, ATTRCAT) == 0)
    {
        return ErrorMsgs(CAT_DELETE_ERROR, print_flag);
    }
    
    int i, res;
    for (i = 0; i < MAXOPEN; i++)
    {
        if (strcmp(catcache[i].relcat_rec.relName, relName) == 0)
        {
            res = CloseRel(i);
            if (res != OK)
            {
                return ErrorMsgs(REL_CLOSE_ERROR, print_flag);
            }
        }
        
    }

    // ---- Remove relation file ----
    char path[MAX_PATH_LENGTH];
    snprintf(path, sizeof(path) + RELNAME + 2, "%s/%s", DB_DIR, relName);

    if (unlink(path) < 0)
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }
    
    // ---- Remove entries from relcat ----
    FILE *relFp = fopen(RELCAT, "rb");
    FILE *tempRel = fopen("relcat.tmp", "wb");

    RelCatRec rec;
    while (fread(&rec, sizeof(RelCatRec), 1, relFp))
    {
        if (strcmp(rec.relName, relName) != 0)
        {
            fwrite(&rec, sizeof(RelCatRec), 1, tempRel);
        }
    }

    fclose(relFp);
    fclose(tempRel);
    rename("relcat.tmp", RELCAT);

    // ---- Remove entries from attrcat ----
    FILE *attrFp = fopen(ATTRCAT, "rb");
    FILE *tempAttr = fopen("attrcat.tmp", "wb");

    AttrCatRec attr;
    while (fread(&attr, sizeof(AttrCatRec), 1, attrFp))
    {
        if (strcmp(attr.relName, relName) != 0)
        {
            fwrite(&attr, sizeof(AttrCatRec), 1, tempAttr);
        }
    }

    fclose(attrFp);
    fclose(tempAttr);
    rename("attrcat.tmp", ATTRCAT);

    if (print_flag)
    {
        printf("[INFO] Destroyed relation '%s'. All catalog entries removed.\n", relName);
    }

    return OK;
}
