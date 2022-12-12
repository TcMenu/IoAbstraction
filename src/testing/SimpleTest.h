#ifndef IOA_SIMPLE_TEST_H
#define IOA_SIMPLE_TEST_H

/**
 * @file SimpleTest.h - a very simple test framework for all platforms that IoAbstraction supports using testmanager as
 * the executor queue. This is enough of a test framework for me to test my libraries, it may well be enough for you
 * as well, but it is far from complete. Unfortunately, aunit just does not work on the boards I need to test on, I've
 * excused it for a long time, but now it's more than half and will get worse as more boards move to the API. This
 * replaces enough to just about do the job for our stuff.
 */

#include <TaskManagerIO.h>
#include <IoAbstraction.h>
#include <IoLogging.h>
#include <SimpleCollections.h>

#define FAIL_REASON_SIZE 32

#ifdef __AVR__
#include <avr/pgmspace.h>
#define FromPgm(x) F(x)
#define PGM_TYPE const __FlashStringHelper*
#else
#define FromPgm(x) (x)
#define PGM_TYPE const char*
#endif

namespace SimpleTest {

    enum TestStatus {
        NOT_RUN, RUNNING, FAILED, PASSED, IGNORED
    };

    const char* niceStatus(TestStatus s);

    class FailureInfo {
    private:
        PGM_TYPE file;
        int line = 0;
    public:
        FailureInfo() {}
        void withFileAndLine(PGM_TYPE f, int l) {
            file = f;
            line = l;
        }
    };

    class UnitTestExecutor {
    private:
        static UnitTestExecutor *currentlyRunning;
        TestStatus testStatus = NOT_RUN;
        FailureInfo failureReason;
        const char* testName;
    public:
        void init(const char* name, bool ignored = false);

        virtual void performTest() = 0;

        TestStatus getTestStatus() const { return testStatus; }
        const char* getTestName() const { return testName; }

        void exec();
        void setFailed(PGM_TYPE file, int line, const char* reason);

        virtual void setup() {}
        virtual void teardown() {}

        static UnitTestExecutor* getCurrentTest() {
            return currentlyRunning;
        }
    };

    class ExecutionWithId {
    private:
        uint16_t key;
        UnitTestExecutor* executor;
    public:
        ExecutionWithId(): key(-1), executor(nullptr) {}
        ExecutionWithId(const ExecutionWithId& other) = default;
        ExecutionWithId& operator=(const ExecutionWithId& other) = default;
        ExecutionWithId(uint16_t key, UnitTestExecutor* exec): key(key), executor(exec) {}

        uint16_t getKey() const { return key; }
        UnitTestExecutor* getTest() { return executor; }
    };

    /**
     * Implement this predicate to be able to filter out tests based on the name or any other parameter.
     * You are given a reference to the test and can return true = run test, false = dont run.
     */
    typedef bool (*TestFilterPredicate)(UnitTestExecutor* theTest);

    class TestManager {
    private:
        BtreeList<uint8_t, ExecutionWithId> testsRecorded;
        static TestManager* instance;
        int currentIndex = 0;
        bool needsSummary = true;
        TestFilterPredicate filterPredicate = nullptr;

        TestManager(): testsRecorded(32) {}

    public:
        static TestManager* getInstance();

        void begin();

        void runLoop();

        void addTest(UnitTestExecutor* t) {
            testsRecorded.add(ExecutionWithId(testsRecorded.count(), t));
        }

        void printSummary();

        void setTestFilterPredicate(TestFilterPredicate predicate) {
            filterPredicate = predicate;
        }
    };

    inline void startTesting() {
        TestManager::getInstance()->begin();
    }
}

namespace STestInternal {

    void internalEquality(PGM_TYPE file, int line, bool success, uint32_t x, uint32_t y, const char *how);

    void assertStringInternal(PGM_TYPE file, int line, const char *x, const char *y);

    void failInternal(PGM_TYPE file, int line, const char *reason);

    void assertBoolInternal(PGM_TYPE file, int line, bool valid, const char *reason);

    void assertFloatInternal(PGM_TYPE file, int line, float x, float y, float allowable);

