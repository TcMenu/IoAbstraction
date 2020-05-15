
/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _SIMPLE_COLLECTIONS_H
#define _SIMPLE_COLLECTIONS_H

#include <string.h>
#include <inttypes.h>

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
        this->currentSize = 0;
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
        if(amtToMove > 0) {
            memmove(&binTree[insertionPoint + 1], &binTree[insertionPoint], amtToMove * sizeof(V));
        }
        binTree[insertionPoint] = item;
        itemsInList++;
        return true;
    }

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

    V* getByKey(K key) {
        if(itemsInList == 0) return NULL;
        bsize_t loc = nearestLocation(key);
        return (binTree[loc].getKey() == key) ? &binTree[loc] : NULL;
    };

    bsize_t nearestLocation(K key) {
        // a few short circuits
        if(itemsInList == 0) return 0; // always first item in this case
        else if(itemsInList == 1) return (key <= binTree[1].getKey()) ? 0 : 1;

        bsize_t start = 0;
        bsize_t end = itemsInList - 1;
        bsize_t midPoint;
        bool resolved = false;
        while(!resolved) {
            midPoint = (end - start) / 2;
            resolved = ((end - start) < 2);
            auto midKey = binTree[midPoint].getKey();
            if(midKey == key) return midPoint;
            else if(midKey > key) end = midPoint;
            else if(midKey < key) start = midPoint;
        }

        serdebugF4("start mid end", start, midPoint, end);

        // in this case we return the first item LOWER than the current one
        while(midPoint > 0 && binTree[midPoint].getKey() > key) --midPoint;
        return midPoint;
    }

    const V* items() {
        return binTree;
    };

    bsize_t count() {
        return itemsInList;
    }

    bsize_t capacity() {
        return currentSize;
    }
};

#endif // _SIMPLE_COLLECTIONS_H
