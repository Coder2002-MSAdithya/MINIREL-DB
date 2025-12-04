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
    argc : Number of command-line arguments.
    argv : Argument vector where:

SPECIFICATIONS:
    argv[0] = "create"
    argv[1] = relation name
    argv[2] = attribute name 1
    argv[3] = attribute format 1   ("i", "f", or "sN")
    argv[4] = attribute name 2
    argv[5] = attribute format 2
    ...
    argv[argc-2] = attribute name K
    argv[argc-1] = attribute format K
    argv[argc]   = NIL

FUNCTION DESCRIPTION:
    The CREATE command defines a new base relation and writes corresponding entries into the relation catalog (relcat) and attribute catalog (attrcat). 
    It:
        - Validates that the database is open.
        - Ensures the relation name is legal and does not already exist.
        - Validates attribute names and checks for duplicates.
        - Validates attribute type formats:
            • "i" → integer (4 bytes)
            • "f" → float   (4 bytes)
            • "sN" → string of length N (N+1 bytes stored)
        - Computes the total record length to ensure the record fits within a MINIREL data page.
        - Creates an empty file for the relation.
        - Creates a corresponding freemap file "<relName>.fmap".
        - Inserts a new tuple in relcat describing the relation.
        - Inserts a tuple into attrcat for each attribute.
    Upon success, an empty file exists with a complete schema recorded in the catalogs.

ALGORITHM:
    1) Check db_open; if false, report DBNOTOPEN.
    2) Validate relation name length < RELNAME.
    3) Validate all attribute names:
        a) Name length < ATTRNAME.
        b) No duplicates across attributes.
    4) For each attribute type:
        a) Validate allowed formats ("i", "f", "sN").
        b) For string formats, validate numeric N and ensure N <= MAX_N.
        c) Accumulate total record length.
    5) Ensure record length ≤ (PAGESIZE–HEADER_SIZE).
    6) Check relation does not already exist via FindRel().
    7) Create an empty file.
    8) Create a freemap file "<relName>.fmap".
    9) Compute recsPerPg = min( (#slots fitting in page), number of bits in slotmap ).
    10) Insert a RelCatRec into relcat using InsertRec().
    11) For each attribute:
        a) Construct an AttrCatRec with correct offset, type, length.
        b) Insert into attrcat via InsertRec().
    12) Print success message if invoked by the user.

BUGS:
    None known.

ERRORS REPORTED:
    DBNOTOPEN          – No database currently open.
    REL_LENGTH_EXCEEDED– Relation name exceeds maximum length.
    ATTR_NAME_EXCEEDED – Attribute name too long.
    DUP_ATTR           – Duplicate attribute names in schema.
    INVALID_FORMAT     – Illegal attribute type format.
    STR_LEN_INVALID    – String length invalid or exceeds limits.
    RELEXIST           – Relation already exists in catalogs.
    REC_TOO_LONG       – Record does not fit a single page.
    FILESYSTEM_ERROR   – Unable to create file.
    MEM_ALLOC_ERROR    – From InsertRec() or catalog operations.

GLOBAL VARIABLES MODIFIED:
    db_err_code
    Catalog contents:
        • relcat updated with a new relation entry.
        • attrcat updated with K attribute entries.

IMPLEMENTATION NOTES:
    - The freemap file is created empty; all bits = 0 until InsertRec() begins populating pages.
    - This function does not open the relation; it only creates metadata and empty files.
    - The caller must ensure the CREATE command syntax is correct.
    - Relation file is created empty; no pages exist until the first INSERT.

------------------------------------------------------------*/

