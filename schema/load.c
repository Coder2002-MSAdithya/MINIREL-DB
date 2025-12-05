/************************INCLUDES*******************************/
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrec.h"
#include "../include/openrel.h"
#include "../include/closerel.h"
#include "../include/freemap.h"
#include "../include/insertrec.h"
#include "../include/unpinrel.h"
#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


/*------------------------------------------------------------

FUNCTION swap_bytes (data, size)

PARAMETER DESCRIPTION:
    data : Pointer to a byte-buffer containing a single value.
    size : Number of bytes making up that value (typically sizeof(int) = 4 or sizeof(float) = 4).

FUNCTION DESCRIPTION:
    This routine reverses the byte order of the object stored at *data. 
    It is used to convert values from little-endian format to big-endian format (or vice-versa).

ALGORITHM:
    1) Loop i from 0 to [(size/2)-1].
    2) For each i, swap data[i] with data[size-1-i].
    3) After the loop, the buffer pointed to by 'data' has reversed byte ordering.

ERRORS REPORTED:
    None. The caller must ensure:
        - 'data' is not NULL.
        - 'size' is the correct width of the object being swapped.

GLOBAL VARIABLES MODIFIED:
       None.

IMPLEMENTATION NOTES:
    - This is a low-level helper and does NOT perform alignment checks.
    - Must be used consistently when reading binary files that were produced on systems of different endian architectures.
    - Only needed on little-endian systems when data arrives in big-endian form.

------------------------------------------------------------*/

static void swap_bytes(char *data, size_t size)
{
    for (size_t i = 0; i < size / 2; i++)
    {
        char temp = data[i];
        data[i] = data[size - 1 - i];
        data[size - 1 - i] = temp;
    }
}


