/********************************************************** 
			This is the basic definition file.
***********************************************************/

#ifndef TYPES_MINIREL
#define TYPES_MINIREL

/*************************************************************
							CONSTANTS							
*************************************************************/
#define PAGESIZE	512	/* number of bytes in a page */
#define SLOTMAP		8	/* size of slotmap in bytes */
#define HEADER_SIZE	16	/* number of bytes in header */
#define	MAXRECORD	(PAGESIZE - SLOTMAP)	/* PAGESIZE minus number of bytes taken up for slot map */
#define RELNAME		20	/* max length of a relation name */
#define MAXOPEN		20  /* max number of files that can be open at the same time */
#define ATTRNAME	20  /* max length of an attribute name */
#define MAX_PATH_LENGTH		1024 /*max length of a path passed as command line arg */

#define	OK			0	/* return codes */
#define NOTOK		-1

#define RELCAT		"relcat"   /* name of the relation catalog file */
#define ATTRCAT		"attrcat"  /* name of the attribute catalog file */

/*************************************************************
						TYPE DEFINITIONS 
*************************************************************/

/* Rid Structure */
typedef struct recid {
	short	pid;
	short	slotnum;
} Rid;

/* Page Structure */
typedef struct ps 
{
	unsigned slotmap;
	char contents [MAXRECORD];
} Page;

typedef struct relcat_rec 
{
    char relName[RELNAME];  		// relation name
    int recLength;              	// record length in bytes
    int recsPerPg;              	// records per page
    int numAttrs;               	// number of attributes
    int numRecs;                	// number of records
    int numPgs;                 	// number of pages
} RelCatRec;

typedef struct attrcat_rec 
{
    int offset;                 	// offset of attribute within record
    int length;                 	// length of attribute
    char type;                    	// 'i', 'f', 's'
    char attrName[ATTRNAME]; 		// attribute name
    char relName[RELNAME];   		// relation name it belongs to
} AttrCatRec;

typedef struct attrDesc 
{
    AttrCatRec attr;  				// attribute catalog record
    struct attrDesc *next;			// pointer to next attribute catalog record
} AttrDesc;

typedef struct cacheentry {
	Rid relcatRid;          		// catalog record RID
    char relName[RELNAME];			// relation name
    int recLength;					// record length in bytes
    int recsPerPg;					// records per page
    int numAttrs;					// number of attributes
    int numRecs;					// number of records
    int numPgs;						// number of pages
    int relFile;            		// file descriptor
    int dirty;              		// 0 = clean, 1 = modified
    AttrDesc *attrList; 		// linked list of attributes
} CacheEntry;

typedef struct buffer 
{
	int relId;						// relation id
    char page[PAGESIZE];  			// page content
    int dirty;            			// 1 if modified
    short pid;              		// which page is stored here
	int relFile;					// file descriptor of a relation
} Buffer;

#endif

/*****************************************************************/

