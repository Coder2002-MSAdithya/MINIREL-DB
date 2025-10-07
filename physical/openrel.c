#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


int OpenRel(const char *relName)
{
    // Step 1: Check if already open
    int relNum = FindRelNum(relName);
    if (relNum != NOTOK)
    {
        return relNum;  // Already open
    }

    // Step 2: Find free cache slot
    int freeSlot = -1;
    for (int i = 0; i < MAXOPEN; i++)
    {
        if (catcache[i].relcat_rec.relName[0] == '\0')  // Empty slot
        {
            freeSlot = i;
            break;
        }
    }
    if (freeSlot == -1)
        return ErrorMsgs(BUFFER_FULL, print_flag);

    // Step 3: Read relcat to find the relation’s metadata
    FILE *fp = fopen(RELCAT, "rb");
    if (!fp)
    {
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    RelCatRec rcrec;
    int found = 0;
    while (fread(&rcrec, sizeof(RelCatRec), 1, fp) == 1)
    {
        if (strcmp(rcrec.relName, relName) == 0)
        {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found)
    {
        return ErrorMsgs(RELNOEXIST, print_flag);
    }

    // Step 4: Fill cache entry
    catcache[freeSlot].relcat_rec = rcrec;
    catcache[freeSlot].dirty = 0;

    // Step 5: Open relation file
    catcache[freeSlot].relFile = open(relName, O_RDWR);
    if (catcache[freeSlot].relFile < 0)
    {
        return ErrorMsgs(REL_OPEN_ERROR, print_flag);
    }

    // Step 6: Initialize its buffer
    buffer[freeSlot].dirty = 0;
    buffer[freeSlot].pid = -1;
    buffer[freeSlot].relFile = catcache[freeSlot].relFile;
    memset(buffer[freeSlot].page, 0, sizeof(buffer[freeSlot].page));

    return freeSlot;
}