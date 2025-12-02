#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrel.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include "../include/getnextrec.h"
#include "../include/insertrec.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

void copy_attribute(void *srcRec, void *dstRec, AttrDesc **srcAttrDesc, AttrDesc **dstAttrDesc) 
{
    int dOffset = (*dstAttrDesc)->attr.offset;
    int sOffset = (*srcAttrDesc)->attr.offset;
    int sType = (*srcAttrDesc)->attr.type;
    int sSize = (*srcAttrDesc)->attr.length;
    
    writeAttrToRec((char *)dstRec, 
                   (char *)srcRec + sOffset, 
                   sType, 
                   sSize, 
                   dOffset);
    
    *dstAttrDesc = (*dstAttrDesc)->next;
    *srcAttrDesc = (*srcAttrDesc)->next;
}

int Join(int argc, char **argv)
{
    if (!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *dstRelName = argv[1];
    char *src1RelName = argv[2];
    char *attrName1 = argv[3];
    char *src2RelName = argv[4];
    char *attrName2 = argv[5];

    int s1 = OpenRel(src1RelName);
    int s2 = OpenRel(src2RelName);

    if (s1 == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", src1RelName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), src1RelName);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (s2 == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", src2RelName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), src2RelName);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int d = FindRel(dstRelName);
    if (d)
    {
        printf("Relation '%s' already exists in the DB.\n", dstRelName);
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *ad1 = FindRelAttr(s1, attrName1);
    AttrDesc *ad2 = FindRelAttr(s2, attrName2);

    if (!ad1)
    {
        printf("Attribute '%s' NOT present in relation '%s' of the DB.\n", src1RelName, attrName1);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName1);
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (!ad2)
    {
        printf("Attribute '%s' NOT present in relation '%s' of the DB.\n", src2RelName, attrName2);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName2);
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char t1 = (ad1->attr).type;
    char t2 = (ad2->attr).type;

    if (t1 != t2)
    {
        printf("Type '%s' of '%s.%s' incompatible with '%s' of '%s.%s'.\n", t1, src1RelName, attrName1, t2, src2RelName, attrName2);
        db_err_code = INCOMPATIBLE_TYPES;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *head1 = catcache[s1].attrList;
    AttrDesc *head2 = catcache[s2].attrList;

    AttrDesc *resHead = NULL, *resTail = NULL;

    // --- Step 1: Copy all attributes from first relation (including attrName1) ---
    for (AttrDesc *p = head1; p; p = p->next)
    {
        AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));
        if (!newNode)
        {
            db_err_code = MEM_ALLOC_ERROR;
            return ErrorMsgs(db_err_code, print_flag);
        }

        newNode->attr = p->attr; // copy metadata
        newNode->next = NULL;

        if (resHead == NULL)
            resHead = resTail = newNode;
        else
        {
            resTail->next = newNode;
            resTail = newNode;
        }
    }

    // --- Step 2: Copy attributes from second relation, excluding join attribute ---
    for (AttrDesc *p = head2; p; p = p->next)
    {
        if (strncmp(p->attr.attrName, attrName2, ATTRNAME) == 0)
            continue; // skip join attr from second relation

        AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));
        if (!newNode)
        {
            db_err_code = MEM_ALLOC_ERROR;
            return ErrorMsgs(db_err_code, print_flag);
        }

        newNode->attr = p->attr;
        newNode->next = NULL;

        // --- Step 3: Handle name conflicts safely (≤20 chars including '\0') ---
        bool duplicate = false;
        for (AttrDesc *q = resHead; q; q = q->next)
        {
            if (strncmp(q->attr.attrName, p->attr.attrName, ATTRNAME) == 0)
            {
                duplicate = true;
                break;
            }
        }

        if (duplicate)
        {
            char newName[ATTRNAME];
            snprintf(newName, ATTRNAME, "%s_%s", p->attr.attrName, src2RelName);

            // Ensure total length ≤ 19 (reserve 1 byte for '\0')
            newName[ATTRNAME - 1] = '\0';

            // Truncate safely if it exceeds max length
            if (strlen(newName) >= ATTRNAME)
                newName[ATTRNAME - 1] = '\0';

            strncpy(newNode->attr.attrName, newName, ATTRNAME - 1);
            newNode->attr.attrName[ATTRNAME - 1] = '\0';
        }

        // Append to result list
        if (resHead == NULL)
            resHead = resTail = newNode;
        else
        {
            resTail->next = newNode;
            resTail = newNode;
        }
    }

    /* --- Step 4: Create the new joined relation schema --- */
    int status = CreateFromAttrList(dstRelName, resHead);

    if(status != OK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* --- Step 5: Cleanup memory for the result schema list --- */
    AttrDesc *tmp;
    while (resHead)
    {
        tmp = resHead;
        resHead = resHead->next;
        free(tmp);
    }

    if(status != OK)
    {
        return ErrorMsgs(db_err_code, print_flag);
        return NOTOK;
    }

    d = OpenRel(dstRelName);
    int dstRecSize = catcache[d].relcat_rec.recLength;

    void *recPtr1, *recPtr2;
    recPtr1 = malloc(catcache[s1].relcat_rec.recLength);
    recPtr2 = malloc(catcache[s2].relcat_rec.recLength);

    if(!recPtr1 || !recPtr2)
    {
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    Rid rid1, rid2;

    char t = t1;
    
    int o1 = ad1->attr.offset;
    int o2 = ad2->attr.offset;

    int l1 = ad1->attr.length;
    int l2 = ad2->attr.length;

    int cmpSize = MIN(ad1->attr.length, ad2->attr.length);
    int dNumAttrs = catcache[d].relcat_rec.numAttrs;
    int s1NumAttrs = catcache[s1].relcat_rec.numAttrs;
    int s2NumAttrs = catcache[s2].relcat_rec.numAttrs;

    rid1 = INVALID_RID;
    do
    {
        rid2 = INVALID_RID;
        if(GetNextRec(s1, rid1, &rid1, recPtr1) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(!isValidRid(rid1))
        {
            break;
        }

        do
        {
            if(GetNextRec(s2, rid2, &rid2, recPtr2) == NOTOK)
            {
                return ErrorMsgs(db_err_code, print_flag);
            }

            if(!isValidRid(rid2))
            {
                break;
            }

            void *valPtr1 = (char *)recPtr1 + o1;
            void *valPtr2 = (char *)recPtr2 + o2;

            if(compareVals(valPtr1, valPtr2, t, cmpSize, CMP_EQ))
            {
                void *dstRecPtr = malloc(dstRecSize);
                AttrDesc *dstAttrDescPtr = catcache[d].attrList;
                AttrDesc *src1AttrDescPtr = catcache[s1].attrList;
                AttrDesc *src2AttrDescPtr = catcache[s2].attrList;
                int ctr = 0;

                for(;ctr<dNumAttrs;ctr++)
                {
                    int dOffset = dstAttrDescPtr->attr.offset;

                    if(ctr < s1NumAttrs)
                    {
                        copy_attribute(recPtr1, dstRecPtr, &src1AttrDescPtr, &dstAttrDescPtr);
                    }
                    else
                    {
                        if(src2AttrDescPtr == ad2) 
                        {
                            src2AttrDescPtr = src2AttrDescPtr->next;
                            ctr--;
                        }
                        else
                        {
                            copy_attribute(recPtr2, dstRecPtr, &src2AttrDescPtr, &dstAttrDescPtr);
                        }
                    }
                }

                if(InsertRec(d, dstRecPtr) == NOTOK)
                {
                    return ErrorMsgs(db_err_code, print_flag);
                }
                
                free(dstRecPtr);
            }
        }
        while(1);

    } 
    while(1);

    free(recPtr1);
    free(recPtr2);

    printf("Join of relations %s and %s into %s successfully performed.\n",
    src1RelName, src2RelName, dstRelName);
}