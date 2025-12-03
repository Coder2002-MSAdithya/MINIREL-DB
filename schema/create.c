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