int Create(int argc, char *argv[])
{
    bool flag = (strcmp(argv[0], "create") == OK);
    FILE *fp;
    
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }
    
    char *relName = argv[1];
    int recLength = 0;
    int recsPerPg, numAttrs, numRecs, numPgs;
    
    if(strlen(relName) >= RELNAME)
    {
        db_err_code = REL_LENGTH_EXCEEDED;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }
    
    for(int i=2; i<argc; i+=2)
    {
        char *attrName = argv[i];
        
        if(strlen(attrName) >= ATTRNAME)
        {
            printf("Attribute name '%s' is too long.\n" 
            "Please consider using a different name.\n", attrName);
            db_err_code = ATTR_NAME_EXCEEDED;
            return ErrorMsgs(db_err_code, print_flag && flag);
        }
    }
    
    for(int i=2; i<argc; i+=2)
    {
        for(int j=i+2; j<argc; j+=2)
        {
            if(!strncmp(argv[i], argv[j], ATTRNAME))
            {
                printf("Attribute name '%s' has been duplicated.\n", argv[i]);
                db_err_code = DUP_ATTR;
                return ErrorMsgs(db_err_code, print_flag && flag);
            }
        }
    }
    
    for(int j = 3; j < argc; j += 2)
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
                printf("Format '%s' for attribute '%s' is INVALID.\n", argv[j], argv[j-1]);
                db_err_code = INVALID_FORMAT;
                return ErrorMsgs(db_err_code, print_flag && flag);
            }

            for(const char *p = format + 1; *p; ++p)
            {
                if (!isdigit((unsigned char)*p))
                {
                    printf("Format '%s' for attribute '%s' is INVALID.\n", argv[j], argv[j-1]);
                    db_err_code = INVALID_FORMAT;
                    return ErrorMsgs(db_err_code, print_flag && flag);
                }
            }
            
            if(strlen(format) >= 4)
            {
                db_err_code = STR_LEN_INVALID;
                return ErrorMsgs(db_err_code, print_flag && flag);
            }

            int N = atoi(format + 1);

            if (N <= 0 || N > MAX_N)
            {
                printf("String length for attribute '%s' is INVALID.\n", argv[j-1]);
                db_err_code = STR_LEN_INVALID;
                return ErrorMsgs(db_err_code, print_flag && flag);
            }
            else
            {
                recLength += (N + 1);
            }
        }
        else
        {
            printf("Format '%s' for attribute '%s' is INVALID.\n", argv[j], argv[j-1]);
            db_err_code = INVALID_FORMAT;
            return ErrorMsgs(db_err_code, print_flag && flag);
        }
    }

    if(FindRel(relName))
    {
        printf("Relation '%s' already exists in the DB.\n", relName);
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    if(recLength > (PAGESIZE - HEADER_SIZE))
    {
        db_err_code = REC_TOO_LONG;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    /* Create the heap file */
    if(!(fp = fopen(relName, "w")))
    {
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag && flag);
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

    RelCatRec rc = {"relName", recLength, recsPerPg, numAttrs, numRecs, numPgs};
    strncpy(rc.relName, relName, RELNAME);

    if(InsertRec(RELCAT_CACHE, &rc) == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    int offset = 0;

    for(int j = 3; j < argc; j += 2)
    {
        char *format = argv[j];
        AttrCatRec ac;

        if(strcmp(format, "i") == 0)
        {
            ac = (AttrCatRec){offset, sizeof(int), "i", "attrName", "relName", false, 0, 0};
            offset += sizeof(int);
        }
        else if(strcmp(format, "f") == 0)
        {
            ac = (AttrCatRec){offset, sizeof(float), "f", "attrName", "relName", false, 0, 0};
            offset += sizeof(float);
        }
        else if(format[0] == 's')
        {
            int N = atoi(format + 1);
            ac = (AttrCatRec){offset, (N + 1), "s", "attrName", "relName", false, 0, 0};
            offset += (N + 1);
        }

        strncpy(ac.relName, relName, RELNAME);
        strncpy(ac.attrName, argv[j-1], ATTRNAME);

        if(InsertRec(ATTRCAT_CACHE, &ac) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    if(flag)
        printf("Relation %s created successfully with %d attributes.\n", relName, numAttrs);

    return OK;
}