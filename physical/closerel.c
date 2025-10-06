#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


int CloseRel(int relNum)
{
    if (relNum < 0 || relNum >= MAXOPEN)
        return NOTOK;

    CacheEntry *entry = &catcache[relNum];

    if (entry->relcat_rec.relName[0] == '\0')
        return NOTOK; // Not open

    // Step 1: Flush dirty page if any
    if (buffer[relNum].dirty)
    {
        lseek(entry->relFile, buffer[relNum].pid * PAGESIZE, SEEK_SET);
        write(entry->relFile, buffer[relNum].page, PAGESIZE);
        buffer[relNum].dirty = 0;
    }

    // Step 2: If catalog info dirty, update relcat on disk
    if (entry->dirty)
    {
        FILE *fp = fopen(RELCAT, "r+b");
        if (!fp)
            return ErrorMsgs(CAT_OPEN_ERROR, print_flag);

        // Find record by relName
        RelCatRec temp;
        while (fread(&temp, sizeof(RelCatRec), 1, fp) == 1)
        {
            if (strcmp(temp.relName, entry->relcat_rec.relName) == 0)
            {
                fseek(fp, -sizeof(RelCatRec), SEEK_CUR);
                fwrite(&entry->relcat_rec, sizeof(RelCatRec), 1, fp);
                break;
            }
        }
        fclose(fp);
        entry->dirty = 0;
    }

    // Step 3: Close file
    close(entry->relFile);

    // Step 4: Clear cache + buffer
    memset(entry, 0, sizeof(CacheEntry));
    memset(&buffer[relNum], 0, sizeof(Buffer));

    return OK;
}