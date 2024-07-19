#include "STMPE811.h"

#define I2C_ADDR_STMPE811 0x41
//
#define STMPE811_ADDR_READ 0x83
#define STMPE811_ADDR_WRITE 0x82

/* Chip IDs */
#define STMPE811_ID 0x0811

/* Identification registers & System Control */
#define STMPE811_REG_CHP_ID_LSB 0x00
#define STMPE811_REG_CHP_ID_MSB 0x01
#define STMPE811_REG_ID_VER 0x02

/* Global interrupt Enable bit */
#define STMPE811_GIT_EN 0x01

/* IO expander functionalities */
#define STMPE811_ADC_FCT 0x01
#define STMPE811_TS_FCT 0x02
#define STMPE811_IO_FCT 0x04
#define STMPE811_TEMPSENS_FCT 0x08

/* Global Interrupts definitions */
#define STMPE811_GIT_IO 0x80                                                                                          /* IO interrupt                   */
#define STMPE811_GIT_ADC 0x40                                                                                         /* ADC interrupt                  */
#define STMPE811_GIT_TEMP 0x20                                                                                        /* Not implemented                */
#define STMPE811_GIT_FE 0x10                                                                                          /* FIFO empty interrupt           */
#define STMPE811_GIT_FF 0x08                                                                                          /* FIFO full interrupt            */
#define STMPE811_GIT_FOV 0x04                                                                                         /* FIFO overflowed interrupt      */
#define STMPE811_GIT_FTH 0x02                                                                                         /* FIFO above threshold interrupt */
#define STMPE811_GIT_TOUCH 0x01                                                                                       /* Touch is detected interrupt    */
#define STMPE811_ALL_GIT 0x1F                                                                                         /* All global interrupts          */
#define STMPE811_TS_IT (STMPE811_GIT_TOUCH | STMPE811_GIT_FTH | STMPE811_GIT_FOV | STMPE811_GIT_FF | STMPE811_GIT_FE) /* Touch screen interrupts */

/* General Control Registers */
#define STMPE811_REG_SYS_CTRL1 0x03
#define STMPE811_REG_SYS_CTRL2 0x04
#define STMPE811_REG_SPI_CFG 0x08

/* Interrupt system Registers */
#define STMPE811_REG_INT_CTRL 0x09
#define STMPE811_REG_INT_EN 0x0A
#define STMPE811_REG_INT_STA 0x0B
#define STMPE811_REG_IO_INT_EN 0x0C
#define STMPE811_REG_IO_INT_STA 0x0D

/* IO Registers */
#define STMPE811_REG_IO_SET_PIN 0x10
#define STMPE811_REG_IO_CLR_PIN 0x11
#define STMPE811_REG_IO_MP_STA 0x12
#define STMPE811_REG_IO_DIR 0x13
#define STMPE811_REG_IO_ED 0x14
#define STMPE811_REG_IO_RE 0x15
#define STMPE811_REG_IO_FE 0x16
#define STMPE811_REG_IO_AF 0x17

/* ADC Registers */
#define STMPE811_REG_ADC_INT_EN 0x0E
#define STMPE811_REG_ADC_INT_STA 0x0F
#define STMPE811_REG_ADC_CTRL1 0x20
#define STMPE811_REG_ADC_CTRL2 0x21
#define STMPE811_REG_ADC_CAPT 0x22
#define STMPE811_REG_ADC_DATA_CH0 0x30
#define STMPE811_REG_ADC_DATA_CH1 0x32
#define STMPE811_REG_ADC_DATA_CH2 0x34
#define STMPE811_REG_ADC_DATA_CH3 0x36
#define STMPE811_REG_ADC_DATA_CH4 0x38
#define STMPE811_REG_ADC_DATA_CH5 0x3A
#define STMPE811_REG_ADC_DATA_CH6 0x3B
#define STMPE811_REG_ADC_DATA_CH7 0x3C

