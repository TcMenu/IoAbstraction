
#include "../IoAbstraction.h"
#include "SimpleTest.h"
#include "../IoLogging.h"
#include "TextUtilities.h"
#include <TaskManagerIO.h>

namespace SimpleTest {

    const char* niceStatus(TestStatus s) {
        switch(s) {
            case NOT_RUN:
                return "Not run";
            case RUNNING:
                return "Running";
            case FAILED:
                return "Failed";
            case PASSED:
                return "Passed";
            case IGNORED:
                return "Ignored";
            default:
                return "????";
        }
    }

    UnitTestExecutor *UnitTestExecutor::currentlyRunning = nullptr;

    void UnitTestExecutor::exec() {
        currentlyRunning = this;
        testStatus = RUNNING;
        serlogF2(SER_DEBUG, "Starting test ", testName);
        setup();
        performTest();
        teardown();
        if (testStatus != FAILED) {
            testStatus = PASSED;
        }
        serlogF3(SER_DEBUG, "Test ", testName, niceStatus(testStatus));
        currentlyRunning = nullptr;
    }

    void UnitTestExecutor::setFailed(PGM_TYPE file, int line, const char *reason) {
        testStatus = FAILED;
        failureReason.withFileAndLine(file, line);
    }

    void UnitTestExecutor::init(const char *name, bool ignored) {
        testName = name;
        if (ignored) {
            testStatus = IGNORED;
        } else {
            TestManager::getInstance()->addTest(this);
        }
    }

    TestManager* TestManager::instance = nullptr;

    void TestManager::printSummary() {
        int total = testsRecorded.count();
        int ignored = 0;
        int failed = 0;
        int passed = 0;
        int unknown = 0;

        for (auto tst: testsRecorded) {
            if (tst.getTest()->getTestStatus() == PASSED) passed++;
            else if (tst.getTest()->getTestStatus() == IGNORED) ignored++;
            else if (tst.getTest()->getTestStatus() == SimpleTest::FAILED) failed++;
            else unknown++;
        }

        if (unknown == 0) {
            char sz[50];
            strncpy(sz, "total=", sizeof sz);
            fastltoa(sz, total, 5, NOT_PADDED, sizeof sz);
            strncat(sz, ", passed=", sizeof(sz) - strlen(sz) - 1);
            fastltoa(sz, passed, 5, NOT_PADDED, sizeof sz);
            strncat(sz, ", failed=", sizeof(sz) - strlen(sz) - 1);
            fastltoa(sz, failed, 5, NOT_PADDED, sizeof sz);
            strncat(sz, ", ignored=", sizeof(sz) - strlen(sz) - 1);
            fastltoa(sz, ignored, 5, NOT_PADDED, sizeof sz);
            serlogF2(SER_DEBUG, "Tests finished - ", sz);
        }

        if(failed > 0) {
            serlogF(SER_DEBUG, "T E S T S   F A I L E D")
        }
    }

    void TestManager::begin() {
        serlogF(SER_DEBUG, "==== 8< ==== 8< ==== START EXECUTION ==== 8< ==== 8< ====");
        serlogF3(SER_DEBUG, "Starting test execution on ", testsRecorded.count(), "tests");
        currentIndex = 0;
        needsSummary = true;

        if(serLevelEnabled(SER_IOA_DEBUG)) {
            serlogF(SER_IOA_DEBUG, "The following tests were added");
            for(auto t : testsRecorded) {
                serlogF2(SER_IOA_DEBUG, "Test: ", t.getTest()->getTestName())
            }
        }
    }

    void TestManager::runLoop() {
        if((bsize_t)currentIndex < testsRecorded.count()) {
            auto t = testsRecorded.itemAtIndex(currentIndex);
            if (t->getTest()->getTestStatus() != IGNORED && (filterPredicate == nullptr || filterPredicate(t->getTest()))) {
                t->getTest()->exec();
            }
            currentIndex = currentIndex + 1;
        } else if(needsSummary) {
            needsSummary = false;
            printSummary();
        }
    }

    TestManager *TestManager::getInstance() {
        if(instance == nullptr) {
            instance = new TestManager();
        }
        return instance;
    }
}

namespace STestInternal {
    void assertBoolInternal(PGM_TYPE file, int line, bool valid, const char* reason) {
        auto current = SimpleTest::UnitTestExecutor::getCurrentTest();
        if(current == nullptr || current->getTestStatus() != SimpleTest::RUNNING) return;

        if(!valid) {
            current->setFailed(file, line, reason);
            serlogF4(SER_DEBUG, "Assertion failure at ", file, ", line", line );
            serlogF2(SER_DEBUG, "Detail: ", reason);
        }
    }

    void assertFloatInternal(PGM_TYPE file, int line, float x, float y, float allowable) {
        auto current = SimpleTest::UnitTestExecutor::getCurrentTest();
        if(current == nullptr || current->getTestStatus() != SimpleTest::RUNNING) return;

        if(tcFltAbs(x - y) > allowable) {
            current->setFailed(file, line, "flt!=");
            serlogF4(SER_DEBUG, "Assertion failure at ", file, ", line", line );
            serlogF4(SER_DEBUG, "Detail: ", x, "==", y);
        }
    }

    void internalEquality(PGM_TYPE file, int line, bool eq, uint32_t x, uint32_t y, const char* how) {
        auto current = SimpleTest::UnitTestExecutor::getCurrentTest();
        if(current == nullptr || current->getTestStatus() != SimpleTest::RUNNING) return;

        if(!eq) {
            current->setFailed(file, line, how);
            serlogF4(SER_DEBUG, "Assertion failure at ", file, ", line", line);
            serlogF4(SER_DEBUG, "Details: ", y, how, x);
        }
    }

    void assertStringInternal(PGM_TYPE file, int line, const char* x, const char* y) {
        auto current = SimpleTest::UnitTestExecutor::getCurrentTest();
        if(current == nullptr || current->getTestStatus() != SimpleTest::RUNNING) return;

        if(strcmp(x, y) != 0) {
            current->setFailed(file, line, "==");
            serlogF4(SER_DEBUG, "Assertion failure at ", file, ", line", line);
            serlogF4(SER_DEBUG, "Details: ", x, "eq", y);
        }
    }

    void failInternal(PGM_TYPE file, int line, const char* reason) {
        auto current = SimpleTest::UnitTestExecutor::getCurrentTest();
        if(current == nullptr || current->getTestStatus() != SimpleTest::RUNNING) return;
        current->setFailed(file, line, "fail()");
        serlogF4(SER_DEBUG, "Assertion failure at ", file, line, "fail() was called");
    }
}