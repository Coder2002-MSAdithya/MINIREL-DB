/************************INCLUDES*******************************/
#include "../include/defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/readpage.h"


/*------------------------------------------------------------

FUNCTION BuildAttrList (attrArray, numAttrs)

PARAMETER DESCRIPTION:
    attrArray   - Array of pointers to AttrCatRec structures representing attribute catalog entries for a relation.
    numAttrs    - Number of attribute entries in the array.

FUNCTION DESCRIPTION:
    Builds a linked list of AttrDesc nodes from an array of attribute catalog records. 
    Each node contains the attribute metadata and a pointer to the next attribute in order.

ALGORITHM:
    1. Initialize head and tail pointers to NULL.
    2. For each element in attrArray:
        a. Allocate a new AttrDesc node.
        b. Copy the attribute record into the node.
        c. Append the node to the end of the list.
    3. Return the head of the constructed linked list.

ERRORS REPORTED:
    None.

GLOBAL VARIABLES MODIFIED:
    None.

------------------------------------------------------------*/

AttrDesc* BuildAttrList(AttrCatRec **attrArray, int numAttrs)
{
    AttrDesc *head=NULL, *tail=NULL;
    for(int i=0; i<numAttrs; i++)
    {
        AttrDesc *node=malloc(sizeof(AttrDesc));
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


/*------------------------------------------------------------

FUNCTION OpenCats (argc, argv)

PARAMETER DESCRIPTION:
    None. Invoked internally during OpenDB.

FUNCTION DESCRIPTION:
    Opens the catalog relations (relcat and attrcat).
    Loads their metadata from the first page of the catalog files. 
    Initializes the catalog cache entries. 
    Builds attribute lists for each catalog relation. 
    Resets the buffer pool.

ALGORITHM:
    1. Open RELCAT and ATTRCAT files using O_RDWR.
    2. Read the first page of RELCAT into a temporary buffer.
    3. Extract the relcat and attrcat RelCatRec entries from the page.
    4. Validate file descriptors; return error on failure.
    5. Build attribute lists for relcat and attrcat using BuildAttrList().
    6. Populate catcache[0] with relcat metadata.
    7. Populate catcache[1] with attrcat metadata.
    8. Initialize buffer pool entries:
        - Mark dirty = 0, pid = -1, clear page memory.
    9. Return OK.

ERRORS REPORTED:
    - CAT_OPEN_ERROR via global db_err_code if open() fails.

GLOBAL VARIABLES MODIFIED:
    - catcache[]       (slot 0 for relcat, slot 1 for attrcat)
    - buffer[]         (all entries reset)
    - db_open          (set to true)

IMPLEMENTATION NOTES:
    - Record for relcat is in slot 0 of the page 0 (O based indexing of pages and slots)
    - Record for attrcat is in slot 1 of the page 0 (O based indexing of pages and slots)
    - Last three bits of catcache[0].status and catcache[1] should become 1 (for metadata), 1 (for valid) and 0 (for clean) when catalogs are opened. 

------------------------------------------------------------*/

int OpenCats()
{
    int rel_fd = open(RELCAT, O_RDWR);
    int attr_fd = open(ATTRCAT, O_RDWR);
    char page[PAGESIZE];
    RelCatRec Relcat_rc, Relcat_ac;

    if (rel_fd == NOTOK || attr_fd == NOTOK)
    {
        db_err_code = CAT_OPEN_ERROR;
        return NOTOK;
    }

    read(rel_fd, page, PAGESIZE);
    memcpy(&Relcat_rc, page+HEADER_SIZE, sizeof(RelCatRec));
    memcpy(&Relcat_ac, page+HEADER_SIZE+sizeof(RelCatRec), sizeof(RelCatRec));

    AttrCatRec *rel_attrs[] = {
        &Attrcat_rrelName,
        &Attrcat_recLength,
        &Attrcat_recsPerPg,
        &AttrCat_numAttrs,
        &AttrCat_numRecs,
        &AttrCat_numPgs
    };

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

    catcache[0].relcat_rec = Relcat_rc;
    catcache[0].relFile = rel_fd;
    catcache[0].status = (PINNED_MASK | VALID_MASK);
    catcache[0].relcatRid.pid = 0;
    catcache[0].relcatRid.slotnum = 0;
    catcache[0].attrList = BuildAttrList(rel_attrs, RELCAT_NUMATTRS);

    catcache[1].relcat_rec = Relcat_ac;
    catcache[1].relFile = attr_fd;
    catcache[1].status = (PINNED_MASK | VALID_MASK);
    catcache[1].relcatRid.pid = 0;
    catcache[1].relcatRid.slotnum = 1;
    catcache[1].attrList = BuildAttrList(attr_attrs, ATTRCAT_NUMATTRS);

    for (int i = 0; i < MAXOPEN; i++) 
    {
        buffer[i].dirty = 0;
        buffer[i].pid = -1;
        memset(buffer[i].page, 0, sizeof(buffer[i].page));
    }

    return OK;
}
