#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrel.h"
#include "../include/getnextrec.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

AttrDesc *getAttrDesc(int relNum, const char *attrName)
{
    AttrDesc *ptr = catcache[relNum].attrList;
    for(;ptr;ptr=ptr->next)
    {
        if(strncmp((ptr->attr).attrName, attrName, ATTRNAME) == OK)
        {
            return ptr;
        }
    }
    return NULL;
}

int InsertProjectedRecFromLL(char *dstRelName, AttrDesc *head, void *recPtr)
{
    if (!dstRelName || !head || !recPtr)
        return NOTOK;

    /* Step 1: Count attributes */
    int attrCount = 0;
    AttrDesc *ptr = head;
    for (; ptr; ptr = ptr->next)
        attrCount++;

    /* Step 2: Allocate argv array */
    int insertArgc = 2 + 2 * attrCount;  // excluding NULL terminator
    char **insertArgv = (char **)malloc((insertArgc) * sizeof(char *));
    if (!insertArgv)
        return NOTOK;

    insertArgv[0] = strdup("_insert");
    insertArgv[1] = strdup(dstRelName);

    /* Step 3: Fill in attribute name-value pairs */
    ptr = head;
    int argIndex = 2;
    for (; ptr; ptr = ptr->next)
    {
        insertArgv[argIndex++] = strdup((ptr->attr).attrName);

        char type = (ptr->attr).type;
        int length = (ptr->attr).length;
        int offset = (ptr->attr).offset;
        char *valStr = NULL;

        if (type == 'i')
        {
            int val = *(int *)((char *)recPtr + offset);
            valStr = (char *)malloc(32);
            if (valStr)
                sprintf(valStr, "%d", val);
        }
        else if (type == 'f')
        {
            float val = *(float *)((char *)recPtr + offset);
            valStr = (char *)malloc(64);
            if (valStr)
                sprintf(valStr, "%.2f", val);
        }
        else if (type == 's')
        {
            char *src = (char *)recPtr + offset;
            valStr = (char *)malloc(length);
            if (valStr)
            {
                strncpy(valStr, src, length);
                valStr[length-1] = '\0';
            }
        }
        else
        {
            valStr = strdup("");
        }

        if (!valStr)
        {
            /* free everything allocated so far to avoid leaks */
            db_err_code = MEM_ALLOC_ERROR;
            for (int j = 0; j < argIndex; j++)
                free(insertArgv[j]);
            free(insertArgv);
            return NOTOK;
        }

        insertArgv[argIndex++] = valStr;
    }

    /* Step 5: Call Insert() */
    int ret = Insert(insertArgc, insertArgv);

    /* Step 6: Free all memory */
    for (int i = 0; i < argIndex; i++)
        free(insertArgv[i]);
    free(insertArgv);

    return ret;
}

int Project(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *dstRelName = argv[1];
    char *srcRelName = argv[2];

    int r = OpenRel(srcRelName);

    if(r == NOTOK)
    {
        if(db_err_code == RELNOEXIST)
        {
            printf("Relation '%s' does NOT exist in the DB.\n", srcRelName);
            printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), srcRelName, NULL);
        }
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *head = NULL;
    AttrDesc *tail = NULL;
    int recLength = catcache[r].relcat_rec.recLength;

    for(int c = 3; c < argc; c++)
    {
        AttrDesc *ptr = getAttrDesc(r, argv[c]);

        if(!ptr)
        {
            printf("Attribute '%s' does NOT exist in relation '%s' of the DB.\n", argv[c], srcRelName);
            printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), argv[c], srcRelName);
            db_err_code = ATTRNOEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }
        else
        {
            /* Add a copy of the AttrDesc node pointed to by ptr 
            to the linked list headed by head of type (AttrDesc *) */

            /* Allocate a new AttrDesc node */
            AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));

            if (!newNode)
            {
                db_err_code = MEM_ALLOC_ERROR;
                return ErrorMsgs(db_err_code, print_flag);
            }

            /* Copy the attribute catalog record contents */
            newNode->attr = ptr->attr;

            /* Initialize next pointer */
            newNode->next = NULL;

            /* Insert at end of linked list */
            if (!head)
            {
                head = newNode;
                tail = newNode;
            }
            else
            {
                tail->next = newNode;
                tail = newNode;
            }
        }
    }

    int s = FindRel(dstRelName);

    if(s)
    {
        printf("Relation '%s' already exists in the DB.\n", dstRelName);
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    s = CreateFromAttrList(dstRelName, head);

    if(s != OK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    Rid recRid = INVALID_RID;
    void *recPtr = malloc(recLength);
    do
    {
        if(GetNextRec(r, recRid, &recRid, recPtr) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(!isValidRid(recRid))
        {
            break;
        }

        int sIns = InsertProjectedRecFromLL(dstRelName, head, recPtr);
        if(sIns != OK && sIns != DUP_ROWS)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }
    }
    while(true);

    printf("Projected relation %s into %s successfully.\n", srcRelName, dstRelName);

    return OK;
}