/************************INCLUDES*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

const int s_numRecs = sizeof(students) / sizeof(students[0]);
const int s_recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / sizeof(Student), SLOTMAP<<3);
const int s_numPgs = (s_numRecs + s_recsPerPg - 1) / s_recsPerPg;
const int p_numRecs = sizeof(professors) / sizeof(professors[0]);
const int p_recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / sizeof(Professor), SLOTMAP<<3);
const int p_numPgs = (p_numRecs + p_recsPerPg - 1) / p_recsPerPg;

RelCatRec stud_rc = {
    .relName = "Students",
    .recLength = sizeof(Student),
    .recsPerPg = s_recsPerPg,
    .numAttrs = 3,
    .numRecs = s_numRecs,
    .numPgs = s_numPgs
};

RelCatRec prof_rc = {
    .relName = "Profs",
    .recLength = sizeof(Professor),
    .recsPerPg = p_recsPerPg,
    .numAttrs = 4,
    .numRecs = p_numRecs,
    .numPgs = p_numPgs
};

//attrcat records for columns of Profs
AttrCatRec profs_name = {offsetof(Professor, name), 30, 's', "name", "Profs", false, 0, 0};
AttrCatRec profs_id = {offsetof(Professor, id), sizeof(int), 'i', "id", "Profs", false, 0, 0};
AttrCatRec profs_sal = {offsetof(Professor, salary), sizeof(float), 'f', "salary", "Profs", false, 0, 0};
AttrCatRec profs_des = {offsetof(Professor, designation), 30, 's', "designation", "Profs", false, 0, 0};

//attrcat records for columns of Students
AttrCatRec stud_name = {offsetof(Student, name), 40, 's', "name", "Students", false, 0, 0};
AttrCatRec stud_id = {offsetof(Student, id), sizeof(int), 'i', "id", "Students", false, 0, 0};
AttrCatRec stud_stp = {offsetof(Student, stipend), sizeof(float), 'f', "stipend", "Students", false, 0, 0};



/*------------------------------------------------------------

FUNCTION CreateRelCat (void)

PARAMETER DESCRIPTION:
    None.

FUNCTION DESCRIPTION:
    Creates the system relation catalog RELCAT. 
    It contains one RelCatRec entry for each system-defined relation:
        - relcat
        - attrcat
    These catalog records describe the schema of the system catalogs.

ALGORITHM:
    1. Build an array relcat_recs[] containing:
            Relcat_rc, Relcat_ac
    2. Pass this array to writeRecsToFile(), which:
            - formats them into pages
            - writes appropriate magic header
            - stores into physical file RELCAT
    3. Return OK or NOTOK based on write status.

ERRORS REPORTED:
    CAT_CREATE_ERROR (forwarded from writeRecsToFile)

GLOBAL VARIABLES MODIFIED:
    None directly (but overwrites RELCAT file on disk).

IMPLEMENTATION NOTES:
    - RELCAT must always be written before ATTRCAT.
    - Order of records must be consistent.

------------------------------------------------------------*/

int CreateRelCat()
{
    // Create array of RelCatRecs to insert them one by one into page
    RelCatRec relcat_recs[] = {Relcat_rc, Relcat_ac, stud_rc, prof_rc};

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(RELCAT, relcat_recs, NUM_CATS + NT_RELCAT, sizeof(RelCatRec), '$');
}


/*------------------------------------------------------------

FUNCTION CreateAttrCat (void)

PARAMETER DESCRIPTION:
    None.

FUNCTION DESCRIPTION:
    Creates the attribute catalog ATTRCAT. It contains AttrCatRec entries describing the attributes of:
        - relcat
        - attrcat
    Each attribute record includes its:
        - offset
        - length
        - type
        - attribute name
        - parent relation name

ALGORITHM:
    1. Build an array attrcat_recs[] containing all attribute descriptions for system catalogs.
    2. Call writeRecsToFile() to physically write pages to ATTRCAT.
    3. Return OK or NOTOK based on write status.

ERRORS REPORTED:
    CAT_CREATE_ERROR

GLOBAL VARIABLES MODIFIED:
    ATTRCAT file on disk.

------------------------------------------------------------*/

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
        profs_id,
        profs_name,
        profs_des,
        profs_sal,
        stud_id,
        stud_name,
        stud_stp
    };

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(ATTRCAT, attrcat_recs, attrCat_numRecs, sizeof(AttrCatRec), '!');
}


// Just for testing
int CreateDummy()
{
    if(writeRecsToFile(stud_rc.relName, students, stud_rc.numRecs, sizeof(Student), '_') != OK)
    {
        return NOTOK;
    }

    if(writeRecsToFile(prof_rc.relName, professors, prof_rc.numRecs, sizeof(Professor), '_') != OK)
    {
        return NOTOK;
    }
    
    return OK;
}


/*------------------------------------------------------------

FUNCTION CreateCats (void)

PARAMETER DESCRIPTION:
    None.

FUNCTION DESCRIPTION:
    This routine initializes the MiniRel system catalogs. 
    It creates:
        - RELCAT    (relation catalog)
        - ATTRCAT   (attribute catalog)
    This routine is called during CreateDB(), and must succeed before the database can be opened or relations can be created.

ALGORITHM:
    1. Call CreateRelCat().
    2. Call CreateAttrCat().
    3. If both succeed, return OK.
    4. On failure:
        - set db_err_code = CAT_CREATE_ERROR
        - return NOTOK.

ERRORS REPORTED:
    CAT_CREATE_ERROR

GLOBAL VARIABLES MODIFIED:
    db_err_code

------------------------------------------------------------*/

int CreateCats()
{
    if(CreateRelCat() == OK && CreateAttrCat() == OK && CreateDummy() == OK)
    {
        return OK;
    }

    db_err_code = CAT_CREATE_ERROR;
    return NOTOK;
}