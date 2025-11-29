#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/helpers.h"   /* for compareVals, etc. */
#include "../include/bptree.h"    /* declare the public prototypes here */

/* ============================================================
 * B+ TREE CONSTANTS / LAYOUT
 * ============================================================ */

#define BPT_META_MAGIC   "BPTREE"
#define BPT_NODE_MAGIC   "BPNODE"
#define NODE_HDR_SIZE    32   /* bytes reserved for node header */

#define BPT_LEAF         'L'
#define BPT_INTERNAL     'I'

#define INS_NO_FREE_SLOT  2   /* internal code: page has no free slot */

/* Convenience macros for index buffer */
#define IDX_BUF(relNum)   (idx_buffer[(relNum)])
#define IDX_PAGE(relNum)  (idx_buffer[(relNum)].buffer.page)

/* ============================================================
 * META PAGE STRUCTURE (PAGE 0)
 * ============================================================ */

typedef struct __attribute__((packed)) {
    char magic[8];    // "BPTREE\0"
    short rootPid;    // >=1 if tree has root, 0 if empty
    short keyLength;  // bytes
    char  keyType;    // 'i','f','s'
    char  padding[PAGESIZE - 8 - 2 - 2 - 1];
} BPTMetaPage;

/* ============================================================
 * FILENAME & FILE HELPERS
 * ============================================================ */

/* Exactly: <relName>.<attrName>.bpidx in current directory */
static void build_index_filename(AttrCatRec *attrPtr, char *fname, size_t n)
{
    snprintf(fname, n, "%s.%s.bpidx", attrPtr->relName, attrPtr->attrName);
}

/* Low-level: open index file in given mode */
static FILE *open_index_file(AttrCatRec *attrPtr, const char *mode)
{
    char path[MAX_PATH_LENGTH];
    build_index_filename(attrPtr, path, sizeof(path));
    FILE *fp = fopen(path, mode);
    if (!fp) {
        db_err_code = FILESYSTEM_ERROR;
    }
    return fp;
}

/* Read meta page 0 into a local struct. Return OK/NOTOK. */
static int read_meta_page(AttrCatRec *attrPtr, BPTMetaPage *meta)
{
    FILE *fp = open_index_file(attrPtr, "rb");
    if (!fp) {
        return NOTOK;
    }
    if (fread(meta, 1, sizeof(BPTMetaPage), fp) != sizeof(BPTMetaPage)) {
        fclose(fp);
        db_err_code = INVALID_FILE_SIZE;
        return NOTOK;
    }
    fclose(fp);

    if (strncmp(meta->magic, BPT_META_MAGIC, strlen(BPT_META_MAGIC)) != 0) {
        db_err_code = PAGE_MAGIC_ERROR;
        return NOTOK;
    }
    return OK;
}

/* Write meta page 0 */
static int write_meta_page(AttrCatRec *attrPtr, BPTMetaPage *meta)
{
    FILE *fp = open_index_file(attrPtr, "rb+");
    if (!fp) {
        return NOTOK;
    }
    if (fwrite(meta, 1, sizeof(BPTMetaPage), fp) != sizeof(BPTMetaPage)) {
        fclose(fp);
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }
    fclose(fp);
    return OK;
}

/* ============================================================
 * NODE HEADER HELPERS (IN-BUFFER)
 * ============================================================ */

static void init_node_header(char *page, char nodeType, short parentPid)
{
    memset(page, 0, PAGESIZE);
    memcpy(page, BPT_NODE_MAGIC, strlen(BPT_NODE_MAGIC));   // magic
    page[8]  = nodeType;   // nodeType
    page[9]  = 0;          // reserved
    *(short *)(page + 10) = 0;          // numKeys
    *(short *)(page + 12) = parentPid;  // parentPid
    *(short *)(page + 14) = -1;         // nextLeafPid (only for leaves; -1 default)
    *(short *)(page + 16) = 0;          // reserved
    /* rest is zero from memset */
}

static char get_node_type(char *page)
{
    return page[8];
}

static short get_num_keys(char *page)
{
    return *(short *)(page + 10);
}

static void set_num_keys(char *page, short n)
{
    *(short *)(page + 10) = n;
}

static short get_parent_pid(char *page)
{
    return *(short *)(page + 12);
}

static void set_parent_pid(char *page, short p)
{
    *(short *)(page + 12) = p;
}

static short get_next_leaf(char *page)
{
    return *(short *)(page + 14);
}

