#ifndef IOA_SIMPLE_TEST_H
#define IOA_SIMPLE_TEST_H

/**
 * @file SimpleTest.h - a very simple test framework for all platforms that IoAbstraction supports using taskmanager as
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

namespace SimpleTest {

    enum TestStatus {
        NOT_RUN, RUNNING, FAILED, PASSED, IGNORED
    };

    const char* niceStatus(TestStatus s);

    class FailureInfo {
    private:
        const char* file = "";
        int line = 0;
        char reason[FAIL_REASON_SIZE];
    public:
        FailureInfo() {}
        void withFileAndLine(const char* f, int l) {
            file = f;
            line = l;
        }
        void withReason(const char* r) {
            strncpy(reason, r, sizeof reason);
        }
    };

    class UnitTestExecutor : public Executable {
    private:
        static UnitTestExecutor *currentlyRunning;
        TestStatus testStatus = NOT_RUN;
        FailureInfo failureReason;
        char testName[32] = {};
    public:
        void init(const char* name, bool ignored = false);

        virtual void performTest() = 0;

        TestStatus getTestStatus() const { return testStatus; }
        void exec() override;
        void setFailed(const char* file, int line, const char* reason);

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

    class TestManager : public BaseEvent {
    private:
        BtreeList<uint8_t, ExecutionWithId> testsRecorded;
        static TestManager* instance;

        TestManager(): testsRecorded(32) {}

    public:
        static TestManager* getInstance();

        void begin();

        void addTest(UnitTestExecutor* t) {
            testsRecorded.add(ExecutionWithId(testsRecorded.count(), t));
        }

        void exec() override;

        uint32_t timeOfNextCheck() override;

        void printSummary();
    };

    inline void startTesting() {
        TestManager::getInstance()->begin();
    }
}

namespace STestInternal {

    void internalEquality(const char *file, int line, bool success, uint32_t x, uint32_t y, const char *how);

    void assertStringInternal(const char *file, int line, const char *x, const char *y);

    void failInternal(const char *file, int line, const char *reason);

    void assertBoolInternal(const char *file, int line, bool valid, const char *reason);

    inline void assertEqualityInternal(const char *file, int line, short x, short y) {
        internalEquality(file, line, x == y, x, y, "==");
    }
    inline void assertEqualityInternal(const char *file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, x == y, x, y, "==");
    }

    inline void assertEqualityInternal(const char *file, int line, int x, int y) {
        internalEquality(file, line, x != y, x, y, "==");
    }
    inline void assertEqualityInternal(const char *file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, x != y, x, y, "==");
    }

    inline void assertEqualityInternal(const char *file, int line, long x, long y) {
        internalEquality(file, line, x != y, x, y, "==");
    }
    inline void assertEqualityInternal(const char *file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, x != y, x, y, "==");
    }

    inline void assertNonEqualityInternal(const char *file, int line, short x, short y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }
    inline void assertNonEqualityInternal(const char *file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }

    inline void assertNonEqualityInternal(const char *file, int line, int x, int y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }
    inline void assertNonEqualityInternal(const char *file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }

    inline void assertNonEqualityInternal(const char *file, int line, long x, long y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }
    inline void assertNonEqualityInternal(const char *file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, x != y, x, y, "!=");
    }


    inline void assertLessInternal(const char *file, int line, short x, short y) {
        internalEquality(file, line, x < y, x, y, "<");
    }
    inline void assertLessInternal(const char *file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, x < y, x, y, "<");
    }
    inline void assertLessInternal(const char *file, int line, int x, int y) {
        internalEquality(file, line, x < y, x, y, "<");
    }
    inline void assertLessInternal(const char *file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, x < y, x, y, "<");
    }
    inline void assertLessInternal(const char *file, int line, long x, long y) {
        internalEquality(file, line, x < y, x, y, "<");
    }
    inline void assertLessInternal(const char *file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, x < y, x, y, "<");
    }

    inline void assertMoreInternal(const char *file, int line, short x, short y) {
        internalEquality(file, line, x < y, x, y, ">");
    }
    inline void assertMoreInternal(const char *file, int line, unsigned short x, unsigned short y) {
        internalEquality(file, line, x < y, x, y, ">");
    }
    inline void assertMoreInternal(const char *file, int line, int x, int y) {
        internalEquality(file, line, x < y, x, y, ">");
    }
    inline void assertMoreInternal(const char *file, int line, unsigned int x, unsigned int y) {
        internalEquality(file, line, x < y, x, y, ">");
    }
    inline void assertMoreInternal(const char *file, int line, long x, long y) {
        internalEquality(file, line, x < y, x, y, ">");
    }
    inline void assertMoreInternal(const char *file, int line, unsigned long x, unsigned long y) {
        internalEquality(file, line, x < y, x, y, ">");
    }

}

#define assertTrue(x) STestInternal::assertBoolInternal(__FILE__, __LINE__, x, "True")
#define assertFalse(x) STestInternal::assertBoolInternal(__FILE__, __LINE__, !(x), "False")
#define assertEquals(x, y) STestInternal::assertEqualityInternal(__FILE__, __LINE__, x, y)
#define assertNotEquals(x, y) STestInternal::assertNonEqualityInternal(__FILE__, __LINE__, x, y)
#define assertLessThan(x, y) STestInternal::assertLessInternal(__FILE__, __LINE__, x, y)
#define assertMoreThan(x, y) STestInternal::assertMoreInternal(__FILE__, __LINE__, x, y)
#define assertStringEquals(x, y) STestInternal::assertStringInternal(__FILE__, __LINE__, x, y)
#define fail(x) STestInternal::failInternal(__FILE__, __LINE__, x)

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

#define test(name) testi(name, false)

#endif //IOA_SIMPLE_TEST_H
