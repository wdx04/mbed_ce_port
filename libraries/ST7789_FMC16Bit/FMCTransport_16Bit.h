#pragma once

#if defined(FSMC_NORSRAM_DEVICE) || defined(FMC_NORSRAM_DEVICE)

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} TFT_LCD_TypeDef;

class FMCTransport_16Bit
{
public:
    // subbank_no can be 1 or 2
    // address_no can be 0 or 19
    FMCTransport_16Bit(int subbank_no, int address_no);

    bool is_valid();

    void write_register(uint8_t regval);

    void write_data(uint16_t data);

    void write_data(const uint16_t *data, size_t count);

    uint16_t read_data(void);

    volatile uint16_t* get_register_pointer();

    volatile uint16_t* get_data_pointer();

private:
    void init(unsigned int subbank_no, unsigned int address_no);
};

#endif
