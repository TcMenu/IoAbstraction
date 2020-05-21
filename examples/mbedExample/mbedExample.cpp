#include "mbed.h"
#include "rtos.h"
#include <IoLogging.h>

int count = 0;

int main() {
    serdebugF2("hello from mbed", count);
    ThisThread::sleep_for(1.0);
}
