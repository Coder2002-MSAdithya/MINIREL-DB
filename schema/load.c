#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrec.h"
#include "../include/openrel.h"
#include "../include/closerel.h"
#include "../include/freemap.h"
#include "../include/insertrec.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

int Load(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *fileName = argv[2];

    // Try to open the relation to check if it exists
    int r = OpenRel(relName);
    if (r == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Check if source file exists
    if(access(fileName, F_OK))
    {
        db_err_code = FILE_NO_EXIST;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    int recSize = catcache[r].relcat_rec.recLength;
    int recPerPg = catcache[r].relcat_rec.recsPerPg;
    char *recPtr = malloc(recSize);

    if(!recPtr)
    {
        CloseRel(r);
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    // Open the file for reading
    FILE *file = fopen(fileName, "rb");
    if (!file)
    {
        db_err_code = FILESYSTEM_ERROR;
        free(recPtr);
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Read file fileName record by record each of size recSize
    // and insert each record using InsertRec
    int recordsRead = 0;
    int insertResult = OK;
    
    while (fread(recPtr, recSize, 1, file) == 1)
    {
        recordsRead++;
        
        // Insert the record into the relation
        insertResult = InsertRec(r, recPtr);
        
        if (insertResult == NOTOK)
        {
            // InsertRec failed, db_err_code should already be set by InsertRec
            free(recPtr);
            fclose(file);
            CloseRel(r);
            return ErrorMsgs(db_err_code, print_flag);
        }
        
        // Clear the buffer for the next record
        memset(recPtr, 0, recSize);
    }

    // Check if we stopped because of read error (not EOF)
    if (!feof(file) && ferror(file))
    {
        db_err_code = FILESYSTEM_ERROR;
        free(recPtr);
        fclose(file);
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Clean up
    fclose(file);
    free(recPtr);
    
    // Print success message if all records were loaded successfully
    printf("%s successfully loaded with %d tuples.\n", relName, recordsRead);
    CloseRel(r);

    return OK;
}