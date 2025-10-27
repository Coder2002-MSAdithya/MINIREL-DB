#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/getnextrec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int Print(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc < 2)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc > 2)
    {
        db_err_code = TOO_MANY_ARGS;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];

    int r = OpenRel(relName);
    if(r == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("OK, printing relation %s\n\n", relName);

    /* --------- Print header line with attribute names --------- */
    AttrDesc *aptr = catcache[r].attrList;
    int attrCount = 0;
    for(AttrDesc *p = aptr; p; p = p->next)
    {
        printf("%-15s", (p->attr).attrName);
        attrCount++;
    }
    printf("\n");

    /* Print separator line */
    for(int i = 0; i < attrCount; i++)
        printf("--------------- ");
    printf("\n");

    /* --------- Now iterate through all records --------- */
    Rid startRid = (Rid){-1, -1};
    int recSize = catcache[r].relcat_rec.recLength;
    void *recPtr = malloc(recSize);
    if(!recPtr)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        return NOTOK;
    }

    while(true)
    {
        int status = GetNextRec(r, startRid, &startRid, recPtr);

        if(status == NOTOK || !isValidRid(startRid))
            break;

        /* Traverse each attribute and print its value */
        for(AttrDesc *p = aptr; p; p = p->next)
        {
            AttrCatRec *ac = &(p->attr);
            char *attrPtr = (char *)recPtr + ac->offset;

            switch(ac->type)
            {
                case 'i': {
                    int ival;
                    memcpy(&ival, attrPtr, sizeof(int));
                    printf("%-15d", ival);
                    break;
                }

                case 'f': {
                    float fval;
                    memcpy(&fval, attrPtr, sizeof(float));
                    printf("%-15.2f", fval);
                    break;
                }

                case 's': {
                    // Copy string with null termination for safety
                    int len = ac->length;
                    char *buf = (char *)malloc(len + 1);
                    memcpy(buf, attrPtr, len);
                    buf[len] = '\0';

                    // Trim trailing nulls/spaces for variable-length strings
                    for(int j = len - 1; j >= 0 && (buf[j] == '\0' || buf[j] == ' '); j--)
                        buf[j] = '\0';

                    printf("%-15s", buf);
                    free(buf);
                    break;
                }

                default:
                    printf("%-15s", "(unknown)");
                    break;
            }
        }

        printf("\n");
    }

    free(recPtr);
    return OK;
}