static void set_next_leaf(char *page, short p)
{
    *(short *)(page + 14) = p;
}

/* Data area starts after header */
static inline char *node_data(char *page)
{
    return page + NODE_HDR_SIZE;
}

/* ============================================================
 * INDEX BUFFER MANAGEMENT
 * ============================================================ */

/* 
 * ReadBPTreePage:
 *   Ensure that idx_buffer[relNum] contains page pidx of the index file
 *   associated with attrPtr.
 *
 *   If another index page is in the buffer and dirty, flush it first.
 */
int ReadBPTreePage(int relNum, AttrCatRec *attrPtr, short pidx)
{
    if (relNum < 0 || relNum >= MAXOPEN) {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    IdxBuf *ib = &IDX_BUF(relNum);

    /* If the desired page is already loaded and matches attrPtr, nothing to do */
    if (ib->valid &&
        ib->attrCatRecPtr == attrPtr &&
        ib->buffer.pid == pidx)
    {
        return OK;
    }

    /* If buffer has some index loaded and dirty, flush it */
    if (ib->valid && ib->buffer.dirty) {
        int rc = FlushBPTreePage(relNum, ib->attrCatRecPtr);
        if (rc == NOTOK) return NOTOK;
    }

    /* Now load requested page */
    FILE *fp = open_index_file(attrPtr, "rb");
    if (!fp) {
        db_err_code = IDXNOEXIST;
        return NOTOK;
    }

    if (fseek(fp, (long)pidx * PAGESIZE, SEEK_SET) != 0) {
        fclose(fp);
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    size_t n = fread(ib->buffer.page, 1, PAGESIZE, fp);
    fclose(fp);
    if (n != PAGESIZE) {
        db_err_code = INVALID_FILE_SIZE;
        return NOTOK;
    }

    ib->buffer.pid   = pidx;
    ib->buffer.dirty = 0;
    ib->valid        = 1;
    ib->attrCatRecPtr = attrPtr;

    return OK;
}

/*
 * FlushBPTreePage:
 *   If idx_buffer[relNum] holds a dirty page for this attrPtr, write it back.
 */
int FlushBPTreePage(int relNum, AttrCatRec *attrPtr)
{
    if (relNum < 0 || relNum >= MAXOPEN) {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    IdxBuf *ib = &IDX_BUF(relNum);
    if (!ib->valid ||
        ib->attrCatRecPtr != attrPtr ||
        !ib->buffer.dirty)
    {
        return OK;  // nothing to do
    }

    FILE *fp = open_index_file(attrPtr, "rb+");
    if (!fp) return NOTOK;

    if (fseek(fp, (long)ib->buffer.pid * PAGESIZE, SEEK_SET) != 0) {
        fclose(fp);
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    size_t n = fwrite(ib->buffer.page, 1, PAGESIZE, fp);
    fclose(fp);
    if (n != PAGESIZE) {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    ib->buffer.dirty = 0;
    return OK;
}

/* ============================================================
 * KEY UTILITIES
 * ============================================================ */

static int key_size_bytes(AttrCatRec *attrPtr)
{
    switch (attrPtr->type) {
        case 'i': return sizeof(int);
        case 'f': return sizeof(float);
        case 's': return attrPtr->length;
        default:  return -1;
    }
}

/* ============================================================
 * INTERNAL FORWARD DECLARATIONS
 * ============================================================ */

static int bptree_insert_key(AttrCatRec *attrPtr, int relNum,
                             void *valuePtr, Rid insertRid);
static int bptree_find_key(AttrCatRec *attrPtr, int relNum,
                           Rid startRid, Rid *foundRid, void *valuePtr, int cmpOp);
static int bptree_delete_key(AttrCatRec *attrPtr, int relNum,
                             void *valuePtr, Rid *removeRid);
static int find_leaf(AttrCatRec *attrPtr, int relNum, void *valuePtr, short *leafPidOut);
static int dump_node_recursive(int relNum, AttrCatRec *attrPtr,
                               short pid, int level);

/* ============================================================
 * PUBLIC API: CREATE / DESTROY
 * ============================================================ */

/*
 * CreateBPTree:
 *   Create a new index file for (relNum, attrPtr).
 *   - Fails with IDXEXIST if file already exists or attrPtr->hasIndex != 0.
 *   - Fails with INDEX_NONEMPTY if relation already has records.
 *   - Initializes meta page and a single empty leaf root at page 1.
 */
int CreateBPTree(int relNum, AttrCatRec *attrPtr)
{
    if (!db_open) {
        db_err_code = DBNOTOPEN;
        return NOTOK;
    }

    if (attrPtr->hasIndex) {
        db_err_code = IDXEXIST;
        return NOTOK;
    }

    /* Relation must be empty */
    CacheEntry *entry = &catcache[relNum];
    if (!(entry->status & VALID_MASK)) {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }
    if (entry->relcat_rec.numRecs > 0) {
        db_err_code = INDEX_NONEMPTY;
        return NOTOK;
    }

    /* Create file if it does not exist */
    char path[MAX_PATH_LENGTH];
    build_index_filename(attrPtr, path, sizeof(path));

    FILE *test = fopen(path, "rb");
    if (test) {
        fclose(test);
        db_err_code = IDXEXIST;
        return NOTOK;
    }

    FILE *fp = fopen(path, "wb+");
    if (!fp) {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    /* Write meta page */
    BPTMetaPage meta;
    memset(&meta, 0, sizeof(meta));
    memcpy(meta.magic, BPT_META_MAGIC, strlen(BPT_META_MAGIC));
    meta.rootPid   = 1;  // first node page
    meta.keyLength = key_size_bytes(attrPtr);
    meta.keyType   = attrPtr->type;

    if (fwrite(&meta, 1, sizeof(meta), fp) != sizeof(meta)) {
        fclose(fp);
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    /* Initialize root node page as empty leaf */
    char nodePage[PAGESIZE];
    init_node_header(nodePage, BPT_LEAF, 0);  // parent = 0 (root)

    if (fwrite(nodePage, 1, PAGESIZE, fp) != PAGESIZE) {
        fclose(fp);
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    fclose(fp);

    /* Update in-memory attr catalog fields (caller must persist if needed) */
    attrPtr->hasIndex = 1;
    attrPtr->nPages   = 2;  // meta + one leaf
    attrPtr->nKeys    = 0;

    return OK;
}

/*
 * DestroyBPTree:
 *   Drop the index file and clear catalog info.
 */
int DestroyBPTree(int relNum, AttrCatRec *attrPtr)
{
    if (!db_open) {
        db_err_code = DBNOTOPEN;
        return NOTOK;
    }

    if (!attrPtr->hasIndex) {
        db_err_code = IDXNOEXIST;
        return NOTOK;
    }

    /* Flush any buffered page */
    FlushBPTreePage(relNum, attrPtr);
    idx_buffer[relNum].valid = 0;
    idx_buffer[relNum].attrCatRecPtr = NULL;

    /* Remove file */
    char path[MAX_PATH_LENGTH];
    build_index_filename(attrPtr, path, sizeof(path));

    if (remove(path) != 0) {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    attrPtr->hasIndex = 0;
    attrPtr->nPages   = 0;
    attrPtr->nKeys    = 0;

    return OK;
}

/* ============================================================
 * PUBLIC API: ADD / REMOVE / FIND
 * ============================================================ */

int AddKeytoBPTree(int relNum, AttrCatRec *attrPtr, void *valuePtr, Rid insertRid)
{
    if (!db_open) {
        db_err_code = DBNOTOPEN;
        return NOTOK;
    }

    if (!attrPtr->hasIndex) {
        db_err_code = IDXNOEXIST;
        return NOTOK;
    }

    return bptree_insert_key(attrPtr, relNum, valuePtr, insertRid);
}

int RemoveKeyfromBPTree(int relNum, AttrCatRec *attrPtr, Rid *removeRid)
{
    if (!db_open) {
        db_err_code = DBNOTOPEN;
        return NOTOK;
    }

    if (!attrPtr->hasIndex) {
        db_err_code = IDXNOEXIST;
        return NOTOK;
    }

    /* TODO: extend to supply key value; currently just a stub */
    void *dummyVal = NULL;
    return bptree_delete_key(attrPtr, relNum, dummyVal, removeRid);
}

int FindKeyWithBPTree(int relNum, AttrCatRec *attrPtr, Rid startRid,
                      Rid *foundRid, void *valuePtr, int cmpOp)
{
    if (!db_open) {
        db_err_code = DBNOTOPEN;
        return NOTOK;
    }

    if (!attrPtr->hasIndex) {
        db_err_code = IDXNOEXIST;
        return NOTOK;
    }

    return bptree_find_key(attrPtr, relNum, startRid, foundRid, valuePtr, cmpOp);
}

/* ============================================================
 * INTERNAL B+ TREE LOGIC (SEARCH / INSERT / DELETE)
 * ============================================================ */

/* Descend from root to leaf, returning leaf page id containing given key range. */
static int find_leaf(AttrCatRec *attrPtr, int relNum, void *valuePtr, short *leafPidOut)
{
    BPTMetaPage meta;
    if (read_meta_page(attrPtr, &meta) != OK)
        return NOTOK;

    if (meta.rootPid == 0) {
        db_err_code = IDXNOEXIST;  // empty tree
        return NOTOK;
    }

    short pid = meta.rootPid;

    while (1) {
        if (ReadBPTreePage(relNum, attrPtr, pid) != OK)
            return NOTOK;

        char *page = IDX_PAGE(relNum);
        char nodeType = get_node_type(page);

        if (nodeType == BPT_LEAF) break;

        /* internal node: find child to follow */
        int ksize = key_size_bytes(attrPtr);
        char *data = node_data(page);
        short numKeys = get_num_keys(page);

        /* child0 at start */
        short child = *(short *)data;

        /* For each key[i], if value >= key[i], move to child[i+1] */
        char *p = data + sizeof(short);  // points to key0
        for (int i = 0; i < numKeys; i++) {
            void *keyPtr = p + i * (ksize + sizeof(short));
            if (compareVals(valuePtr, keyPtr, attrPtr->type, ksize, CMP_GTE)) {
                child = *(short *)((char *)keyPtr + ksize);
            } else {
                break;
            }
        }
        pid = child;
    }

    *leafPidOut = pid;
    return OK;
}

/*
 * insertIntoPage:
 *   Try to insert recPtr into leaf page pidx of relation relNum.
 *
 *   Parameters:
 *     relNum       - relation number
 *     pidx         - page index (leaf)
 *     valuePtr     - pointer to key value
 *     insertRid    - Rid of record
 *     becameFull   - (out) true if page was not full but becomes full
 *     hasFreeAfter - (out) true if page has at least one free slot after insert
 *
 *   This function:
 *     - reads the page into idx_buffer[relNum].buffer.page
 *     - updates the leaf contents (key+Rid pairs)
 *     - updates relcat_rec.numRecs is NOT touched here (that's for data, not index)
 *
 *   It does NOT touch the freemap; the caller handles that if needed.
 */
static int insertIntoLeafPage(int relNum,
                              short pidx,
                              AttrCatRec *attrPtr,
                              void *valuePtr,
                              Rid insertRid,
                              bool *becameFull,
                              bool *hasFreeAfter)
{
    if (ReadBPTreePage(relNum, attrPtr, pidx) != OK)
        return NOTOK;

    char *page = IDX_PAGE(relNum);
    char nodeType = get_node_type(page);
    if (nodeType != BPT_LEAF) {
        db_err_code = UNKNOWN_ERROR;
        return NOTOK;
    }

    int ksize = key_size_bytes(attrPtr);
    if (ksize <= 0) {
        db_err_code = INVALID_FORMAT;
        return NOTOK;
    }

    short numKeys = get_num_keys(page);
    char *data = node_data(page);

    int entrySize = ksize + sizeof(Rid);
    int maxKeys = (PAGESIZE - NODE_HDR_SIZE) / entrySize;

    /* Compute full mask logically (for flags) */
    bool wasFull = (numKeys == maxKeys);

    /* Find insert position (sorted by key) */
    int pos = 0;
    for (; pos < numKeys; pos++) {
        char *keyPtr = data + pos * entrySize;
        if (compareVals(valuePtr, keyPtr, attrPtr->type, ksize, CMP_LT))
            break;
    }

    if (numKeys >= maxKeys) {
        if (becameFull)   *becameFull = true;
        if (hasFreeAfter) *hasFreeAfter = false;
        return INS_NO_FREE_SLOT;  /* need split; not yet implemented */
    }

    /* Simple insert without split: shift right and insert */
    memmove(data + (pos + 1) * entrySize,
            data + pos       * entrySize,
            (numKeys - pos) * entrySize);

    /* Copy key */
    memcpy(data + pos * entrySize, valuePtr, ksize);
    /* Copy Rid */
    memcpy(data + pos * entrySize + ksize, &insertRid, sizeof(Rid));

    set_num_keys(page, numKeys + 1);
    IDX_BUF(relNum).buffer.dirty = 1;

    bool nowFull = ((numKeys + 1) == maxKeys);

    if (becameFull)   *becameFull   = (!wasFull && nowFull);
    if (hasFreeAfter) *hasFreeAfter = (!nowFull);

    return OK;
}

/* Insert into leaf / handle split: currently only no-split path implemented. */
static int bptree_insert_key(AttrCatRec *attrPtr, int relNum,
                             void *valuePtr, Rid insertRid)
{
    BPTMetaPage meta;
    if (read_meta_page(attrPtr, &meta) != OK)
        return NOTOK;

    if (meta.rootPid == 0) {
        /* Should not happen if tree was created properly; rootPid >=1 */
        db_err_code = UNKNOWN_ERROR;
        return NOTOK;
    }

    short leafPid;
    if (find_leaf(attrPtr, relNum, valuePtr, &leafPid) != OK)
        return NOTOK;

    bool becameFull = false;
    bool hasFreeAfter = false;

    int rc = insertIntoLeafPage(relNum, leafPid, attrPtr,
                                valuePtr, insertRid,
                                &becameFull, &hasFreeAfter);

    if (rc == OK) {
        attrPtr->nKeys += 1;
        /* NOTE: caller should persist attrPtr changes to attrcat if needed. */
        return OK;
    }

    if (rc == INS_NO_FREE_SLOT) {
        /* TODO: implement leaf split + parent update
           - allocate new leaf page
           - redistribute keys
           - update nextLeafPid
           - insert separator into parent (possibly splitting parent)
           - update meta.rootPid and write meta
        */
        db_err_code = UNKNOWN_ERROR;
        return NOTOK;
    }

    return NOTOK;
}

/* Skeleton for search: currently unimplemented. */
static int bptree_find_key(AttrCatRec *attrPtr, int relNum,
                           Rid startRid, Rid *foundRid, void *valuePtr, int cmpOp)
{
    (void)attrPtr;
    (void)relNum;
    (void)startRid;
    (void)foundRid;
    (void)valuePtr;
    (void)cmpOp;

    /* TODO:
       - Implement leaf scanning and nextLeafPid following
       - Use compareVals with cmpOp to find the first matching key
         after startRid.
    */
    db_err_code = UNKNOWN_ERROR;
    return NOTOK;
}

/* Skeleton for delete: currently unimplemented. */
static int bptree_delete_key(AttrCatRec *attrPtr, int relNum,
                             void *valuePtr, Rid *removeRid)
{
    (void)attrPtr;
    (void)relNum;
    (void)valuePtr;
    (void)removeRid;

    /* TODO:
       - Descend to leaf that should contain key.
       - Locate entry matching key and/or RID and remove it.
       - Handle underflow with borrow/merge if needed.
       - Update meta/root if root shrinks.
    */
    db_err_code = UNKNOWN_ERROR;
    return NOTOK;
}

/* ============================================================
 * DEBUG DUMPER
 * ============================================================ */

static void print_indent(int level)
{
    for (int i = 0; i < level; i++)
        printf("  ");
}

static void format_key(char *buf, size_t bufsz, void *keyPtr, AttrCatRec *attrPtr)
{
    if (attrPtr->type == 'i') {
        int v;
        memcpy(&v, keyPtr, sizeof(int));
        snprintf(buf, bufsz, "%d", v);
    } else if (attrPtr->type == 'f') {
        float v;
        memcpy(&v, keyPtr, sizeof(float));
        snprintf(buf, bufsz, "%g", v);
    } else if (attrPtr->type == 's') {
        int len = attrPtr->length;
        char tmp[64];
        int copyLen = len < (int)sizeof(tmp) - 1 ? len : (int)sizeof(tmp) - 1;
        memcpy(tmp, keyPtr, copyLen);
        tmp[copyLen] = '\0';
        /* Strip trailing nulls/spaces for readability */
        int end = copyLen - 1;
        while (end >= 0 && (tmp[end] == '\0' || tmp[end] == ' '))
            tmp[end--] = '\0';
        snprintf(buf, bufsz, "\"%s\"", tmp);
    } else {
        snprintf(buf, bufsz, "?");
    }
}

static int dump_node_recursive(int relNum, AttrCatRec *attrPtr,
                               short pid, int level)
{
    if (ReadBPTreePage(relNum, attrPtr, pid) != OK) {
        printf("Error: ReadBPTreePage failed for pid=%d, err=%d\n", pid, db_err_code);
        return NOTOK;
    }

    char *page = IDX_PAGE(relNum);

    /* Sanity check magic */
    if (strncmp(page, BPT_NODE_MAGIC, strlen(BPT_NODE_MAGIC)) != 0) {
        print_indent(level);
        printf("pid=%d: INVALID NODE MAGIC\n", pid);
        return NOTOK;
    }

    char nodeType = get_node_type(page);
    short numKeys = get_num_keys(page);
    short parent  = get_parent_pid(page);

    int ksize = key_size_bytes(attrPtr);
    if (ksize <= 0) {
        print_indent(level);
        printf("pid=%d: invalid key size\n", pid);
        return NOTOK;
    }

    char *data = node_data(page);

    if (nodeType == BPT_LEAF) {
        short nextLeaf = get_next_leaf(page);
        print_indent(level);
        printf("Leaf(pid=%d, parent=%d, next=%d, numKeys=%d)\n",
               pid, parent, nextLeaf, numKeys);

        int entrySize = ksize + sizeof(Rid);
        for (int i = 0; i < numKeys; i++) {
            char *entryPtr = data + i * entrySize;
            char keyBuf[128];
            Rid rid;

            format_key(keyBuf, sizeof(keyBuf), entryPtr, attrPtr);
            memcpy(&rid, entryPtr + ksize, sizeof(Rid));

            print_indent(level + 1);
            printf("[%d] key=%s -> rid=(%d,%d)\n",
                   i, keyBuf, rid.pid, rid.slotnum);
        }
    } else if (nodeType == BPT_INTERNAL) {
        print_indent(level);
        printf("Internal(pid=%d, parent=%d, numKeys=%d)\n",
               pid, parent, numKeys);

        /* child0 at start */
        short *child0Ptr = (short *)data;
        short child0 = *child0Ptr;

        /* Print keys list */
        print_indent(level + 1);
        printf("Keys: [");
        char *p = data + sizeof(short); /* skip child0 */
        for (int i = 0; i < numKeys; i++) {
            char keyBuf[128];
            void *keyPtr = p + i * (ksize + sizeof(short));
            format_key(keyBuf, sizeof(keyBuf), keyPtr, attrPtr);
            printf("%s%s", (i == 0 ? "" : ", "), keyBuf);
        }
        printf("]\n");

        /* Recurse on children: child0, then each key's right child */
        print_indent(level + 1);
        printf("Child 0 (pid=%d):\n", child0);
        dump_node_recursive(relNum, attrPtr, child0, level + 2);

        for (int i = 0; i < numKeys; i++) {
            void *keyPtr = p + i * (ksize + sizeof(short));
            short child = *(short *)((char *)keyPtr + ksize);

            char keyBuf[128];
            format_key(keyBuf, sizeof(keyBuf), keyPtr, attrPtr);

            print_indent(level + 1);
            printf("Child %d (pid=%d) [>= %s]:\n", i + 1, child, keyBuf);
            dump_node_recursive(relNum, attrPtr, child, level + 2);
        }
    } else {
        print_indent(level);
        printf("pid=%d: UNKNOWN nodeType=%c\n", pid, nodeType);
        return NOTOK;
    }

    return OK;
}

int DumpBPTree(int relNum, AttrCatRec *attrPtr)
{
    if (!db_open) {
        db_err_code = DBNOTOPEN;
        return NOTOK;
    }

    if (!attrPtr->hasIndex) {
        db_err_code = IDXNOEXIST;
        return NOTOK;
    }

    BPTMetaPage meta;
    if (read_meta_page(attrPtr, &meta) != OK) {
        printf("DumpBPTree: failed to read meta page, err=%d\n", db_err_code);
        return NOTOK;
    }

    printf("===== B+ TREE DUMP for %s.%s =====\n",
           attrPtr->relName, attrPtr->attrName);
    printf("rootPid=%d, keyLen=%d, keyType=%c, nPages=%d, nKeys=%d\n",
           meta.rootPid, meta.keyLength, meta.keyType,
           attrPtr->nPages, attrPtr->nKeys);

    if (meta.rootPid == 0) {
        printf("(empty tree)\n");
        printf("===== END B+ TREE DUMP =====\n");
        return OK;
    }

    int rc = dump_node_recursive(relNum, attrPtr, meta.rootPid, 0);
    printf("===== END B+ TREE DUMP =====\n");
    return rc;
}