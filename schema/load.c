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
#include <stddef.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

// Helper function to swap bytes for endianness conversion
static void swap_bytes(char *data, size_t size)
{
    for (size_t i = 0; i < size / 2; i++)
    {
        char temp = data[i];
        data[i] = data[size - 1 - i];
        data[size - 1 - i] = temp;
    }
}

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
        printf("Relation '%s' does NOT exist in the DB.\n", relName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), relName, NULL);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Check whether every component of the path is alphanumeric
    if(!isValidPath(fileName))
    {
        db_err_code = PATH_NOT_VALID;
        return ErrorMsgs(PATH_NOT_VALID, print_flag);
    }

    // Check if source file exists
    if(access(fileName, F_OK))
    {
        if(errno == ENOENT)
        {
            db_err_code = FILE_NO_EXIST;
        }
        else
        {
            db_err_code = FILESYSTEM_ERROR;
        }
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

    // Determine system endianness
    int is_little_endian = 1;
    unsigned int test = 1;
    char *test_ptr = (char*)&test;
    if (test_ptr[0] == 0)
    {
        is_little_endian = 0; // System is big-endian, no conversion needed
    }

    // Pre-compute offsets that need endianness conversion
    int *int_offsets = NULL;
    int *float_offsets = NULL;
    int int_count = 0;
    int float_count = 0;
    
    // First pass: count the number of int and float attributes
    AttrDesc *ptr = catcache[r].attrList;
    while (ptr)
    {
        if (ptr->attr.type[0] == 'i')
        {
            int_count++;
        }
        else if (ptr->attr.type[0] == 'f')
        {
            float_count++;
        }
        ptr = ptr->next;
    }
    
    // Allocate arrays for offsets
    if (int_count > 0)
    {
        int_offsets = malloc(int_count * sizeof(int));
        if (!int_offsets)
        {
            free(recPtr);
            CloseRel(r);
            return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
        }
    }
    
    if (float_count > 0)
    {
        float_offsets = malloc(float_count * sizeof(int));
        if (!float_offsets)
        {
            free(int_offsets);
            free(recPtr);
            CloseRel(r);
            return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
        }
    }
    
    // Second pass: collect the offsets
    ptr = catcache[r].attrList;
    int int_idx = 0, float_idx = 0;
    while (ptr)
    {
        if (ptr->attr.type[0] == 'i')
        {
            int_offsets[int_idx++] = ptr->attr.offset;
        }
        else if (ptr->attr.type[0] == 'f')
        {
            float_offsets[float_idx++] = ptr->attr.offset;
        }
        ptr = ptr->next;
    }

    // Open the file for reading
    FILE *file = fopen(fileName, "rb");
    if (!file)
    {
        db_err_code = FILESYSTEM_ERROR;
        free(int_offsets);
        free(float_offsets);
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

        // Convert integers and floats from big-endian to little-endian if needed
        if (is_little_endian)
        {
            // Convert integers
            for (int i = 0; i < int_count; i++)
            {
                char *int_ptr = recPtr + int_offsets[i];
                swap_bytes(int_ptr, sizeof(int));
            }
            
            // Convert floats
            for (int i = 0; i < float_count; i++)
            {
                char *float_ptr = recPtr + float_offsets[i];
                swap_bytes(float_ptr, sizeof(float));
            }
        }
        
        // Insert the record into the relation
        insertResult = InsertRec(r, recPtr);
        
        if (insertResult == NOTOK)
        {
            // InsertRec failed, db_err_code should already be set by InsertRec
            free(int_offsets);
            free(float_offsets);
            free(recPtr);
            fclose(file);
            CloseRel(r);
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    // Check if we stopped because of read error (not EOF)
    if (!feof(file) && ferror(file))
    {
        db_err_code = FILESYSTEM_ERROR;
        free(int_offsets);
        free(float_offsets);
        free(recPtr);
        fclose(file);
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Clean up
    fclose(file);
    free(int_offsets);
    free(float_offsets);
    free(recPtr);
    
    // Print success message if all records were loaded successfully
    printf("%s successfully loaded with %d tuples.\n", relName, recordsRead);
    CloseRel(r);

    return OK;
}