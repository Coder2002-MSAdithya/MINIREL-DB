#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

int CreateRelCat()
{
    // Create array of RelCatRecs to insert them one by one into page
    RelCatRec relcat_recs[] = {Relcat_rc, Relcat_ac};

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(RELCAT, relcat_recs, NUM_CATS, sizeof(RelCatRec), '$');
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
        AttrCat_arelName,
        AttrCat_hasIndex,
        AttrCat_nPages,
        AttrCat_nKeys,
    };

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(ATTRCAT, attrcat_recs, attrCat_numRecs, sizeof(AttrCatRec), '!');
}

int CreateCats()
{
    if(CreateRelCat() == OK && CreateAttrCat() == OK)
    {
        return OK;
    }

    db_err_code = CAT_CREATE_ERROR;
    return NOTOK;
}