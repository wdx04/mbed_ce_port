#ifndef ds18b20_h
#define ds18b20_h

#include "mbed.h"

#define OWI_SKIP_ROM 0xCC
#define DS18S20_START 0x44
#define DS18S20_READ_SCRATCH_PAD 0xBE

class OWI
{
    public:
        OWI(PinName pin);
        void sendByte(unsigned char data);
        unsigned char receiveByte();
        unsigned char detectPresence();
        
    private:
        void write0();
        void write1();
        unsigned char readBit();
        DigitalInOut owi_io;
};

class DS18B20
{
    public:
        DS18B20(PinName pin);
        float readTemp(bool waitForConversion = false);
    
    private:
        OWI DS18B20_OWI;
};

#endif