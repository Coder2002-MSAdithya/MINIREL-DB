#include <stdio.h>
#include <stdbool.h>
#include "../include/error.h"

/*
 * ErrorMsgs
 * ----------
 * errorNum  : Error message number (see errors.h for codes).
 * printFlag : If non-zero, prints out the corresponding error message.
 *
 * Returns the same errorNum for use in `return ErrorMsgs(...)` calls.
 */

int ErrorMsgs(int errorNum, bool printFlag)
{
    if(printFlag)
    {
        switch(errorNum)
        {
            case ARGC_INSUFFICIENT:
                printf("Error %d: Insufficient number of arguments passed. \n"
                       "→ Ensure you provide the required arguments in the correct order. \n\n",
                       errorNum);
                break;

            case METADATA_SECURITY:
                printf("Error %d: Metadata table modification error. \n"
                       "→ Direct modification of system metadata is not allowed. \n"
                       "  Use provided APIs or commands to alter metadata safely. \n\n",
                       errorNum);
                break;
            
            case BUFFER_FULL:
                printf("Error %d: Buffer is full. \n"
                       "→ Please close a relation to open new relation. \n\n",
                       errorNum);
                break;

            case INVALID_FORMAT:
                printf("Error %d: Attribute format is invalid. \n"
                       "→ Please enter i for integer, f for floating point and sN for string format with maxixmum N length. \n\n",
                       errorNum);
                break;

            case STR_LEN_INVALID:
                printf("Error %d: Length of string type attributes must be between 0 and 50. \n\n", errorNum);
                break;

            case TOO_MANY_ARGS:
                printf("Error %d: More number of arguments passed than required. \n"
                       "→ Ensure you provide the required arguments in the correct order. \n\n",
                       errorNum);
                break;
            
            case FILE_NO_EXIST:
                printf("Error %d: File that you are trying to load from does'nt exist. \n\n", errorNum);
                break;
            
            case LOAD_NONEMPTY:
                printf("Error %d: CANNOT load data into a non-empty relation. \n\n", errorNum);
                break;

            case RELNOEXIST:
                printf("Error %d: Relation does not exist. \n"
                       "→ Check the relation name for typos or create the relation before use. \n\n",
                       errorNum);
                break;
            
            case REL_OPEN_ERROR:
                printf("Error %d: Relation file could not be opened. \n\n", errorNum);
                break;

            case REL_CLOSE_ERROR:
                printf("Error %d: Relation file could not be closed. \n\n", errorNum);
                break;

            case RELEXIST:
                printf("Error %d: Relation already exists. \n"
                       "→ Choose a different relation name or drop the existing relation first. \n\n",
                       errorNum);
                break;

            case REL_LENGTH_EXCEEDED:
                printf("Error %d: Relation name too long. \n"
                       "→ Use a shorter name within the allowed length. \n\n",
                       errorNum);
                break;

            case INVALID_RELNUM:
                printf("Error %d: Relation number is invalid. \n\n", errorNum);
                break;

            case ATTRNOEXIST:
                printf("Error %d: Attribute does not exist. \n"
                        "→ Verify that the attribute name is correct and exists in the relation. \n\n",
                        errorNum);
                break;

            case ATTREXIST:
                printf("Error %d: Attribute already exists. \n"
                        "→ Use a unique attribute name when creating or altering a table. \n\n",
                        errorNum);
                break;

            case ATTR_NAME_EXCEEDED:
                printf("Error %d: Attribute name too long. \n"
                        "→ Shorten the attribute name to meet the allowed length. \n\n",
                        errorNum);
                break;

            case DUP_ATTR:
                printf("Error %d: Duplicate attributes in table definition. \n"
                        "→ Remove duplicate attributes or rename them uniquely. \n\n",
                        errorNum);
                break;
            
            case CAT_CREATE_ERROR:
                printf("Error %d: The catalog files could NOT be created. \n"
                        "Please try again after closing any processes that are using this directory. \n\n", 
                        errorNum);
                break;

            case CAT_DELETE_ERROR:
                printf("Error %d: Systems catalogs can't be deleted. \n", errorNum);
                break;
            
            case CAT_OPEN_ERROR:
                printf("Error %d: Could NOT open catalogs for this database. \n"
                        "Maybe you have chosen an invalid directory. \n\n", 
                        errorNum);
                break;

            case CAT_CLOSE_ERROR:
                printf("Error %d: Could NOT close catalogs for this database. \n\n", errorNum);
                break;

            case CAT_ALREADY_EXISTS:
                printf("Error %d: Something went wrong. Catalogs already exist. \n\n", errorNum);
                break;

            case MEM_ALLOC_ERROR:
                printf("Error %d: Memory could NOT be allocated to perform this operation. \n\n", errorNum);
                break;
            
            case REC_TOO_LONG:
                printf("Error %d: This relation CANNOT be created as its record would NOT fit within a page. \n\n", errorNum);
                break;

            case DBNOTEXIST:
                printf("Error %d: Database does not exist. \n"
                        "→ Verify the database name or create the database first. \n\n", 
                        errorNum);
                break;
            
            case DBPREFIXNOTFOUND:
                printf("Error %d: Prefix to your Database path does NOT exist. \n"
                        "→ Implementation ONLY supports ONE level of directory creation. \n\n", 
                        errorNum);
                break;

            case DBEXISTS:
                printf("Error %d: Database already exists. \n"
                        "→ Choose another name or drop the existing database. \n\n", 
                        errorNum);
                break;

            case DB_LENGTH_EXCEEDED:
                printf("Error %d: Database name too long. \n"
                        "→ Use a shorter database name within the allowed length. \n\n",
                        errorNum);
                break;

            case DBNOTCLOSED:
                printf("Error %d: Current database not yet closed. \n"
                        "→ Close the currently open database before performing this operation. \n\n",
                        errorNum);
                break;

            case DBNOTOPEN:
                printf("Error %d: No database is currently open. \n"
                        "→ Open a database before attempting this operation. \n\n",
                        errorNum);
                break;

            case DBPATHNOTVALID:
                    printf("Error %d: The path you have provided for DB creation contains illegal characters. \n" 
                            "Only paths containing alphabets and digits other than / are allowed. \n\n", 
                            errorNum);
                    break;
            
            case DBDESTROYERROR:
                    printf("Error %d: Deletion of DB directory NOT successful.. \n\n", errorNum);
                    break;

            case FILESYSTEM_ERROR:
                printf("Error %d: File system error occurred. \n"
                        "→ Check OS-level file permissions, disk space, or path validity. \n\n",
                        errorNum);
                break;

            case INVALID_FILE_SIZE:
                printf("Error %d: Trying to load an invalid file. \n\n", errorNum);
                break;

            case PAGE_OUT_OF_BOUNDS:
                printf("Error %d: The page is not in given relation. \n\n", errorNum);
                break;

            case PAGE_MAGIC_ERROR:
                printf("Error %d: The page does not belong to MINIREL. \n\n", errorNum);
                break;

            case UNKNOWN_ERROR:
                printf("Error %d: Unknown error encountered. \n"
                        "→ Recheck the operation. If the issue persists, consult logs or support. \n\n",
                        errorNum);
                break;

            default:
                printf("Error %d: Unrecognized error code. \n"
                        "→ Check if the error code is defined in errors.h. \n\n",
                        errorNum);
                break;
        }
    }
    return errorNum;
}