/* Touch Screen Registers */
#define STMPE811_REG_TSC_CTRL 0x40
#define STMPE811_REG_TSC_CFG 0x41
#define STMPE811_REG_WDM_TR_X 0x42
#define STMPE811_REG_WDM_TR_Y 0x44
#define STMPE811_REG_WDM_BL_X 0x46
#define STMPE811_REG_WDM_BL_Y 0x48
#define STMPE811_REG_FIFO_TH 0x4A
#define STMPE811_REG_FIFO_STA 0x4B
#define STMPE811_REG_FIFO_SIZE 0x4C
#define STMPE811_REG_TSC_DATA_X 0x4D
#define STMPE811_REG_TSC_DATA_Y 0x4F
#define STMPE811_REG_TSC_DATA_Z 0x51
#define STMPE811_REG_TSC_DATA_XYZ 0x52
#define STMPE811_REG_TSC_FRACT_XYZ 0x56
#define STMPE811_REG_TSC_DATA_INC 0x57
#define STMPE811_REG_TSC_DATA_NON_INC 0xD7
#define STMPE811_REG_TSC_I_DRIVE 0x58
#define STMPE811_REG_TSC_SHIELD 0x59

/* Touch Screen Pins definition */
#define STMPE811_TOUCH_YD STMPE811_PIN_7
#define STMPE811_TOUCH_XD STMPE811_PIN_6
#define STMPE811_TOUCH_YU STMPE811_PIN_5
#define STMPE811_TOUCH_XU STMPE811_PIN_4
#define STMPE811_TOUCH_IO_ALL (uint32_t)(STMPE811_TOUCH_YD | STMPE811_TOUCH_XD | STMPE811_TOUCH_YU | STMPE811_TOUCH_XU)

/* IO Pins definition */
#define STMPE811_PIN_0 0x01
#define STMPE811_PIN_1 0x02
#define STMPE811_PIN_2 0x04
#define STMPE811_PIN_3 0x08
#define STMPE811_PIN_4 0x10
#define STMPE811_PIN_5 0x20
#define STMPE811_PIN_6 0x40
#define STMPE811_PIN_7 0x80
#define STMPE811_PIN_ALL 0xFF

/* IO Pins directions */
#define STMPE811_DIRECTION_IN 0x00
#define STMPE811_DIRECTION_OUT 0x01

/* IO IT types */
#define STMPE811_TYPE_LEVEL 0x00
#define STMPE811_TYPE_EDGE 0x02

/* IO IT polarity */
#define STMPE811_POLARITY_LOW 0x00
#define STMPE811_POLARITY_HIGH 0x04

/* IO Pin IT edge modes */
#define STMPE811_EDGE_FALLING 0x01
#define STMPE811_EDGE_RISING 0x02

/* TS registers masks */
#define STMPE811_TS_CTRL_ENABLE 0x01
#define STMPE811_TS_CTRL_STATUS 0x80

bool STMPE811::init()
{
  // Reset
  char command[2];
  command[0] = STMPE811_REG_SYS_CTRL1;
  command[1] = 2;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  ThisThread::sleep_for(10ms);
  command[1] = 0;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  ThisThread::sleep_for(2ms);

  /* Get the current register value */
  command[0] = STMPE811_REG_SYS_CTRL2;
  i2c.write(STMPE811_ADDR_WRITE, command, 1);
  i2c.read(STMPE811_ADDR_READ, command, 1);
  uint8_t mode = uint8_t(command[0]);
  
  /* Set the Functionalities to be Enabled */    
  mode &= ~(STMPE811_IO_FCT);  
  
  /* Write the new register value */  
  command[0] = STMPE811_REG_SYS_CTRL2;
  command[1] = mode;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);

  /* Select TSC pins in TSC alternate mode */
  command[0] = STMPE811_REG_IO_AF;
  i2c.write(STMPE811_ADDR_WRITE, command, 1);
  i2c.read(STMPE811_ADDR_READ, command, 1);

  uint8_t tmp = 0;
  /* Get the current register value */
  tmp = uint8_t(command[0]);
  /* Enable the selected pins alternate function */   
  tmp &= ~(uint8_t)STMPE811_TOUCH_IO_ALL;   
  /* Write back the new register value */
  command[0] = STMPE811_REG_IO_AF;
  command[1] = char(tmp);
  i2c.write(STMPE811_ADDR_WRITE, command, 2);

  /* Set the Functionalities to be Enabled */    
  mode &= ~(STMPE811_TS_FCT | STMPE811_ADC_FCT);  
  
  /* Set the new register value */ 
  command[0] = STMPE811_REG_SYS_CTRL2;
  command[1] = mode;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Select Sample Time, bit number and ADC Reference */
  command[0] = STMPE811_REG_ADC_CTRL1;
  command[1] = 0x49;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Wait for 2 ms */
  ThisThread::sleep_for(2ms);
  
  /* Select the ADC clock speed: 3.25 MHz */
  command[0] = STMPE811_REG_ADC_CTRL2;
  command[1] = 0x01;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Select 2 nF filter capacitor */
  /* Configuration: 
     - Touch average control    : 4 samples
     - Touch delay time         : 500 uS
     - Panel driver setting time: 500 uS 
  */
  command[0] = STMPE811_REG_TSC_CFG;
  command[1] = 0x9A;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Configure the Touch FIFO threshold: single point reading */
  command[0] = STMPE811_REG_FIFO_TH;
  command[1] = 0x01;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Clear the FIFO memory content. */
  command[0] = STMPE811_REG_FIFO_STA;
  command[1] = 0x01;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Put the FIFO back into operation mode  */
  command[0] = STMPE811_REG_FIFO_STA;
  command[1] = 0x00;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Set the range and accuracy pf the pressure measurement (Z) : 
     - Fractional part :7 
     - Whole part      :1 
  */
  command[0] = STMPE811_REG_TSC_FRACT_XYZ;
  command[1] = 0x01;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Set the driving capability (limit) of the device for TSC pins: 50mA */
  command[0] = STMPE811_REG_TSC_I_DRIVE;
  command[1] = 0x01;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /* Touch screen control configuration (enable TSC):
     - No window tracking index
     - XYZ acquisition mode
   */
  command[0] = STMPE811_REG_TSC_CTRL;
  command[1] = 0x01;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  
  /*  Clear all the status pending bits if any */
  command[0] = STMPE811_REG_INT_STA;
  command[1] = 0xFF;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);

  /* Wait for 2 ms delay */
  ThisThread::sleep_for(2ms);
  return true;
}

