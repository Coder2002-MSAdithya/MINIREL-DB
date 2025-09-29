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
                printf("Error %d: Insufficient number of arguments passed.\n"
                       "→ Ensure you provide the required arguments in the correct order.\n",
                       errorNum);
                break;

            case METADATA_SECURITY:
                printf("Error %d: Metadata table modification error.\n"
                       "→ Direct modification of system metadata is not allowed.\n"
                       "  Use provided APIs or commands to alter metadata safely.\n",
                       errorNum);
                break;

            case RELNOEXIST:
                printf("Error %d: Relation does not exist.\n"
                       "→ Check the relation name for typos or create the relation before use.\n",
                       errorNum);
                break;

            case RELEXIST:
                printf("Error %d: Relation already exists.\n"
                       "→ Choose a different relation name or drop the existing relation first.\n",
                       errorNum);
                break;

            case REL_LENGTH_EXCEEDED:
                printf("Error %d: Relation name too long.\n"
                       "→ Use a shorter name within the allowed length.\n",
                       errorNum);
                break;

            case ATTRNOEXIST:
                printf("Error %d: Attribute does not exist.\n"
                       "→ Verify that the attribute name is correct and exists in the relation.\n",
                       errorNum);
                break;

            case ATTREXIST:
                printf("Error %d: Attribute already exists.\n"
                       "→ Use a unique attribute name when creating or altering a table.\n",
                       errorNum);
                break;

            case ATTR_NAME_EXCEEDED:
                printf("Error %d: Attribute name too long.\n"
                       "→ Shorten the attribute name to meet the allowed length.\n",
                       errorNum);
                break;

            case DUP_ATTR:
                printf("Error %d: Duplicate attributes in table definition.\n"
                       "→ Remove duplicate attributes or rename them uniquely.\n",
                       errorNum);
                break;
            
            case CAT_CREATE_ERROR:
                printf("Error %d: The catalog files could NOT be created.\n"
                "Please try again after closing any processes that are using this directory.\n", errorNum);
                break;
            
            case CAT_OPEN_ERROR:
                printf("Could NOT open catalogs for this database.\n"
                "Maybe you have chosen an invalid directory.\n");
                break;

            case DBNOTEXIST:
                printf("Error %d: Database does not exist.\n"
                       "→ Verify the database name or create the database first.\n",
                       errorNum);
                break;
            
            case DBPREFIXNOTFOUND:
                    printf("Error %d: Prefix to your Database path does NOT exist.\n"
                    "Implementation ONLY supports ONE level of directory creation", errorNum);

            case DBEXISTS:
                printf("Error %d: Database already exists.\n"
                       "→ Choose another name or drop the existing database.\n",
                       errorNum);
                break;

            case DB_LENGTH_EXCEEDED:
                printf("Error %d: Database name too long.\n"
                       "→ Use a shorter database name within the allowed length.\n",
                       errorNum);
                break;

            case DBNOTCLOSED:
                printf("Error %d: Current database not yet closed.\n"
                       "→ Close the currently open database before performing this operation.\n",
                       errorNum);
                break;

            case DBNOTOPEN:
                printf("Error %d: No database is currently open.\n"
                       "→ Open a database before attempting this operation.\n",
                       errorNum);
                break;

            case DBPATHNOTVALID:
                    printf("Error %d: The path you have provided for DB creation contains illegal characters.\n" 
                        "Only paths containing alphabets and digits other than / are allowed.\n", errorNum);
                    break;
            
            case DBDESTROYERROR:
                    printf("Deletion of DB directory NOT successful..\n");
                    break;

            case FILESYSTEM_ERROR:
                printf("Error %d: File system error occurred.\n"
                       "→ Check OS-level file permissions, disk space, or path validity.\n",
                       errorNum);
                break;

            case UNKNOWN_ERROR:
                printf("Error %d: Unknown error encountered.\n"
                       "→ Recheck the operation. If the issue persists, consult logs or support.\n",
                       errorNum);
                break;

            default:
                printf("Error %d: Unrecognized error code.\n"
                       "→ Check if the error code is defined in errors.h.\n",
                       errorNum);
                break;
        }
    }
    return errorNum;
}
