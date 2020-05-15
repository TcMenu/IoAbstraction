#ifndef IOABSTRACTION_SIMPLECOLLECTIONSTEST_H
#define IOABSTRACTION_SIMPLECOLLECTIONSTEST_H

#include <SimpleCollections.h>
#include <unity.h>

class TestStorage {
private:
    int testItem;
    int testKey;
public:
    TestStorage() {
        testItem = testKey = 0;
    }

    TestStorage(int key, int item) {
        this->testKey = key;
        this->testItem = item;
    }

    TestStorage(const TestStorage& other) {
        this->testItem = other.testItem;
        this->testKey = other.testKey;
    }

    int getKey() const {
        return testKey;
    }

    int getItem() const {
        return testItem;
    }
};

TestStorage storage1(1, 100);
TestStorage storage2(2, 101);
TestStorage storage3(3, 102);
TestStorage storage4(4, 103);
TestStorage storage5(5, 104);
TestStorage storage6(6, 105);

void testNearestLocationEdgeCases() {
    BtreeList<int, TestStorage> btreeList(5, GROW_NEVER);
    serdebugF("1");
    TEST_ASSERT_EQUAL(0, btreeList.nearestLocation(1));
    serdebugF("2");

    TEST_ASSERT_TRUE(btreeList.add(storage4));
    serdebugF("3");

    TEST_ASSERT_EQUAL(0, btreeList.nearestLocation(1));
    serdebugF("4");
    TEST_ASSERT_EQUAL(1, btreeList.nearestLocation(5));
}

void testAddingWithoutSortOrResize() {
    BtreeList<int, TestStorage> btreeList(5, GROW_NEVER);

    TEST_ASSERT_TRUE(btreeList.add(storage1));
    TEST_ASSERT_TRUE(btreeList.add(storage2));
    TEST_ASSERT_TRUE(btreeList.add(storage3));
    TEST_ASSERT_TRUE(btreeList.add(storage4));
    TEST_ASSERT_TRUE(btreeList.add(storage5));
    TEST_ASSERT_FALSE(btreeList.add(storage6));

    TEST_ASSERT_NOT_NULL(btreeList.getByKey(1));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(2));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(3));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(4));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(5));
    TEST_ASSERT_NULL(btreeList.getByKey(6));

    TEST_ASSERT_EQUAL(btreeList.getByKey(1)->getItem(), 100);
    TEST_ASSERT_EQUAL(btreeList.getByKey(2)->getItem(), 101);
    TEST_ASSERT_EQUAL(btreeList.getByKey(3)->getItem(), 102);
    TEST_ASSERT_EQUAL(btreeList.getByKey(4)->getItem(), 103);
    TEST_ASSERT_EQUAL(btreeList.getByKey(5)->getItem(), 104);
}

void testAddingWithSortNoResize() {
    BtreeList<int, TestStorage> btreeList(5, GROW_NEVER);

    TEST_ASSERT_TRUE(btreeList.add(storage2));
    TEST_ASSERT_TRUE(btreeList.add(storage1));
    TEST_ASSERT_TRUE(btreeList.add(storage5));
    TEST_ASSERT_TRUE(btreeList.add(storage4));
    TEST_ASSERT_TRUE(btreeList.add(storage3));
    TEST_ASSERT_FALSE(btreeList.add(storage6));

    TEST_ASSERT_NOT_NULL(btreeList.getByKey(1));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(2));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(3));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(4));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(5));
    TEST_ASSERT_NULL(btreeList.getByKey(6));

    const TestStorage* allItems = btreeList.items();
    TEST_ASSERT_EQUAL(allItems[0].getKey(), 1);
    TEST_ASSERT_EQUAL(allItems[1].getKey(), 2);
    TEST_ASSERT_EQUAL(allItems[2].getKey(), 3);
    TEST_ASSERT_EQUAL(allItems[3].getKey(), 4);
    TEST_ASSERT_EQUAL(allItems[4].getKey(), 5);

    TEST_ASSERT_EQUAL(btreeList.getByKey(1)->getItem(), 100);
    TEST_ASSERT_EQUAL(btreeList.getByKey(2)->getItem(), 101);
    TEST_ASSERT_EQUAL(btreeList.getByKey(3)->getItem(), 102);
    TEST_ASSERT_EQUAL(btreeList.getByKey(4)->getItem(), 103);
    TEST_ASSERT_EQUAL(btreeList.getByKey(5)->getItem(), 104);
}

void testAddingWithSortAndResizeBy5() {
    BtreeList<int, TestStorage> btreeList(5, GROW_BY_5);

    TEST_ASSERT_TRUE(btreeList.add(storage2));
    TEST_ASSERT_TRUE(btreeList.add(storage1));
    TEST_ASSERT_TRUE(btreeList.add(storage5));
    TEST_ASSERT_TRUE(btreeList.add(storage4));
    TEST_ASSERT_TRUE(btreeList.add(storage3));

    TEST_ASSERT_EQUAL(5, btreeList.capacity());
    TEST_ASSERT_TRUE(btreeList.add(storage6));
    TEST_ASSERT_EQUAL(10, btreeList.capacity());

    TEST_ASSERT_NOT_NULL(btreeList.getByKey(1));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(2));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(3));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(4));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(5));
    TEST_ASSERT_NOT_NULL(btreeList.getByKey(6));

    const TestStorage* allItems = btreeList.items();
    TEST_ASSERT_EQUAL(allItems[0].getKey(), 1);
    TEST_ASSERT_EQUAL(allItems[1].getKey(), 2);
    TEST_ASSERT_EQUAL(allItems[2].getKey(), 3);
    TEST_ASSERT_EQUAL(allItems[3].getKey(), 4);
    TEST_ASSERT_EQUAL(allItems[4].getKey(), 5);
    TEST_ASSERT_EQUAL(allItems[5].getKey(), 6);

    TEST_ASSERT_EQUAL(btreeList.getByKey(1)->getItem(), 100);
    TEST_ASSERT_EQUAL(btreeList.getByKey(2)->getItem(), 101);
    TEST_ASSERT_EQUAL(btreeList.getByKey(3)->getItem(), 102);
    TEST_ASSERT_EQUAL(btreeList.getByKey(4)->getItem(), 103);
    TEST_ASSERT_EQUAL(btreeList.getByKey(5)->getItem(), 104);
    TEST_ASSERT_EQUAL(btreeList.getByKey(6)->getItem(), 105);
}

#endif //IOABSTRACTION_SIMPLECOLLECTIONSTEST_H
