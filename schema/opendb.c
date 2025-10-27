#include "../include/defs.h"
#include "../include/helpers.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/opencats.h"
#include "../include/readpage.h"
#include "../include/openrel.h"
#include "../include/closerel.h"
#include "../include/insertrec.h"
#include "../include/deleterec.h"
#include "../include/writerec.h"
#include "../include/findrec.h"
#include "../include/getnextrec.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

/*
 * Function: OpenDB()
 * ------------------------
 * changes the working directory to be the database directory, opens the catalogs and initializes 
 * the various global data structures
 *
 * argv[0] = “opendb”
 * argv[1] = database name
 * argv[argc] = NIL
 *
 *  returns: OK    upon opening database
 *           NOTOK or errorCode upon failure
 *
 * GLOBAL VARIABLES MODIFIED:
 *      DB_DIR is assigned with invoked location of minirel.
 *      db_open is set to true
 *
 * ERRORS REPORTED:
 *      ARGC_INSUFFICIENT
 *      DBNOTCLOSED
 *      DBNOTEXIST
 *
 * ALGORITHM:
 *   1. Checks for Errors.
 *   2. changes to given database directory and then OpenCats.
 *   
 * IMPLEMENTATION NOTES:
 *      Uses OpenCats from physical layer.
 *
 */

int OpenDB(int argc, char **argv)
{
    if(argc < 2)
    {
        return ErrorMsgs(ARGC_INSUFFICIENT, print_flag);
    }

    if(db_open)
    {
        return ErrorMsgs(DBNOTCLOSED, print_flag);
    }

    strncpy(DB_DIR, argv[1], MAX_PATH_LENGTH);

    if(!isValidPath(DB_DIR))
    {
        return ErrorMsgs(DBPATHNOTVALID, print_flag);
    }

    if(chdir(DB_DIR) == NOTOK)
    {
        if(errno == ENOENT)
        {
            return ErrorMsgs(DBNOTEXIST, print_flag);
        }
        else
        {
            return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
        }
    }

    db_open = true;

    //OpenCats also initializes various global data structures
    if(OpenCats() == OK)
    {
        printf("Database %s has been opened successfully for use.\n", DB_DIR);
        // Code just to test InsertRec
        // Student s1 = {"Adithya M S", 24336, 12399.78};
        // Student s2 = {"Medha Dabhi", 24326, 12399.78};
        // Student s3 = {"Sai Dharma", 23334, 12123.32};
        // Student s4 = {"Sujoy Hansda", 23145, 21345.67};

        // Student su1 = {"AdithyaUpdated", 24337, 12100};
        // Student su2 = {"MedhaUpdated", 24327, 12200};
        // Student su3 = {"SaiUpdated", 24937, 12300};
        // Student su4 = {"SujoyUpdated", 24937, 12400};
        
        
        // DeleteRec(r, (Rid){2, 4});
        // DeleteRec(r, (Rid){3, 8});
        // InsertRec(r, &s2);
        // InsertRec(r, &s4);
        // WriteRec(r, &su2, (Rid){2, 4});
        // WriteRec(r, &su4, (Rid){3, 8});
        // CloseRel(r);
        
        // Code to test FindRec
        // int r = OpenRel("Students");
        // Rid foundRid;
        // void *recPtr = malloc(sizeof(Student));
        // float val = 9.00;
        // int val1 = 300;
        // char val2[] = "Belle";
        // Rid startRid = (Rid){1, 8};
        // FindRec(r, startRid, &foundRid, recPtr, 'f', sizeof(float), offsetof(Student, stipend), (void *)(&val), CMP_GTE);
        // printf("Next record found has page number %d and slotnumber %d\n", foundRid.pid, foundRid.slotnum);
        // printf("Name of that person is %s\n", ((Student *)recPtr)->name);
        // CloseRel(r);

        //Code to test GetNextRec
        // int r = OpenRel("Students");
        // Rid startRid = (Rid){1, 7};
        // Rid foundRid = (Rid){-1, -1};
        // void *recPtr = malloc(sizeof(Student));
        // while(startRid.pid != -1 && startRid.slotnum != -1)
        // {
        //     GetNextRec(r, startRid, &startRid, recPtr);
        //     Student s = *(Student *)(recPtr);
        //     if(startRid.pid != -1 && startRid.slotnum != -1)
        //     printf("StudentRec : name = %s | id = %d | stipend = %f\n", s.name, s.id, s.stipend);
        // }
        // CloseRel(r);

        //Modified OpenRel to use FindRec
        // int r = OpenRel("Profs");
        // AttrDesc *p;
        // for(p=catcache[r].attrList;p;p=p->next)
        // {
        //     printf("AttrName : %s\n", (p->attr).attrName);
        // }
        // CloseRel(r);

        // Testing modified OpenRel
        // int r1 = OpenRel("Students");
        // int r2 = OpenRel("Profs");
        // printf("Students : %d | Profs: %d", r1, r2);
    }
    else
    {
        chdir(ORIG_DIR);
        db_open = false;
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    return OK;
}
