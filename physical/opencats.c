#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

int OpenCats()
{
    // Open catalog files
    int rel_fd = open(RELCAT, O_RDWR);
    int attr_fd = open(ATTRCAT, O_RDWR);

    if (rel_fd < 0 || attr_fd < 0)
    {
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    // Load relcat entry into cache[0]
    catcache[0].relcat_rec = Relcat_rc;
    catcache[0].relFile = rel_fd;
    catcache[0].dirty = 0;
    catcache[0].valid = 1;

    // Build attr list for relcat
    AttrDesc *rhead = NULL, *rtail = NULL;
    AttrCatRec *rel_attrs[] = {
        &Attrcat_rrelName,
        &Attrcat_recLength,
        &Attrcat_recsPerPg,
        &AttrCat_numAttrs,
        &AttrCat_numRecs,
        &AttrCat_numPgs
    };

    for (int i = 0; i < RELCAT_NUMATTRS; i++) 
    {
        AttrDesc *node = malloc(sizeof(AttrDesc));
        node->attr = *rel_attrs[i];
        node->next = NULL;
        if (!rhead)
        {
            rhead = node;
        }
        else 
        {
            rtail->next = node;
            rtail = node;
        }
    }
    
    catcache[0].attrList = rhead;

    // Load attrcat entry into cache[1]
    catcache[1].relcat_rec = Relcat_ac;
    catcache[1].relFile = attr_fd;
    catcache[1].dirty = 0;
    catcache[1].valid = 1;

    // Build attr list for attrcat
    AttrDesc *ahead = NULL, *atail = NULL;
    AttrCatRec *attr_attrs[] = {
        &AttrCat_offset,
        &AttrCat_length,
        &AttrCat_type,
        &AttrCat_attrName,
        &AttrCat_arelName
    };

    for (int i = 0; i < ATTRCAT_NUMATTRS; i++) 
    {
        AttrDesc *node = malloc(sizeof(AttrDesc));
        node->attr = *attr_attrs[i];
        node->next = NULL;
        if (!ahead) 
        {
            ahead = node;
        }
        else 
        {
            atail->next = node;
            atail = node;
        }
    }
    
    catcache[1].attrList = ahead;

    // Initialize buffer pool
    for (int i = 0; i < MAXOPEN; i++) 
    {
        buffer[i].relNum = -1;
        buffer[i].dirty = 0;
        buffer[i].pid = -1;
        buffer[i].relFile = -1;
        memset(buffer[i].page, 0, sizeof(buffer[i].page));
    }

    // Mark DB as open
    db_open = true;

    return OK;
}
