
/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef SIMPLE_COLLECTIONS_H
#define SIMPLE_COLLECTIONS_H

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

namespace ioaTreeInternal {

    typedef uint32_t (*KeyAccessor)(const void *item);

    typedef void (*CopyOperator)(void *dest, const void *src);

    /**
     * This is an internal btree storage engine that used by all the templates to actually store the data (and
     * hopefully save a lot of FLASH). It implements the costly features of the btree in a one off way. However,
     * as a result the keys must now be unsigned integer, and cannot exceed uint32_t. If you need more than this
     * use a std container.
     */
    class BtreeStorage {
    private:
        void *binTree;
        void *swapperSpace;
        KeyAccessor keyAccessor;
        CopyOperator copier;
        bsize_t currentCapacity;
        bsize_t currentSize;
        bsize_t sizeOfAnItem;
        GrowByMode growByMode;
    public:
        BtreeStorage(bsize_t size, GrowByMode howToGrow, bsize_t eachItemSize, KeyAccessor compareOperator,
                     CopyOperator copyOperator);

        ~BtreeStorage();

        bool add(const void *newItem);

        bsize_t nearestLocation(uint32_t key);

        void *getByKey(uint32_t key);

        void clear() { currentSize = 0; }

        void *underlyingData() { return binTree; }

        bsize_t getCapacity() const { return currentCapacity; }

        bsize_t getCurrentSize() const { return currentSize; }

    private:
        bool checkCapacity();
        void *memoryOf(void *baseMem, bsize_t item) const { return ((uint8_t *) baseMem) + (item * sizeOfAnItem); }
    };
}
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
 * All keys must be of an unsigned integer type not exceeding uint32_t in length, you look up an item by it's key value,
 * the list will either return NULL or a valid item. You can also obtain all values too, which is always sorted by key
 * and is marked const, do not alter this array.
 */
template<class K, class V> class BtreeList {
private:
    ioaTreeInternal::BtreeStorage treeStorage;
public:
    explicit BtreeList(bsize_t size = DEFAULT_LIST_SIZE, GrowByMode howToGrow = DEFAULT_GROW_MODE)
        : treeStorage(size, howToGrow, sizeof(V), keyAccessor, copyInternal) { }

    static uint32_t keyAccessor(const void* itm) {
        return reinterpret_cast<const V*>(itm)->getKey();
    }

    static void copyInternal(void* dest, const void* src) {
        auto *itemDest = reinterpret_cast<V*>(dest);
        auto *itemSrc = reinterpret_cast<const V*>(src);
        *itemDest = *itemSrc;
    }

    /**
     * Adds an item into the current list of items, the list will be sorted by using getKey()
     * on the object passed in. If the list cannot fit the item and cannot resize to do so, then
     * false is returned.
     * @param item the item to add
     * @return true if added, otherwise false
     */
    bool add(const V& item) { return treeStorage.add(&item); }

    /**
     * Get a value by it's key using a binary search algorithm.
     * @param key the key to be looked up
     * @return the value at that key position or null.
     */
    V* getByKey(K key) { return reinterpret_cast<V*>(treeStorage.getByKey(key)); };

    /**
     * gets the nearest location to the key, this is an in-exact method in that it gives the exact match if available
     * or otherwise the nearest one that is lower.
     * @param key the key to lookup
     * @return the position in the list
     */
    bsize_t nearestLocation(const K& key) { return treeStorage.nearestLocation(key); }

    /**
     * @return a list of all items
     */
    const V* items() { return reinterpret_cast<V*>(treeStorage.underlyingData()); };

    /**
     * gets an item by it's index
     * @param idx the index to find
     * @return the item at the index or null.
     */
    V* itemAtIndex(bsize_t idx) {
        auto* binTree = reinterpret_cast<V*>(treeStorage.underlyingData());
        return (idx < treeStorage.getCurrentSize())  ? &binTree[idx] : NULL;
    }

    /**
     * @return number of items in the list
     */
    bsize_t count() {
        return treeStorage.getCurrentSize();
    }

    /**
     * @return current capacity of the list
     */
    bsize_t capacity() { return treeStorage.getCapacity(); }

    void clear() { treeStorage.clear(); }
};

#endif // _SIMPLE_COLLECTIONS_H
