#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/readpage.h"

AttrDesc* BuildAttrList(AttrCatRec **attrArray, int numAttrs)
{
    AttrDesc *head=NULL, *tail=NULL;
    for(int i=0;i<numAttrs;i++)
    {
        AttrDesc *node=malloc(sizeof(AttrDesc));
        if(!node)
        {
            db_err_code = MEM_ALLOC_ERROR;
            FreeLinkedList(&head, offsetof(AttrDesc, next));
            return NOTOK;
        }
        node->attr=*attrArray[i];
        node->next=NULL;

        if(!head)
        {
            head=node;
            tail=node;
        }
        else
        {
            tail->next=node;
            tail=node;
        }
    }
    return head;
}

int OpenCats()
{
    // Open catalog files
    int rel_fd = open(RELCAT, O_RDWR);
    int attr_fd = open(ATTRCAT, O_RDWR);

    if(rel_fd == NOTOK || attr_fd == NOTOK)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    // Page variable
    char page[PAGESIZE];

    // To place the current catalog values of relcat and attrcat
    RelCatRec Relcat_rc, Relcat_ac;

    // Read the first page of relcat into page buffer
    if(read(rel_fd, page, PAGESIZE) == NOTOK)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    // The first record of relcat relation stores relcat's info
    memcpy(&Relcat_rc, page+HEADER_SIZE, sizeof(RelCatRec));

    // The second record of the relcat relation holds attrcat's info
    memcpy(&Relcat_ac, page+HEADER_SIZE+sizeof(RelCatRec), sizeof(RelCatRec));

    if (rel_fd == NOTOK || attr_fd == NOTOK)
    {
        db_err_code = CAT_OPEN_ERROR;
        return NOTOK;
    }

    // Attributes of relcat relation
    AttrCatRec *rel_attrs[] = {
        &Attrcat_rrelName,
        &Attrcat_recLength,
        &Attrcat_recsPerPg,
        &AttrCat_numAttrs,
        &AttrCat_numRecs,
        &AttrCat_numPgs
    };

    // Attributes of attrcat relation
    AttrCatRec *attr_attrs[] = {
        &AttrCat_offset,
        &AttrCat_length,
        &AttrCat_type,
        &AttrCat_attrName,
        &AttrCat_arelName,
        &AttrCat_hasIndex,
        &AttrCat_nPages,
        &AttrCat_nKeys
    };

    // Load relcat entry into cache[0]
    catcache[0].relcat_rec = Relcat_rc;
    catcache[0].relFile = rel_fd;
    catcache[0].status = (PINNED_MASK | VALID_MASK); // last three bits are 1 (for metadata), 1 (for valid) and 0 (for clean)
    /* Record for attrcat is in slot 0 of the page 0 (O based indexing of pages and slots)*/
    catcache[0].relcatRid.pid = 0;
    catcache[0].relcatRid.slotnum = 0;
    catcache[0].attrList = BuildAttrList(rel_attrs, RELCAT_NUMATTRS);

    // Load attrcat entry into cache[1]
    catcache[1].relcat_rec = Relcat_ac;
    catcache[1].relFile = attr_fd;
    catcache[1].status = (PINNED_MASK | VALID_MASK); // last three bits are 1 (for metadata), 1 (for valid) and 0 (for clean)
    /* Record for attrcat is in slot 1 of the page 0 (O based indexing of pages and slots)*/
    catcache[1].relcatRid.pid = 0;
    catcache[1].relcatRid.slotnum = 1;
    catcache[1].attrList = BuildAttrList(attr_attrs, ATTRCAT_NUMATTRS);

    // Initialize buffer pool
    for (int i = 0; i < MAXOPEN; i++) 
    {
        buffer[i].dirty = 0;
        buffer[i].pid = -1;
        memset(buffer[i].page, 0, sizeof(buffer[i].page));
    }

    // Mark DB as open (OpenDB already does this)
    db_open = true;

    return OK;
}
