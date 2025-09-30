#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"

int writeCatRecsToFile(const char *filename, void *recs, int numRecs, int recordSize, char magicChar)
{
    FILE *fp = fopen(filename, "wb");

    if(!fp)
    {
        return ErrorMsgs(CAT_CREATE_ERROR, print_flag);
    }

    char page[PAGESIZE];
    int recsPerPg = (PAGESIZE-HEADER_SIZE)/recordSize;

    int written = 0;

    while(written < numRecs)
    {
        // Prepare the magic word to write
        char MAGIC_STR[MAGIC_SIZE+1] = {magicChar};
        strcpy(MAGIC_STR+1, GEN_MAGIC);

        //Reset page after it is over
        memset(page, 0, PAGESIZE);

        // Write Magic string
        strcpy(page, MAGIC_STR);

        //Pointer to slotmap
        unsigned long *slotmapPtr = (unsigned long *)(page + MAGIC_SIZE);
        *slotmapPtr = 0;

        //Page data start
        char *dataStart = page + HEADER_SIZE;

        //Fill records into this page
        int slot = 0;
        
        while(slot < recsPerPg && written < numRecs)
        {
            memcpy(dataStart+slot*recordSize, (char *)recs+written*recordSize, recordSize);

            //Mark slot map filled (LSB first)
            *slotmapPtr |= (1UL << slot);

            slot++;
            written++;
        }

        //Write the page
        fwrite(page, 1, PAGESIZE, fp);
    }

    fclose(fp);
    return OK;
}

int CreateRelCat()
{
    FILE *rfp;
    char page[PAGESIZE];
    unsigned long slotmap;
    RelCatRec relcat_rc;
    RelCatRec relcat_ac;

    const int relcat_recLength = (int)sizeof(RelCatRec);
    const int attrcat_recLength = (int)sizeof(AttrCatRec);

    const int relcat_recsPerPg = (PAGESIZE - HEADER_SIZE) / relcat_recLength;
    const int attrcat_recsPerPg = (PAGESIZE - HEADER_SIZE) / attrcat_recLength;

    const int relcat_numAttrs = 6;
    const int attrcat_numAttrs = 5;

    const int attrCat_numRecs = relcat_numAttrs + attrcat_numAttrs;

    //Initialized relcat record for relcat
    relcat_rc = (RelCatRec) {RELCAT, relcat_recLength, relcat_recsPerPg, relcat_numAttrs, 2, 1};

    //Initialized relcat record for attrcat
    relcat_ac = (RelCatRec) {ATTRCAT, attrcat_recLength, attrcat_recsPerPg, attrcat_numAttrs, attrCat_numRecs, 1};

    // Create array of RelCatRecs to insert them one by one into page
    RelCatRec relcat_recs[] = {relcat_rc, relcat_ac};

    //Slotmap has two records filled now
    slotmap = 0xC000000000000000;

    //Put the magic number at the start of each page
    strcpy(page, "$MINIREL");
    *((long *)(page + MAGIC_SIZE)) = slotmap;
    *((RelCatRec *)(page + HEADER_SIZE)) = relcat_rc;
    *((RelCatRec *)(page + HEADER_SIZE + sizeof(RelCatRec))) = relcat_ac;

    rfp = fopen(RELCAT, "wb");
    fwrite(page, 1, PAGESIZE, rfp);
    fclose(rfp);

    return OK;
}

int CreateAttrCat()
{
    printf("CreateAttrCat\n");
    return OK;
}


int CreateCats()
{
    if(CreateRelCat() == OK && CreateAttrCat() == OK)
    {
        printf("CreateCats OK\n");
        return OK;
    }

    printf("CreateCats NOTOK");
    return NOTOK;
}