void STMPE811::set_conversion(float _x_a, float _x_offset, float _y_b, float _y_offset)
{
    x_a = _x_a;
    x_b = 0.0f;
    x_offset = _x_offset;
    y_a = 0.0f;
    y_b = _y_b;
    y_offset = _y_offset;
}

void STMPE811::set_conversion(float _x_a, float _x_b, float _x_offset, float _y_a, float _y_b, float _y_offset)
{
    x_a = _x_a;
    x_b = _x_b;
    x_offset = _x_offset;
    y_a = _y_a;
    y_b = _y_b;
    y_offset = _y_offset;
}

bool STMPE811::get_touch_state(int16_t &x, int16_t &y)
{
  char command[2];
  command[0] = STMPE811_REG_TSC_CTRL;
  i2c.write(STMPE811_ADDR_WRITE, command, 1);
  i2c.read(STMPE811_ADDR_READ, command, 1);
  if ((uint8_t(command[0]) & STMPE811_TS_CTRL_STATUS) != (uint8_t)STMPE811_TS_CTRL_STATUS)
  {
    // Reset FIFO
    command[0] = STMPE811_REG_FIFO_STA;
    command[1] = 1;
    i2c.write(STMPE811_ADDR_WRITE, command, 2);
    command[1] = 0;
    i2c.write(STMPE811_ADDR_WRITE, command, 2);
    return false;
  }
  command[0] = STMPE811_REG_FIFO_SIZE;
  i2c.write(STMPE811_ADDR_WRITE, command, 1);
  i2c.read(STMPE811_ADDR_READ, command, 1);
  if (command[0] <= 0)
  {
    return false;
  }

  uint8_t dataXYZ[4];
  uint32_t uldataXYZ;

  command[0] = STMPE811_REG_TSC_DATA_NON_INC;
  i2c.write(STMPE811_ADDR_WRITE, command, 1);
  i2c.read(STMPE811_ADDR_READ, reinterpret_cast<char *>(dataXYZ), sizeof(dataXYZ));

  // Calculate positions values
  uldataXYZ = (dataXYZ[0] << 24) | (dataXYZ[1] << 16) | (dataXYZ[2] << 8) | (dataXYZ[3] << 0);
  uint16_t raw_x = uint16_t((uldataXYZ >> 20) & 0x00000FFF);
  uint16_t raw_y = uint16_t((uldataXYZ >> 8) & 0x00000FFF);

  // Reset FIFO
  command[0] = STMPE811_REG_FIFO_STA;
  command[1] = 1;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);
  command[1] = 0;
  i2c.write(STMPE811_ADDR_WRITE, command, 2);

  x = int16_t(float(raw_x) * x_a + float(raw_y) * x_b + x_offset);
  y = int16_t(float(raw_x) * y_a + float(raw_y) * y_b + y_offset);

  return true;
}
