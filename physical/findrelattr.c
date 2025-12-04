#include <string.h>
#include "../include/defs.h"
#include "../include/globals.h"


/*--------------------------------------------------------------

FUNCTION FindRelAttr (relNum, attrName):

PARAMETER DESCRIPTION:
    relNum    → Relation number (index into catcache array)
    attrName  → Attribute name to search for within the relation

FUNCTION DESCRIPTION:
    This routine searches the in-memory attribute descriptor list (catcache[relNum].attrList) corresponding to an open relation.
    It scans the linked list of AttrDesc nodes and returns the node whose attr.attrName matches the given attribute name.
    If the attribute is not found, the function returns NULL.
    No errors are reported by this routine; callers must check for NULL to detect failure.

ALGORITHM:
    1) Retrieve the head of the linked list of attribute descriptors for relation relNum.
    2) Sequentially scan each node:
        - Compare attrName with the node’s attr.attrName using bounded string comparison (strncmp).
    3) If a match is found:
        - Return pointer to that AttrDesc node.
    4) If list ends without match:
        - Return NULL.

GLOBAL VARIABLES ACCESSED:
    catcache[] → Reads attrList stored for each open relation.

ERRORS REPORTED:
    None.  

BUGS:
    None known.

---------------------------------------------------------------*/

AttrDesc *FindRelAttr(int relNum, char *attrName)
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