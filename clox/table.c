#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75 // count / capacity

void initTable(Table *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

// Use linear probing to find bucket for 'key'.
// The returned bucket can be in 3 cases:
// - the 1st one containing tombstone on the probing sequence.
// - empty if key isn't present (and there're no tombstones on the probing sequence).
// - has existing value if key is present.
static Entry *findEntry(Entry *entries, int capacity,
                        ObjString *key)
{
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;

    for (;;)
    {
        Entry *entry = &entries[index];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value)) // reached empty entry -> key isn't present
            {
                // return the 1st tombstone or the empty entry
                // (tombstones can be reused to insert new entries)
                return tombstone != NULL ? tombstone : entry;
            }
            else // found a tombstone
            {
                if (tombstone == NULL) // note 1st tombstone
                    tombstone = entry;
            }
        }
        else if (entry->key == key)
        {
            // Use (entry->key == key) since 'string interning' is used:
            // - each sequence of characters is represented by only 1 string object.
            //   -> pointer equality exactly matches value equality.
            return entry;
        }

        // collision -> linear probing (can wrap around)
        index = (index + 1) % capacity;
    }

    /*
    - We grow the array before it is full (load_factor = 0.75),
      so there will always be empty buckets.

    Problem:
    - If we treat tombstones like empty buckets
      -> the array is not resized when it's full of tombstones.
      -> no actual empty buckets to terminate lookup.

    Solution:
    - Treat tombstones like full buckets:
      . Don't reduce count when deleting entries.
      . Increment count during insertion only if new entry goes into empty bucket.
    */
}

// Find entry with key; Return True if found;
// If entry exists, 'value' points to resulting value
bool tableGet(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return false;

    *value = entry->value; // copy value to ouput parameter
    return true;
}

static void adjustCapacity(Table *table, int capacity)
{
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // When array size changes:
    // - entries are redistributed (copied to new array, at different indices).
    // - don't copy the tombstones, since probe sequences are rebuilt anyway.
    //   -> need to recalculate the count
    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL)
            continue;

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

// Add key-value pair to hash table;
// Overwrite value if entry is already present;
// Return True if a new entry was added
bool tableSet(Table *table, ObjString *key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;

    // Only increase count if new entry goes into empty bucket, not tombstone
    // (see why in 'findEntry()')
    if (isNewKey && IS_NIL(entry->value))
        table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

// Find entry with key;
// If found, place a tombstone and return True
bool tableDelete(Table *table, ObjString *key)
{
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return false;

    // Place a tombstone in the entry
    // Don't reduce count (see why in 'findEntry()')
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;

    /* Why do we need tombstone:
    - To avoid breaking (implicit) collision chain.
    - Example:
      . 3 keys A, B, C all have the same preferred bucket 2
      . with linear probing, they end up in bucket 2, 3, 4
      . assume that we delete B just by clearing the entry (bucket 3).
      . when we later try to find C, we start bucket 2,
        and stop at bucket 3 (empty entry) -> not found.
    */
}

// Copy all entries from 1 hash table to another
void tableAddAll(Table *from, Table *to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL)
        {
            tableSet(to, entry->key, entry->value);
        }
    }
}

// Look for a string key in the table
ObjString *tableFindString(Table *table, const char *chars,
                           int length, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash % table->capacity;
    for (;;)
    {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL && IS_NIL(entry->value))
        {
            // Stop if reached an empty (non-tombstone) entry
            return NULL; // not found
        }
        else if (entry->key->length == length &&
                 entry->key->hash == hash &&
                 memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key; // found
        }

        index = (index + 1) % table->capacity;
    }
}

void tableRemoveWhite(Table *table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked)
        {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(Table *table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];
        markObject((Obj *)entry->key);
        markValue(entry->value);
    }
}
