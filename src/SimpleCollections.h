
/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _SIMPLE_COLLECTIONS_H
#define _SIMPLE_COLLECTIONS_H

#include <string.h>
#include <inttypes.h>
#include "IoLogging.h"

/**
 * @file SimpleCollections.h memory efficient collections for use on all embedded devices.
 */


enum GrowByMode : unsigned char {
    GROW_NEVER,
    GROW_BY_5,
    GROW_BY_DOUBLE
};

#ifdef __AVR_ATmega2560__
# define DEFAULT_LIST_SIZE 10
# define DEFAULT_GROW_MODE GROW_BY_5
typedef uint8_t bsize_t;
#elif defined(__AVR__)
# define DEFAULT_LIST_SIZE 5
# define DEFAULT_GROW_MODE GROW_BY_5
typedef uint8_t bsize_t;
#else
# define DEFAULT_LIST_SIZE 10
# define DEFAULT_GROW_MODE GROW_BY_DOUBLE
typedef size_t bsize_t;
#endif // platform

/**
 * A very simple binary search based list. It is useful for storage of items by key or just regular storage,
 * it is very efficient for reading, but relatively inefficient for insertions, but only when compared to
 * a linked list. It is however very memory efficient. There are some restrictions on it's use:
 * * Class V must have a method to get the key: K getKey()
 * * Class K must be directly comparable using less than, greater than and equals (any primitive would generally work).
 *
 * On inserts, the list is re-sorted to get it back into order by key, and will grow automatically if needed using the
 * howToGrow parameter passed to the constructor. The list is therefore always sorted by key, and this will generally
 * always involve a memory copy to move around the items.
 *
 * On using the associative indexer operator, you look up an item by it's key value, the list will either return NULL
 * or a valid item. You can also obtain all values too, which is always sorted by key and is marked const, do not alter
 * this array.
 */
template<class K, class V> class BtreeList {
private:
    V* binTree;
    bsize_t currentSize;
    bsize_t itemsInList;
    GrowByMode howToGrow;
public:
    BtreeList(bsize_t size = DEFAULT_LIST_SIZE, GrowByMode howToGrow = DEFAULT_GROW_MODE) {
        this->binTree = new V[size];
        this->howToGrow = howToGrow;
        this->currentSize = binTree != NULL ? size : 0;
        this->itemsInList = 0;
    }

    ~BtreeList() {
        delete[] binTree;
    }

    /**
     * Adds an item into the current list of items, the list will be sorted by using getKey()
     * on the object passed in. If the list cannot fit the item and cannot resize to do so, then
     * false is returned.
     * @param item the item to add
     * @return true if added, otherwise false
     */
    bool add(V item) {
        // after check capacity returns true, we always have space
        if(!checkCapacity()) return false;

        bsize_t insertionPoint = nearestLocation(item.getKey());
        int amtToMove = itemsInList - insertionPoint;
        serdebugF4("add ", insertionPoint, item.getKey(), amtToMove);

        if(amtToMove > 0) {
            memmove(&binTree[insertionPoint + 1], &binTree[insertionPoint], amtToMove * sizeof(V));
        }
        binTree[insertionPoint] = item;
        itemsInList++;
        return true;
    }

    /**
     * Check the capacity of the list, increasing the size if needed. This is normally called before an add operation
     * to ensure there will be space to add another item.
     * @return
     */
    bool checkCapacity() {
        // a couple of short circuits first
        if(itemsInList < currentSize) return true;
        if(howToGrow == GROW_NEVER) return false;

        // now determine the new size and try and group the list.
        int newSize = currentSize + ((howToGrow == GROW_BY_5) ? 5 : currentSize);
        auto replacement = new V[newSize];
        if(replacement == NULL) return false;

        // now copy over and replace the current tree.
        memcpy(replacement, binTree, sizeof(V) * itemsInList);
        delete[] binTree;
        binTree = replacement;
        currentSize = newSize;
        return true;
    }

    /**
     * Get a value by it's key using a binary search algorithm.
     * @param key the key to be looked up
     * @return the value at that key position or null.
     */
    V* getByKey(K key) {
        if(itemsInList == 0) return NULL;
        bsize_t loc = nearestLocation(key);
        //serdebugF3("getByKey ", loc, key);
        return (binTree[loc].getKey() == key && loc < itemsInList) ? &binTree[loc] : NULL;
    };

    /**
     * gets the nearest location to the key, this is an in-exact method in that it gives the exact match if available
     * or otherwise the nearest one that is lower.
     * @param key the key to lookup
     * @return the position in the list
     */
    bsize_t nearestLocation(K key) {
        // a few short circuits, basically handling quickly nothing in list,
        // one item in the list and an insertion at the end of the list.
        if(itemsInList == 0) return 0; // always first item in this case
        else if(itemsInList == 1) return (key <= binTree[0].getKey()) ? 0 : 1;
        else if(key > binTree[itemsInList - 1].getKey()) return itemsInList;

        // otherwise we search with binary chop
        bsize_t start = 0;
        bsize_t end = itemsInList - 1;
        bsize_t midPoint;
        while((end - start) > 1) {
            midPoint = start + ((end - start) / 2);
            //serdebugF4("start mid end", start, midPoint, end);
            auto midKey = binTree[midPoint].getKey();
            if(midKey == key) return midPoint;
            else if(midKey > key) end = midPoint;
            else if(midKey < key) start = midPoint;
        }

        // when we get here we've got down to two entries and need to
        // either return the exact match, or locate the lower of the two
        // in the case there no exact match.

        // check if start or end contain the key
        if(binTree[start].getKey() == key) return start;
        if(binTree[end].getKey() == key) return end;

        // in this case we return the first item LOWER than the current one
        while(end > 0 && binTree[end - 1].getKey() > key) --end;
        return end;
    }

    /**
     * @return a list of all items
     */
    const V* items() {
        return binTree;
    };

    /**
     * gets an item by it's index
     * @param idx the index to find
     * @return the item at the index or null.
     */
    V* itemAtIndex(bsize_t idx) {
        return (idx < itemsInList)  ? &binTree[idx] : NULL;
    }

    /**
     * @return number of items in the list
     */
    bsize_t count() {
        return itemsInList;
    }

    /**
     * @return current capacity of the list
     */
    bsize_t capacity() {
        return currentSize;
    }

    void clear() {
        itemsInList = 0;
    }
};

#endif // _SIMPLE_COLLECTIONS_H