/*------------------------------------------------------------

FUNCTION Load (argc, argv)

PARAMETER DESCRIPTION:
    argc : Number of arguments in the command.
    argv : Argument vector:
    
SPECIFICATIONS:
    argv[0] = "load"
    argv[1] = relation name
    argv[2] = filename containing raw records
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    The LOAD command bulk-loads binary tuples from an external file directly into an existing MINIREL relation. 
    Each tuple in the external file must already be laid out in the exact binary format of the target relation schema (i.e., attribute offsets, sizes, and total record length must match).
    The routine:
        - Verifies that the database is open.
        - Opens the target relation and confirms it exists.
        - Validates the filepath syntax and checks existence of the external file.
        - Determines the system's endian architecture.
        - Precomputes offsets of INT and FLOAT attributes for on-the-fly byte-swapping.
        - Reads one record at a time from the source file.
        - Performs endian conversion for INT and FLOAT attributes (external file is assumed big-endian; host system may be little-endian).
        - Inserts each tuple using InsertRec(), thereby updating the relation file and freemap.
        - Stops on any failed INSERT and reports the appropriate error.
    Upon successful completion, all tuples in the external file appear in the relation (unless InsertRec rejected some tuple due to duplicate keys or other constraints).

ALGORITHM:
    1) Verify db_open; else report DBNOTOPEN.
    2) Attempt OpenRel(relName); if fails, print catalog-based error.
    3) Validate fileName with isValidPath(); reject illegal characters.
    4) Check file existence via access(); otherwise FILE_NO_EXIST or FILESYSTEM_ERROR.
    5) Lookup record size and attribute descriptors from catcache.
    6) Allocate record buffer recPtr.
    7) Detect host endianness.
    8) Traverse attribute list twice:
        a) First pass → count number of INT and FLOAT attributes.
        b) Second pass → record their byte offsets inside the tuple.
    9) fopen() the external file in "rb" mode.
    10) Repeatedly fread() tuples of size recSize:
        a) If system is little-endian, swap bytes of all INT/FLOAT fields.
        b) Call InsertRec() to append tuple into the file.
        c) Stop immediately if InsertRec() returns NOTOK.
    11) Check for incomplete read or file error → FILESYSTEM_ERROR.
    12) Free all allocated buffers and close file.
    13) CloseRel() the target relation.
    14) Print success message indicating number of tuples loaded.

ERRORS REPORTED:
    DBNOTOPEN        – No open database.
    RELNOEXIST       – Target relation not found.
    PATH_NOT_VALID   – External filename contains illegal characters.
    FILE_NO_EXIST    – External file does not exist.
    FILESYSTEM_ERROR – OS-level I/O failure while opening or reading file.
    MEM_ALLOC_ERROR  – Failed to allocate record buffer or offset arrays.
    REC_INS_ERR      – InsertRec() failure for a particular tuple.
    Any InsertRec()-level error (e.g., DUP_ROWS, REL_PAGE_LIMIT_REACHED) is also propagated.

GLOBAL VARIABLES MODIFIED:
    db_err_code
    Relation file and freemap files (via InsertRec)
    Catalog entry for numRecs via InsertRec()

IMPLEMENTATION NOTES:
    - MINIREL assumes the load file is a raw binary file produced in big-endian attribute layout; this routine converts to host byte order automatically.
    - Load files must strictly follow the schema layout.
    - The function does not allow loading into non-existent or schema-mismatched relations; InsertRec() enforces per-tuple constraints such as duplicate prevention.

------------------------------------------------------------*/
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
        if(db_err_code == RELNOEXIST)
        {
            printf("Relation '%s' does NOT exist in the DB.\n", relName);
            printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), relName, NULL);
        }
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
        is_little_endian = 0; // system is big-endian
    }

    // Pre-compute offsets that need endianness conversion
    int *int_offsets = NULL;
    int *float_offsets = NULL;
    int int_count = 0;
    int float_count = 0;
    int string_count = 0;

    // First pass: count the number of int, float and string attributes
    AttrDesc *ptr = catcache[r].attrList;
    while (ptr)
    {
        char t = ptr->attr.type[0];
        if (t == 'i')
        {
            int_count++;
        }
        else if (t == 'f')
        {
            float_count++;
        }
        else if (t == 's')  // string attribute (length includes trailing NUL)
        {
            string_count++;
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
        char t = ptr->attr.type[0];
        if (t == 'i')
        {
            int_offsets[int_idx++] = ptr->attr.offset;
        }
        else if (t == 'f')
        {
            float_offsets[float_idx++] = ptr->attr.offset;
        }
        ptr = ptr->next;
    }

    // Compute on-disk record size: each string attribute on disk lacks the trailing NUL
    int fileRecSize = recSize - string_count; // subtract 1 byte per string attribute

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

    // Temporary buffer for on-disk record
    char *fileBuf = malloc(fileRecSize);
    if (!fileBuf)
    {
        db_err_code = MEM_ALLOC_ERROR;
        free(int_offsets);
        free(float_offsets);
        free(recPtr);
        fclose(file);
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Read file record by record (on-disk size fileRecSize) and reconstruct in-memory record
    int recordsRead = 0;
    int insertResult = OK;

    while (fread(fileBuf, fileRecSize, 1, file) == 1)
    {
        recordsRead++;

        // Zero record buffer first to clean any padding bytes
        memset(recPtr, 0, recSize);

        // Reconstruct the in-memory record from the on-disk buffer
        int file_pos = 0;
        ptr = catcache[r].attrList;
        while (ptr)
        {
            int offset = ptr->attr.offset;
            int length = ptr->attr.length; // for strings this includes NUL
            char t = ptr->attr.type[0];

            if (t == 's')
            {
                // Copy length - 1 bytes from file, then set terminating NUL in recPtr
                int copy_len = length - 1;
                if (copy_len > 0)
                {
                    memcpy(recPtr + offset, fileBuf + file_pos, copy_len);
                }
                // ensure terminating NUL is present in the in-memory record
                recPtr[offset + copy_len] = '\0';
                file_pos += copy_len;
            }
            else
            {
                // Non-string attribute: copy full length bytes from file
                memcpy(recPtr + offset, fileBuf + file_pos, length);
                file_pos += length;
            }

            ptr = ptr->next;
        }

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
            free(fileBuf);
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
        free(fileBuf);
        fclose(file);
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Clean up
    fclose(file);
    free(int_offsets);
    free(float_offsets);
    free(recPtr);
    free(fileBuf);

    // Print success message if all records were loaded successfully
    printf("%s successfully loaded with %d tuples.\n", relName, recordsRead);
    UnPinRel(r);

    return OK;
}