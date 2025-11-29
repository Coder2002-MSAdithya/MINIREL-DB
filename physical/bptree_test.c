#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/bptree.h"

/* ---- stub for Create to satisfy helpers.o linkage ---- */
/* We never call it in this test; it just avoids undefined reference. */
int Create(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return OK;
}

/* Helper: initialize a fake relation entry in catcache */
static void setup_rel(int relNum, const char *relName)
{
    memset(&catcache[relNum], 0, sizeof(CacheEntry));
    strcpy(catcache[relNum].relcat_rec.relName, relName);
    catcache[relNum].relcat_rec.numRecs = 0;    // empty table
    catcache[relNum].status = VALID_MASK;       // mark as valid
}

/* Helper: initialize an AttrCatRec for a given type */
static void setup_attr(AttrCatRec *attr,
                       const char *relName,
                       const char *attrName,
                       char type,
                       int length)
{
    memset(attr, 0, sizeof(AttrCatRec));
    strcpy(attr->relName, relName);
    strcpy(attr->attrName, attrName);
    attr->type   = type;
    attr->length = length;
    attr->hasIndex = 0;
    attr->nPages = 0;
    attr->nKeys  = 0;
}

/* ---------- INT TEST ---------- */

static void test_int_bptree()
{
    printf("\n=== INT B+ TREE TEST ===\n");

    int relNum = 2;
    setup_rel(relNum, "emp");

    AttrCatRec attr;
    setup_attr(&attr, "emp", "id", 'i', sizeof(int));

    if (CreateBPTree(relNum, &attr) != OK) {
        printf("CreateBPTree(emp.id) failed, db_err_code=%d\n", db_err_code);
        return;
    }

    printf("Created index file emp.id.bpidx\n");

    /* Insert some unsorted keys */
    int keys[] = {10, 5, 20, 15, 7};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        int k = keys[i];
        Rid r = { .pid = 0, .slotnum = i };  // dummy RIDs
        if (AddKeytoBPTree(relNum, &attr, &k, r) != OK) {
            printf("AddKeytoBPTree(emp.id, %d) failed, db_err_code=%d\n",
                   k, db_err_code);
            return;
        }
    }

    printf("Inserted %d integer keys into emp.id index\n", n);

    DumpBPTree(relNum, &attr);
}

/* ---------- FLOAT TEST ---------- */

static void test_float_bptree()
{
    printf("\n=== FLOAT B+ TREE TEST ===\n");

    int relNum = 3;
    setup_rel(relNum, "temp");

    AttrCatRec attr;
    setup_attr(&attr, "temp", "val", 'f', sizeof(float));

    if (CreateBPTree(relNum, &attr) != OK) {
        printf("CreateBPTree(temp.val) failed, db_err_code=%d\n", db_err_code);
        return;
    }

    printf("Created index file temp.val.bpidx\n");

    float keys[] = {1.5f, -0.3f, 2.7f, 0.0f, 1.5f};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        float k = keys[i];
        Rid r = { .pid = 1, .slotnum = i };
        if (AddKeytoBPTree(relNum, &attr, &k, r) != OK) {
            printf("AddKeytoBPTree(temp.val, %g) failed, db_err_code=%d\n",
                   (double)k, db_err_code);
            return;
        }
    }

    printf("Inserted %d float keys into temp.val index\n", n);

    DumpBPTree(relNum, &attr);
}

/* ---------- STRING TEST ---------- */

static void test_string_bptree()
{
    printf("\n=== STRING B+ TREE TEST ===\n");

    int relNum = 4;
    setup_rel(relNum, "names");

    /* Let’s use fixed length 10 bytes for strings */
    AttrCatRec attr;
    setup_attr(&attr, "names", "name", 's', 10);

    if (CreateBPTree(relNum, &attr) != OK) {
        printf("CreateBPTree(names.name) failed, db_err_code=%d\n", db_err_code);
        return;
    }

    printf("Created index file names.name.bpidx\n");

    const char *keys[] = {"bob", "alice", "charlie", "dave", "amy"};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        char keybuf[10];
        memset(keybuf, 0, sizeof(keybuf));
        strncpy(keybuf, keys[i], 10);  // attr.length == 10

        Rid r = { .pid = 2, .slotnum = i };
        if (AddKeytoBPTree(relNum, &attr, keybuf, r) != OK) {
            printf("AddKeytoBPTree(names.name, \"%s\") failed, db_err_code=%d\n",
                   keys[i], db_err_code);
            return;
        }
    }

    printf("Inserted %d string keys into names.name index\n", n);

    DumpBPTree(relNum, &attr);
}

int main(void)
{
    /* Simulate an open DB */
    db_open = true;

    /* Initialize globals we care about */
    memset(catcache, 0, sizeof(catcache));
    memset(buffer, 0, sizeof(buffer));
    memset(idx_buffer, 0, sizeof(idx_buffer));

    /* Run three independent tests */
    test_int_bptree();
    test_float_bptree();
    test_string_bptree();

    return 0;
}
