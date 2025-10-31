#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

// const int s_numRecs = sizeof(students) / sizeof(students[0]);
// const int s_recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / sizeof(Student), SLOTMAP<<3);
// const int s_numPgs = (s_numRecs + s_recsPerPg - 1) / s_recsPerPg;
// const int p_numRecs = sizeof(professors) / sizeof(professors[0]);
// const int p_recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / sizeof(Professor), SLOTMAP<<3);
// const int p_numPgs = (p_numRecs + p_recsPerPg - 1) / p_recsPerPg;

// RelCatRec stud_rc = {
//     .relName = "Students",
//     .recLength = sizeof(Student),
//     .recsPerPg = s_recsPerPg,
//     .numAttrs = 3,
//     .numRecs = s_numRecs,
//     .numPgs = s_numPgs
// };

// RelCatRec prof_rc = {
//     .relName = "Profs",
//     .recLength = sizeof(Professor),
//     .recsPerPg = p_recsPerPg,
//     .numAttrs = 4,
//     .numRecs = p_numRecs,
//     .numPgs = p_numPgs
// };

// //attrcat records for columns of Profs
// AttrCatRec profs_name = {offsetof(Professor, name), 30, 's', "name", "Profs"};
// AttrCatRec profs_id = {offsetof(Professor, id), sizeof(int), 'i', "id", "Profs"};
// AttrCatRec profs_sal = {offsetof(Professor, salary), sizeof(float), 'f', "salary", "Profs"};
// AttrCatRec profs_des = {offsetof(Professor, designation), 30, 's', "designation", "Profs"};

// //attrcat records for columns of Students
// AttrCatRec stud_name = {offsetof(Student, name), 40, 's', "name", "Students"};
// AttrCatRec stud_id = {offsetof(Student, id), sizeof(int), 'i', "id", "Students"};
// AttrCatRec stud_stp = {offsetof(Student, stipend), sizeof(float), 'f', "stipend", "Students"};

int CreateRelCat()
{
    // Create array of RelCatRecs to insert them one by one into page
    // RelCatRec relcat_recs[] = {Relcat_rc, Relcat_ac, stud_rc, prof_rc};
    RelCatRec relcat_recs[] = {RelCat_rc, RelCat_ac};

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(RELCAT, relcat_recs, relcat_numRecs, sizeof(RelCatRec), '$');
}

int CreateAttrCat()
{
    // Create array of AttrCatRecs to insert them one by one into page
    AttrCatRec attrcat_recs[] = {
        AttrCat_rrelName,
        AttrCat_recLength,
        AttrCat_recsPerPg,
        AttrCat_numAttrs,
        AttrCat_numRecs,
        AttrCat_numPgs,
        AttrCat_offset,
        AttrCat_length,
        AttrCat_type,
        AttrCat_attrName,
        AttrCat_arelName,
        // profs_id,
        // profs_name,
        // profs_des,
        // profs_sal,
        // stud_id,
        // stud_name,
        // stud_stp
    };

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(ATTRCAT, attrcat_recs, attrcat_numRecs, sizeof(AttrCatRec), '!');
}

// Just for testing
// int CreateDummy()
// {
//     if(writeRecsToFile(stud_rc.relName, students, stud_rc.numRecs, sizeof(Student), '_') != OK)
//     {
//         return NOTOK;
//     }

//     if(writeRecsToFile(prof_rc.relName, professors, prof_rc.numRecs, sizeof(Professor), '_') != OK)
//     {
//         return NOTOK;
//     }
    
//     return OK;
// }

int CreateCats()
{
    // if(CreateRelCat() == OK && CreateAttrCat() == OK && CreateDummy() == OK)
    if(CreateRelCat() == OK && CreateAttrCat() == OK)
    {
        return OK;
    }

    db_err_code = CAT_CREATE_ERROR;
    return NOTOK;
}