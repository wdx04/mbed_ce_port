#include "ds18b20.h"

OWI::OWI(PinName pin):owi_io(pin)
{
    
}

void OWI::write0()
{
     owi_io.output();  // timeslot = 70us
     owi_io.write(0);  // owi = 0;
     wait_us(60);
     owi_io.input();
     wait_us(10);       
}

void OWI::write1()    // timeslot = 70us
{
     owi_io.output();
     owi_io.write(0);  // owi = 0;
     wait_us(6);
     owi_io.input();
     wait_us(64);   
}

unsigned char OWI::readBit()
{
     unsigned char readbit;
     
     owi_io.output();
     owi_io.write(0);  // owi = 0;
     wait_us(6);
     owi_io.input();
     wait_us(9);
     readbit = owi_io.read();
     wait_us(55);
     
     return readbit;
}

unsigned char OWI::detectPresence()
{
     core_util_critical_section_enter();
     
     unsigned char presencebit;
     
     owi_io.output();
     owi_io.write(0);  // owi = 0;
     wait_us(480);
     owi_io.input();
     wait_us(70);
     presencebit = !owi_io.read();
     wait_us(410);
     
     core_util_critical_section_exit();
     
     return presencebit;
}

void OWI::sendByte(unsigned char data)
{
     core_util_critical_section_enter();
     
     unsigned char temp;
     unsigned char i;
     for (i = 0; i < 8; i++)
     {
         temp = data & 0x01;
         if (temp)
         {
              this->write1();
         }
         else
         {
              this->write0();
         } 
         data >>= 1;
     }
     
     core_util_critical_section_exit();
}

unsigned char OWI::receiveByte()
{
     core_util_critical_section_enter();
     
     unsigned char data;
     unsigned char i;
     data = 0x00;
     for (i = 0; i < 8; i++)
     {
         data >>= 1;
         if (this->readBit())
         {
               data |= 0x80;
         }
     }
     
     core_util_critical_section_exit();
     return data;
}


DS18B20::DS18B20(PinName pin):DS18B20_OWI(pin)
{

}

float DS18B20::readTemp(bool waitForConversion)
{
    unsigned char msb,lsb;
    int itemp;
    float ftemp;
    
    DS18B20_OWI.detectPresence();
    DS18B20_OWI.sendByte(OWI_SKIP_ROM);
    DS18B20_OWI.sendByte(DS18S20_START);
    
    if(waitForConversion)
    {
        ThisThread::sleep_for(750ms);
    }
        
    DS18B20_OWI.detectPresence();
    DS18B20_OWI.sendByte(OWI_SKIP_ROM);
    DS18B20_OWI.sendByte(DS18S20_READ_SCRATCH_PAD);
    lsb = DS18B20_OWI.receiveByte();
    msb = DS18B20_OWI.receiveByte();
        
    itemp = (msb<<8)+lsb;
    ftemp = (float)itemp / 16.0f;      
    
    return ftemp;
}