    inline void assertEqualityInternal(PGM_TYPE file, int line, short x, short y) {
        internalEquality(file, line, x == y, x, y, "==");
    }
    inline void assertEqualityInternal(PGM_TYPE file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, x == y, x, y, "==");
    }

    inline void assertEqualityInternal(PGM_TYPE file, int line, int x, int y) {
        internalEquality(file, line, x == y, x, y, "==");
    }
    inline void assertEqualityInternal(PGM_TYPE file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, x == y, x, y, "==");
    }

    inline void assertEqualityInternal(PGM_TYPE file, int line, long x, long y) {
        internalEquality(file, line, x == y, x, y, "==");
    }
    inline void assertEqualityInternal(PGM_TYPE file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, x == y, x, y, "==");
    }

    inline void assertNonEqualityInternal(PGM_TYPE file, int line, short x, short y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }
    inline void assertNonEqualityInternal(PGM_TYPE file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }

    inline void assertNonEqualityInternal(PGM_TYPE file, int line, int x, int y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }
    inline void assertNonEqualityInternal(PGM_TYPE file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }

    inline void assertNonEqualityInternal(PGM_TYPE file, int line, long x, long y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }
    inline void assertNonEqualityInternal(PGM_TYPE file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }


    inline void assertLessInternal(PGM_TYPE file, int line, short x, short y) {
        internalEquality(file, line, y < x, x, y, "<");
    }
    inline void assertLessInternal(PGM_TYPE file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, y < x, x, y, "<");
    }
    inline void assertLessInternal(PGM_TYPE file, int line, int x, int y) {
        internalEquality(file, line, y < x, x, y, "<");
    }
    inline void assertLessInternal(PGM_TYPE file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, y < x, x, y, "<");
    }
    inline void assertLessInternal(PGM_TYPE file, int line, long x, long y) {
        internalEquality(file, line, y < x, x, y, "<");
    }
    inline void assertLessInternal(PGM_TYPE file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, y < x, x, y, "<");
    }

    inline void assertMoreInternal(PGM_TYPE file, int line, short x, short y) {
        internalEquality(file, line, y > x, x, y, ">");
    }
    inline void assertMoreInternal(PGM_TYPE file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, y > x, x, y, ">");
    }
    inline void assertMoreInternal(PGM_TYPE file, int line, int x, int y) {
        internalEquality(file, line, y > x, x, y, ">");
    }
    inline void assertMoreInternal(PGM_TYPE file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, y > x, x, y, ">");
    }
    inline void assertMoreInternal(PGM_TYPE file, int line, long x, long y) {
        internalEquality(file, line, y > x, x, y, ">");
    }
    inline void assertMoreInternal(PGM_TYPE file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, y > x, x, y, ">");
    }

    // pointers

    inline void assertEqualityInternal(PGM_TYPE file, int line, const void* x, const void* y) {
        internalEquality(file, line, x == y, (int)x, (int)y, "==");
    }

    inline void assertNonEqualityInternal(PGM_TYPE file, int line, const void* x, const void* y) {
        internalEquality(file, line, x != y, (int)x, (int)y, "!=");
    }

    inline void assertEqualityInternal(PGM_TYPE file, int line, const char* x, const char* y) {
        assertStringInternal(file, line, x, y);
    }
}

#define assertTrue(actual) STestInternal::assertBoolInternal(FromPgm(__FILE__), __LINE__, actual, "True")
#define assertFalse(actual) STestInternal::assertBoolInternal(FromPgm(__FILE__), __LINE__, !(actual), "False")
#define assertEquals(expected, actual) STestInternal::assertEqualityInternal(FromPgm(__FILE__), __LINE__, expected, actual)
#define assertNotEquals(expected, actual) STestInternal::assertNonEqualityInternal(FromPgm(__FILE__), __LINE__, expected, actual)
#define assertLessThan(expected, actual) STestInternal::assertLessInternal(FromPgm(__FILE__), __LINE__, expected, actual)
#define assertMoreThan(expected, actual) STestInternal::assertMoreInternal(FromPgm(__FILE__), __LINE__, expected, actual)
#define assertStringEquals(expected, actual) STestInternal::assertStringInternal(FromPgm(__FILE__), __LINE__, expected, actual)
#define assertFloatNear(expected, actual, allowable) STestInternal::assertFloatInternal(FromPgm(__FILE__), __LINE__, expected, actual, allowable)
#define fail(reason) STestInternal::failInternal(FromPgm(__FILE__), __LINE__, reason)

#define testi(name, ignored) \
class UnitTest_##name : public SimpleTest::UnitTestExecutor {\
public:\
  UnitTest_##name();\
  void performTest() override;\
} unitTest_##name##_instance;\
UnitTest_##name :: UnitTest_##name() {\
  init(#name, ignored); \
}\
void UnitTest_##name::performTest()

#define testFi(testClass, name, ignored) \
class testClass ## _ ## name : public testClass {\
public:\
  testClass ## _ ## name();\
  void performTest() override;\
} testClass ## _ ## name ## _instance;\
testClass ## _ ## name :: testClass ## _ ## name() {\
  init(#name, ignored); \
}\
void testClass ## _ ## name :: performTest()

#define test(name) testi(name, false)
#define testF(clazz, name) testFi(clazz, name, false)

#ifdef IOA_USE_ARDUINO
#define DEFAULT_TEST_RUNLOOP void loop() { TestManager::getInstance()->runLoop(); }
#else
#define DEFAULT_TEST_RUNLOOP int main() { setup(); while(1) {TestManager::getInstance()->runLoop();} }
#endif //IOA_USE_ARDUINO

#endif //IOA_SIMPLE_TEST_H
