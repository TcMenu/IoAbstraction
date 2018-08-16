#ifndef _IO_ABSTRACTION_CORE_TYPES
#define _IO_ABSTRACTION_CORE_TYPES

typedef void (*RawIntHandler)(void);


class BasicIoAbstraction {
public:
	virtual ~BasicIoAbstraction() { }
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void attachInterrupt(uint8_t pin, RawIntHandler interruptHandler, uint8_t mode);
	virtual void runLoop() { ; }
};

// Help with usage of the library without resorting pointers
typedef BasicIoAbstraction* IoAbstractionRef;

#endif