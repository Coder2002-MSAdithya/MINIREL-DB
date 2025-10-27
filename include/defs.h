/********************************************************** 
			This is the basic definition file.
***********************************************************/

#ifndef TYPES_MINIREL
#define TYPES_MINIREL

/*************************************************************
							CONSTANTS							
*************************************************************/
#define PAGESIZE	        512	    /* number of bytes in a page */
#define SLOTMAP		        8	    /* size of slotmap in bytes */
#define MAGIC_SIZE          8       /* size of magic number for page */
#define HEADER_SIZE	        16	    /* number of bytes in header */
#define	MAXRECORD	        (PAGESIZE - HEADER_SIZE)	/* PAGESIZE minus number of bytes taken up for header */
#define RELNAME		        20	    /* max length of a relation name */
#define MAXOPEN		        20      /* max number of files that can be open at the same time */
#define ATTRNAME	        20      /* max length of an attribute name */
#define MAX_PATH_LENGTH		1024    /*max length of a path passed as command line arg */
#define CMD_LENGTH          2048    /* Length of a command string */
#define MAX_N               50      /* Max length of a string field */

#define DIRTY_MASK          1 /*LSB of status field of cache entry represents dirty*/
#define VALID_MASK          2 /*2nd least significant bit of status field represents valid bit*/
#define PINNED_MASK         4 /*3rd least significant bit represents whether it's a catalog relation*/

#define	OK			0	/* return codes */
#define NOTOK		-1

#define RELCAT		"relcat"   /* name of the relation catalog file */
#define ATTRCAT		"attrcat"  /* name of the attribute catalog file */
#define GEN_MAGIC   "MINIREL"  /* Common part of MAGIC BYTES of all relation files */

#define CMP_EQ  501
#define CMP_GTE 502
#define CMP_GT  503
#define CMP_LTE 504
#define CMP_NE  505
#define CMP_LT  506 

#define EPSILON 1e-6

#define FLOAT_REL_EPS 1e-6
#define FLOAT_ABS_EPS 1e-9

#define RELCAT_CACHE    0
#define ATTRCAT_CACHE   1

#define RELCAT_NUMATTRS  6
#define ATTRCAT_NUMATTRS 5

#define NUM_CATS         2

#define NT_RELCAT 2
#define NT_ATTRCAT 7

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*************************************************************
						TYPE DEFINITIONS 
*************************************************************/
typedef unsigned short int pageid_t;
typedef unsigned short int slotnum_t;
typedef unsigned int uint32_t;

/* Rid Structure */
typedef struct recid {
	short	pid;
	short	slotnum;
} Rid;

/* Page Structure */
typedef struct ps 
{
	unsigned long slotmap;
    char magicString[MAGIC_SIZE];
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
    char type;                      // 'i', 'f', 's'
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
    RelCatRec relcat_rec;           // relation catalogue record
    int relFile;            		// file descriptor
    int status;                     // LSB is for dirty and 2nd LSB for valid/invalid
    uint32_t timestamp;         //  4 byte UNIX timestamp to implement LRU policy
    AttrDesc *attrList; 		    // linked list of attributes
} CacheEntry;

typedef struct buffer 
{
	// int relNum;						// relation id
    char page[PAGESIZE];  			// page content
    int dirty;            			// 1 if modified
    short pid;              		// which page is stored here					// file descriptor of a relation
} Buffer;

typedef struct Student
{
    char name[40];
    int id;
    float stipend;
} Student;

typedef struct Professor
{
    char name[30];
    int id;
    float salary;
    char designation[30];
} Professor;

#endif

/*****************************************************************/

