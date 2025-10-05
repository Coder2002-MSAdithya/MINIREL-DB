#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

/* helper to read all records of a catalog file into memory */
int readCatalogFile(const char *filename, int recordSize, void *recordArray, int maxRecords) 
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) 
    {
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    int readCount = 0;
    char page[PAGESIZE];

    while (fread(page, 1, PAGESIZE, fp) == PAGESIZE) 
    {
        // --- check magic ---
        if (strncmp(page+1, GEN_MAGIC, strlen(GEN_MAGIC)) != 0) 
        {
            fclose(fp);
            return ErrorMsgs(UNKNOWN_ERROR, print_flag);
        }

        // slotmap right after magic
        unsigned long *slotmapPtr = (unsigned long *)(page + MAGIC_SIZE);
        char *dataStart = page + HEADER_SIZE;

        // scan slots
        for (int slot = 0; slot < (PAGESIZE - HEADER_SIZE) / recordSize; slot++) 
        {
            if ((*slotmapPtr >> slot) & 1UL) 
            {
                if (readCount >= maxRecords) 
                {
                    fclose(fp);
                    return ErrorMsgs(UNKNOWN_ERROR, print_flag);
                }
                memcpy((char *)recordArray + readCount * recordSize, dataStart + slot * recordSize, recordSize);
                readCount++;
            }
        }
    }

    fclose(fp);
    return readCount; // number of records read
}

int OpenCats() 
{
    // Read relcat
    RelCatRec relcatRecords[NUM_CATS];
    int relCount = readCatalogFile(RELCAT, sizeof(RelCatRec), relcatRecords, NUM_CATS);
    if (relCount <= 0) return NOTOK;

    // Read attrcat
    AttrCatRec attrcatRecords[attrCat_numRecs];
    int attrCount = readCatalogFile(ATTRCAT, sizeof(AttrCatRec), attrcatRecords, attrCat_numRecs);
    if (attrCount <= 0) return NOTOK;

    // Populate cache
    for (int i = 0; i < relCount; i++) 
    {
        catcache[i].relcat_rec = relcatRecords[i];
        catcache[i].dirty = 0;
        catcache[i].relFile = -1;  // not opened yet
        catcache[i].attrList = NULL;

        // Link attribute list
        AttrDesc *prev = NULL;
        for (int j = 0; j < attrCount; j++) 
        {
            if (strcmp(attrcatRecords[j].relName, relcatRecords[i].relName) == 0) 
            {
                AttrDesc *node = (AttrDesc *)malloc(sizeof(AttrDesc));
                node->attr = attrcatRecords[j];
                node->next = NULL;
                if (prev == NULL) 
                {
                    catcache[i].attrList = node;
                } 
                else 
                {
                    prev->next = node;
                }
                prev = node;
            }
        }
    }

    db_open = true;
    return OK;
}
