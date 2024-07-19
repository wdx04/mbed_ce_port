#ifndef MMC3680KJ_H
#define MMC3680KJ_H

#include "mbed.h"

#define MMC3680KJ_ADDRESS 0x30

struct Magnets
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

class MMC3680KJ
{
public:

    /** Create a MMC3680KJ instance
     *  which is connected to specified I2C pins with specified address
     *
     * @param i2c_obj I2C object (instance)
     * @param slave_adr (option) I2C-bus address (default: 0x30)
     */
    MMC3680KJ(I2C &i2c_obj, char slave_adr = MMC3680KJ_ADDRESS);

    /** Destructor of MMC3680KJ
     */
    virtual ~MMC3680KJ();

    /** Initializa MMC3680KJ sensor
     *
     *  Configure sensor setting and read parameters for calibration
     *
     */
    bool initialize(void);

    /** Get the ID from MMC3680KJ sensor
     *
     */
    uint8_t getId();

    /** Enable the MMC3680KJ sensor
     *
     */
    bool enable();

    /** Read the current magnet values from MMC3680KJ sensor
     *
     */
    bool getMagnets(Magnets *result);

private:
    I2C &i2c;
    char address;
};

#endif // MMC3680KJ_H
