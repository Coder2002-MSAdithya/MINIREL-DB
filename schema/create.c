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


int Create(int argc, char *argv[])
{
    bool flag = (strcmp(argv[0], "create") == OK);
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
    
    for(int j = 3; j < argc; j += 2)
    {
        char *format = argv[j];

        // Case 1: integer type ("i")
        if (strcmp(format, "i") == 0)
        {
            recLength += sizeof(int);
            continue;
        }

        // Case 2: float type ("f")
        else if (strcmp(format, "f") == 0)
        {
            recLength += sizeof(float);
            continue;
        }

        // Case 3: string type ("sN")
        else if (format[0] == 's')
        {
            // Ensure there is at least one digit after 's'
            if(strlen(format) < 2)
            {
                db_err_code = INVALID_FORMAT;
                return ErrorMsgs(db_err_code, print_flag);
            }

            // Also verify that all chars after 's' are digits
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

            // Extract N (the string length)
            int N = atoi(format + 1);

            // Check validity of N
            if (N <= 0 || N > MAX_N)
            {
                db_err_code = STR_LEN_INVALID;  // e.g. define this in your error codes
                return ErrorMsgs(db_err_code, print_flag);
            }
            else
            {
                recLength += N;
            }

        }
        // Invalid format (none of "i", "f", or "sN")
        else
        {
            db_err_code = INVALID_FORMAT;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    Rid startRid = INVALID_RID;
    RelCatRec rcd;
    int status = FindRec(RELCAT_CACHE, startRid, &startRid, &rcd, 's', RELNAME, offsetof(RelCatRec, relName), relName, CMP_EQ);

    if(status == NOTOK)
    {
        db_err_code = UNKNOWN_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(isValidRid(startRid))
    {
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(recLength > (PAGESIZE - HEADER_SIZE))
    {
        db_err_code = REC_TOO_LONG;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(!(fp = fopen(relName, "w")))
    {
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    recLength = recLength;
    recsPerPg = (PAGESIZE - HEADER_SIZE) / recLength;
    numAttrs = (argc - 2) >> 1;
    numRecs = 0;
    numPgs = 0;

    RelCatRec rc = {"relName", recLength, recsPerPg, numAttrs, numRecs, numPgs};
    strncpy(rc.relName, relName, RELNAME);
    InsertRec(RELCAT_CACHE, &rc);

    int offset = 0;

    for(int j = 3; j < argc; j += 2)
    {
        char *format = argv[j];
        AttrCatRec ac;

        if(strcmp(format, "i") == 0)
        {
            ac = (AttrCatRec){offset, sizeof(int), 'i', "attrName", "relName"};
            offset += sizeof(int);
        }
        else if(strcmp(format, "f") == 0)
        {
            ac = (AttrCatRec){offset, sizeof(float), 'f', "attrName", "relName"};
            offset += sizeof(float);
        }
        else if(format[0] == 's')
        {
            int N = atoi(format + 1);
            ac = (AttrCatRec){offset, N, 's', "attrName", "relName"};
            offset += N;
        }

        strncpy(ac.relName, relName, RELNAME);
        strncpy(ac.attrName, argv[j-1], ATTRNAME);
        InsertRec(ATTRCAT_CACHE, &ac);
    }

    if(flag)
    printf("Relation %s created successfully with %d attributes.\n", relName, numAttrs);

    return OK;
}