#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

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
    // Create array of RelCatRecs to insert them one by one into page
    RelCatRec relcat_recs[] = {Relcat_rc, Relcat_ac};

    // Write the records in an array to the catalog file in pages
    writeCatRecsToFile(RELCAT, relcat_recs, NUM_CATS, sizeof(RelCatRec), '$');

    return OK;
}

int CreateAttrCat()
{
    // Create array of AttrCatRecs to insert them one by one into page
    AttrCatRec attrcat_recs[] = {
        Attrcat_rrelName,
        Attrcat_recLength,
        Attrcat_recsPerPg,
        AttrCat_numAttrs,
        AttrCat_numRecs,
        AttrCat_numPgs,
        AttrCat_offset,
        AttrCat_length,
        AttrCat_type,
        AttrCat_attrName,
        AttrCat_arelName
    };

    // Write the records in an array to the catalog file in pages
    writeCatRecsToFile(ATTRCAT, attrcat_recs, attrCat_numRecs, sizeof(AttrCatRec), '$');

    return OK;
}


int CreateCats()
{
    if(CreateRelCat() == OK && CreateAttrCat() == OK)
    {
        return OK;
    }

    return NOTOK;
}