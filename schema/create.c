/************************INCLUDES*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/insertrec.h"
#include "../include/findrec.h"
#include "../include/findrel.h"
#include "../include/freemap.h"   // <-- for build_fmap_filename

/*------------------------------------------------------------

FUNCTION Create (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command line arguments.
           Expected format:
               create relName attr1 fmt1 attr2 fmt2 ... attrN fmtN
    argv → array of strings:
            argv[0] = "create"
            argv[1] = relation name
            argv[2] = attribute1 name
            argv[3] = attribute1 format ("i", "f", or "sN")
               ...

FUNCTION DESCRIPTION:
    Creates a new relation by:
      - validating the provided schema
      - computing record length and recsPerPg
      - inserting a new RelCatRec into relcat
      - inserting one AttrCatRec for each attribute into attrcat
      - creating the corresponding empty relation file

ALGORITHM:
    1. Validate argument count.
    2. Ensure DB is open.
    3. Validate relation name length.
    4. Validate each attribute name length.
    5. Check for duplicate attribute names.
    6. Parse formats:
        i → integer (4 bytes)
        f → float   (4 bytes)
        sN → string of length N (1 ≤ N ≤ MAX_N)
    7. Compute total record length.
    8. Ensure recLength ≤ (PAGESIZE − HEADER_SIZE).
    9. Check that relation does not already exist (FindRel()).
    10. Create the empty relation file.
    11. Compute recsPerPg = min((PAGESIZE-HEADER)/recLength, SLOTMAP bits).
    12. Prepare RelCatRec and insert via InsertRec().
    13. For each attribute:
            - build AttrCatRec
            - insert into attrcat via InsertRec().
    14. Print success message.

ERRORS REPORTED:
    ARGC_INSUFFICIENT
    DBNOTOPEN
    REL_LENGTH_EXCEEDED
    ATTR_NAME_EXCEEDED
    DUP_ATTR
    INVALID_FORMAT
    STR_LEN_INVALID
    RELEXIST
    REC_TOO_LONG
    FILESYSTEM_ERROR

GLOBAL VARIABLES MODIFIED:
    db_err_code
    catcache (indirectly through InsertRec)
    underlying catalog relations relcat/attrcat

IMPLEMENTATION NOTES:
    • Attribute order in argv determines storage order.

------------------------------------------------------------*/

int Create(int argc, char *argv[])
{
    FILE *fp;

    if(argc < 4)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    char *relName = argv[1];
    int recLength = 0;
    int recsPerPg, numAttrs, numRecs, numPgs;
    
    if(strlen(relName) >= RELNAME)
    {
        db_err_code = REL_LENGTH_EXCEEDED;
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    for(int i=2; i<argc; i+=2)
    {
        char *attrName = argv[i];
        
        if(strlen(attrName) >= ATTRNAME)
        {
            db_err_code = ATTR_NAME_EXCEEDED;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }
    
    for(int i=2; i<argc; i+=2)
    {
        for(int j=i+2; j<argc; j+=2)
        {
            if(!strncmp(argv[i], argv[j], ATTRNAME))
            {
                db_err_code = DUP_ATTR;
                return ErrorMsgs(db_err_code, print_flag);
            }
        }
    }
    
    for(int j=3; j<argc; j+=2)
    {
        char *format = argv[j];

        if (strcmp(format, "i") == 0)
        {
            recLength += sizeof(int);
            continue;
        }
        else if (strcmp(format, "f") == 0)
        {
            recLength += sizeof(float);
            continue;
        }
        else if (format[0] == 's')
        {
            if(strlen(format) < 2)
            {
                db_err_code = INVALID_FORMAT;
                return ErrorMsgs(db_err_code, print_flag );
            }

            for(const char *p = format + 1; *p; ++p)
            {
                if (!isdigit((unsigned char)*p))
                {
                    db_err_code = INVALID_FORMAT;
                    return ErrorMsgs(db_err_code, print_flag);
                }
            }
            
            if(strlen(format) >= 4)
            {
                db_err_code = STR_LEN_INVALID;
                return ErrorMsgs(db_err_code, print_flag);
            }

            int N = atoi(format + 1);

            if (N <= 0 || N > MAX_N)
            {
                db_err_code = STR_LEN_INVALID;
                return ErrorMsgs(db_err_code, print_flag && flag);
            }
            else
            {
                recLength += N;
            }
        }
        else
        {
            db_err_code = INVALID_FORMAT;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    if(FindRel(relName))
    {
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(recLength > (PAGESIZE-HEADER_SIZE))
    {
        db_err_code = REC_TOO_LONG;
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* Create the heap file */
    if(!(fp = fopen(relName, "w")))
    {
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }
    fclose(fp);

    /* Create the freemap file using build_fmap_filename */
    char freeMapName[RELNAME + 6];
    build_fmap_filename(relName, freeMapName, sizeof(freeMapName));

    FILE *fmap = fopen(freeMapName, "w");
    if (!fmap)
    {
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }
    fclose(fmap);

    recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / recLength, (sizeof(unsigned long) << 3));
    numAttrs  = (argc - 2) >> 1;
    numRecs   = 0;
    numPgs    = 0;

    RelCatRec rc = {relName, recLength, recsPerPg, numAttrs, numRecs, numPgs};
    InsertRec(RELCAT_CACHE, &rc);

    int offset = 0;

    for(int j=3; j<argc; j+=2)
    {
        char *format = argv[j];
        AttrCatRec ac;

        if(strcmp(format, "i") == 0)
        {
            ac = (AttrCatRec){offset, sizeof(int), 'i', "attrName", "relName", false, 0, 0};
            offset += sizeof(int);
        }
        else if(strcmp(format, "f") == 0)
        {
            ac = (AttrCatRec){offset, sizeof(float), 'f', "attrName", "relName", false, 0, 0};
            offset += sizeof(float);
        }
        else if(format[0] == 's')
        {
            int N = atoi(format + 1);
            ac = (AttrCatRec){offset, N, 's', "attrName", "relName", false, 0, 0};
            offset += N;
        }

        strncpy(ac.relName, relName, RELNAME);
        strncpy(ac.attrName, argv[j-1], ATTRNAME);
        InsertRec(ATTRCAT_CACHE, &ac);
    }

    printf("Relation %s created successfully with %d attributes.\n", relName, numAttrs);

    return OK;
}