/* mbed TextLCD Library, for a 4-bit LCD based on HD44780
 * Copyright (c) 2007-2010, sford, http://mbed.org
 *               2013, v01: WH, Added LCD types, fixed LCD address issues, added Cursor and UDCs 
 *               2013, v02: WH, Added I2C and SPI bus interfaces  
 *               2013, v03: WH, Added support for LCD40x4 which uses 2 controllers 
 *               2013, v04: WH, Added support for Display On/Off, improved 4bit bootprocess
 *               2013, v05: WH, Added support for 8x2B, added some UDCs   
 *               2013, v06: WH, Added support for devices that use internal DC/DC converters 
 *               2013, v07: WH, Added support for backlight and include portdefinitions for LCD2004 Module from DFROBOT 
 *               2014, v08: WH, Refactored in Base and Derived Classes to deal with mbed lib change regarding 'NC' defined pins 
 *               2014, v09: WH/EO, Added Class for Native SPI controllers such as ST7032 
 *               2014, v10: WH, Added Class for Native I2C controllers such as ST7032i, Added support for MCP23008 I2C portexpander, Added support for Adafruit module  
 *               2014, v11: WH, Added support for native I2C controllers such as PCF21XX, Improved the _initCtrl() method to deal with differences between all supported controllers  
 *               2014, v12: WH, Added support for native I2C controller PCF2119 and native I2C/SPI controllers SSD1803, ST7036, added setContrast method (by JH1PJL) for supported devices (eg ST7032i) 
 *               2014, v13: WH, Added support for controllers US2066/SSD1311 (OLED), added setUDCBlink method for supported devices (eg SSD1803), fixed issue in setPower() 
 *@Todo Add AC780S/KS0066i
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "TextLCD.h"
#include "mbed.h"

//For Testing only
//DigitalOut led1(LED1);
//DigitalOut led2(LED2);
//  led2=!led2;    


// User Defined Characters (UDCs) are defined by an 8 byte bitpattern. The P0..P5 form the character pattern.
//     P7 P6 P5 P4 P3 P2 P1 P0 
// 0   B1 B0  x  0  1  1  1  0
// 1   B1 B0  x  1  0  0  0  1
// .       .............
// 7   B1 B0  x  1  0  0  0  1
//
// Blinking UDCs are enabled when a specific controlbit (BE) is set.
// The blinking pixels in the UDC can be controlled by setting additional bits in the UDC bitpattern.
// Bit 6 and Bit 7 in the pattern will control the blinking mode when Blink is enabled through BE. 
//     B1 B0  Mode
//      0  0  No Blinking in this row of the UDC
//      0  1  Enabled pixels in P4 will blink
//      1  x  Enabled pixels in P0..P4 will blink

/** Some sample User Defined Chars 5x7 dots */
//const char udc_ae[] = {0x00, 0x00, 0x1B, 0x05, 0x1F, 0x14, 0x1F, 0x00};  //æ
//const char udc_0e[] = {0x00, 0x00, 0x0E, 0x13, 0x15, 0x19, 0x0E, 0x00};  //ø
//const char udc_ao[] = {0x0E, 0x0A, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00};  //å
//const char udc_AE[] = {0x0F, 0x14, 0x14, 0x1F, 0x14, 0x14, 0x17, 0x00};  //Æ
//const char udc_0E[] = {0x0E, 0x13, 0x15, 0x15, 0x15, 0x19, 0x0E, 0x00};  //Ø
//const char udc_Ao[] = {0x0E, 0x0A, 0x0E, 0x11, 0x1F, 0x11, 0x11, 0x00};  //Å
//const char udc_PO[] = {0x04, 0x0A, 0x0A, 0x1F, 0x1B, 0x1B, 0x1F, 0x00};  //Padlock Open
//const char udc_PC[] = {0x1C, 0x10, 0x08, 0x1F, 0x1B, 0x1B, 0x1F, 0x00};  //Padlock Closed

//const char udc_alpha[] = {0x00, 0x00, 0x0D, 0x12, 0x12, 0x12, 0x0D, 0x00};  //alpha
//const char udc_ohm[]   = {0x0E, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x1B, 0x00};  //ohm
//const char udc_sigma[] = {0x1F, 0x08, 0x04, 0x02, 0x04, 0x08, 0x1F, 0x00};  //sigma
//const char udc_pi[]    = {0x1F, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x00};  //pi
//const char udc_root[]  = {0x07, 0x04, 0x04, 0x04, 0x14, 0x0C, 0x04, 0x00};  //root

const char udc_0[]  = {0x18, 0x14, 0x12, 0x11, 0x12, 0x14, 0x18, 0x00};  // |>
const char udc_1[]  = {0x03, 0x05, 0x09, 0x11, 0x09, 0x05, 0x03, 0x00};  // <|
const char udc_2[]  = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00};  // |
const char udc_3[]  = {0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00};  // ||
const char udc_4[]  = {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00};  // |||
const char udc_5[]  = {0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00};  // =
const char udc_6[]  = {0x15, 0x0a, 0x15, 0x0a, 0x15, 0x0a, 0x15, 0x00};  // checkerboard
const char udc_7[]  = {0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x10, 0x00};  // \

const char udc_degr[]   = {0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00};  // Degree symbol

const char udc_TM_T[]   = {0x1F, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};  // Trademark T
const char udc_TM_M[]   = {0x11, 0x1B, 0x15, 0x11, 0x00, 0x00, 0x00, 0x00};  // Trademark M

//const char udc_Bat_Hi[] = {0x0E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};  // Battery Full
//const char udc_Bat_Ha[] = {0x0E, 0x11, 0x13, 0x17, 0x1F, 0x1F, 0x1F, 0x00};  // Battery Half
//const char udc_Bat_Lo[] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x00};  // Battery Low
//const char udc_Bat_Hi[] = {0x0E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};  // Battery Full
//const char udc_Bat_Ha[] = {0x0E, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};  // Battery Half
//const char udc_Bat_Lo[] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x1F, 0x00};  // Battery Low
const char udc_Bat_Hi[] = {0x8E, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x00};  // Battery Full, Blink
const char udc_Bat_Ha[] = {0x8E, 0x91, 0x91, 0x9F, 0x9F, 0x9F, 0x9F, 0x00};  // Battery Half, Blink
const char udc_Bat_Lo[] = {0x8E, 0x91, 0x91, 0x91, 0x91, 0x9F, 0x9F, 0x00};  // Battery Low, Blink
const char udc_AC[]     = {0x0A, 0x0A, 0x1F, 0x11, 0x0E, 0x04, 0x04, 0x00};  // AC Power

//const char udc_smiley[] = {0x00, 0x0A, 0x00, 0x04, 0x11, 0x0E, 0x00, 0x00};  // Smiley
//const char udc_droopy[] = {0x00, 0x0A, 0x00, 0x04, 0x00, 0x0E, 0x11, 0x00};  // Droopey
//const char udc_note[]   = {0x01, 0x03, 0x05, 0x09, 0x0B, 0x1B, 0x18, 0x00};  // Note

//const char udc_bar_1[]  = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00};  // Bar 1
//const char udc_bar_2[]  = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00};  // Bar 11
//const char udc_bar_3[]  = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x00};  // Bar 111
//const char udc_bar_4[]  = {0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x00};  // Bar 1111
//const char udc_bar_5[]  = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};  // Bar 11111

//const char udc_ch_1[]  =  {0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00};  // Hor bars 4
//const char udc_ch_2[]  =  {0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f};  // Hor bars 4 (inverted)
//const char udc_ch_3[]  =  {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15};  // Ver bars 3
//const char udc_ch_4[]  =  {0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a};  // Ver bars 3 (inverted)
//const char udc_ch_yr[] =  {0x08, 0x0f, 0x12, 0x0f, 0x0a, 0x1f, 0x02, 0x02};  // Year   (kana)
//const char udc_ch_mo[] =  {0x0f, 0x09, 0x0f, 0x09, 0x0f, 0x09, 0x09, 0x13};  // Month  (kana)
//const char udc_ch_dy[] =  {0x1f, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11, 0x1F};  // Day    (kana)
//const char udc_ch_mi[] =  {0x0C, 0x0a, 0x11, 0x1f, 0x09, 0x09, 0x09, 0x13};  // minute (kana)

//const char udc_bell[]     = {0x04,0x0E,0x0E,0x0E,0x1F,0x00,0x04};
//const char udc_note[]     = {0x02,0x03,0x02,0x0E,0x1E,0x0C,0x00};
//const char udc_clock[]    = {0x00,0x0E,0x15,0x17,0x11,0x0E,0x00};
//const char udc_heart[]    = {0x00,0x0a,0x1F,0x1F,0x0E,0x04,0x00};
//const char udc_duck[]     = {0x00,0x0c,0x1D,0x0F,0x0F,0x06,0x00};
//const char udc_check[]    = {0x00,0x01,0x03,0x16,0x1C,0x08,0x00};
//const char udc_cross[]    = {0x00,0x1B,0x0E,0x04,0x0E,0x1B,0x00};
//const char udc_retarrow[] = {0x01,0x01,0x05,0x09,0x1f,0x08,0x04};

const char udc_None[]    =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
const char udc_All[]     =  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
    
/** Create a TextLCD_Base interface
  *
  * @param type  Sets the panel size/addressing mode (default = LCD16x2)
  * @param ctrl  LCD controller (default = HD44780)           
  */
TextLCD_Base::TextLCD_Base(LCDType type, LCDCtrl ctrl) : _type(type), _ctrl(ctrl) {
    
  // Extract LCDType data  

  // Columns encoded in b7..b0
  _nr_cols = (_type & 0xFF);          

  // Rows encoded in b15..b8  
  _nr_rows = ((_type >> 8) & 0xFF);  

  // Addressing mode encoded in b19..b16  
  _addr_mode = _type & LCD_T_ADR_MSK;
}


/**  Init the LCD Controller(s)
  *  Clear display 
  */
void TextLCD_Base::_init() {
  
  // Select and configure second LCD controller when needed
  if(_type==LCD40x4) {
    _ctrl_idx=_LCDCtrl_1;        // Select 2nd controller   
    _initCtrl();                 // Init 2nd controller   
  }
    
  // Select and configure primary LCD controller
  _ctrl_idx=_LCDCtrl_0;          // Select primary controller  
  _initCtrl();                   // Init primary controller

  // Clear whole display and Reset Cursor location
  // Note: This will make sure that some 3-line displays that skip topline of a 4-line configuration 
  //       are cleared and init cursor correctly.
  cls();   
} 

/**  Init the LCD controller
  *  4-bit mode, number of lines, fonttype, no cursor etc
  *
  *  Note: some configurations are commented out because they have not yet been tested due to lack of hardware
  */
void TextLCD_Base::_initCtrl() {
  int _bias_lines=0; // Set Bias and lines (Instr Set 1), temporary variable.
  int _lines=0;      // Set lines (Ext Instr Set), temporary variable.
      
    this->_setRS(false); // command mode
    
    ThisThread::sleep_for(20ms);         // Wait 20ms to ensure powered up

    // The Controller could be in 8 bit mode (power-on reset) or in 4 bit mode (warm reboot) at this point.
    // Follow this procedure to make sure the Controller enters the correct state. The hardware interface
    // between the uP and the LCD can only write the 4 most significant bits (Most Significant Nibble, MSN).
    // In 4 bit mode the LCD expects the MSN first, followed by the LSN.
    //
    //    Current state:               8 bit mode                |  4 bit mode, MSN is next      | 4 bit mode, LSN is next          
                         //-------------------------------------------------------------------------------------------------                          
    _writeNibble(0x3);   //  set 8 bit mode (MSN) and dummy LSN, |   set 8 bit mode (MSN),       |    set dummy LSN, 
                         //  remains in 8 bit mode               |    change to 8 bit mode       |  remains in 4 bit mode
    ThisThread::sleep_for(15ms);         //                           
    
    _writeNibble(0x3);   //  set 8 bit mode and dummy LSN,       | set 8 bit mode and dummy LSN, |    set 8bit mode (MSN), 
                         //  remains in 8 bit mode               |   remains in 8 bit mode       |  remains in 4 bit mode
    ThisThread::sleep_for(15ms);         // 
    
    _writeNibble(0x3);   //  set 8 bit mode and dummy LSN,       | set 8 bit mode and dummy LSN, |    set dummy LSN, 
                         //  remains in 8 bit mode               |   remains in 8 bit mode       |  change to 8 bit mode
    ThisThread::sleep_for(15ms);         // 

    // Controller is now in 8 bit mode

    _writeNibble(0x2);   // Change to 4-bit mode (MSN), the LSN is undefined dummy
    wait_us(40);         // most instructions take 40us

    // Display is now in 4-bit mode
    // Note: 4/8 bit mode is ignored for most native SPI and I2C devices. They dont use the parallel bus.
    //       However, _writeNibble() method is void anyway for native SPI and I2C devices.
   
    // Device specific initialisations: DC/DC converter to generate VLCD or VLED, number of lines etc
    switch (_ctrl) {

      case KS0078:
          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2B is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:                                            
//            case LCD20x1:
            case LCD24x1:
              _function  = 0x02;    // Function set 001 DL N RE(0) DH REV (Std Regs)
                                    //   DL=0  (4 bits bus)             
                                    //    N=0  (1 line mode), N=1 (2 line mode)
                                    //   RE=0  (Dis. Extended Regs, special mode for KS0078)
                                    //   DH=1  (Disp shift enable, special mode for KS0078)                                
                                    //   REV=0 (Reverse normal, special mode for KS0078)
                                    
              _function_1 = 0x04;   // Function set 001 DL N RE(1) BE 0 (Ext Regs)
                                    //   DL=0  (4 bits bus)             
                                    //    N=0  (1 line mode), N=1 (2 line mode)
                                    //   RE=1  (Ena Extended Regs, special mode for KS0078)
                                    //   BE=0  (Blink Enable, CG/SEG RAM, special mode for KS0078)                                
                                    //      0 

              _function_x = 0x00;   // Ext Function set 0000 1 FW BW NW (Ext Regs)
                                    //    NW=0  (1,2 line), NW=1 (4 Line, special mode for KS0078)                                
              break;                                

//            case LCD12x3D:         // Special mode for KS0078 and PCF21XX            
//            case LCD12x3D1:        // Special mode for KS0078 and PCF21XX            
//            case LCD12x4D:         // Special mode for KS0078 and PCF21XX            
//            case LCD16x3D:         // Special mode for KS0078             
//            case LCD16x4D:         // Special mode for KS0078            
//            case LCD24x3D:         // Special mode for KS0078
//            case LCD24x3D1:        // Special mode for KS0078
            case LCD24x4D:         // Special mode for KS0078
              _function  = 0x02;    // Function set 001 DL N RE(0) DH REV (Std Regs)
                                    //   DL=0  (4 bits bus)             
                                    //    N=0  (dont care for 4 line mode)              
                                    //   RE=0  (Dis. Extended Regs, special mode for KS0078)
                                    //   DH=1  (Disp shift enable, special mode for KS0078)                                
                                    //   REV=0 (Reverse normal, special mode for KS0078)
                                    
              _function_1 = 0x04;   // Function set 001 DL N RE(1) BE 0 (Ext Regs)
                                    //   DL=0  (4 bits bus)             
                                    //    N=0  (1 line mode), N=1 (2 line mode)
                                    //   RE=1  (Ena Extended Regs, special mode for KS0078)
                                    //   BE=0  (Blink Enable, CG/SEG RAM, special mode for KS0078)                                
                                    //      0 

              _function_x = 0x01;   // Ext Function set 0000 1 FW BW NW (Ext Regs)
                                    //    NW=0  (1,2 line), NW=1 (4 Line, special mode for KS0078)                                
              break;                                

            case LCD16x3G:            // Special mode for ST7036                        
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  
              
            default:
              // All other LCD types are initialised as 2 Line displays (including LCD16x1C and LCD40x4)            
              _function  = 0x0A;    // Function set 001 DL N RE(0) DH REV (Std Regs)
                                    //   DL=0  (4 bits bus)             
                                    //    N=1  (1 line mode), N=1 (2 line mode)
                                    //   RE=0  (Dis. Extended Regs, special mode for KS0078)
                                    //   DH=1  (Disp shift enable, special mode for KS0078)                                
                                    //   REV=0 (Reverse normal, special mode for KS0078)
                                    
              _function_1 = 0x0C;   // Function set 001 DL N RE(1) BE 0 (Ext Regs)
                                    //   DL=0  (4 bits bus)             
                                    //    N=1  (1 line mode), N=1 (2 line mode)
                                    //   RE=1  (Ena Extended Regs, special mode for KS0078)
                                    //   BE=0  (Blink Enable, CG/SEG RAM, special mode for KS0078)                                
                                    //      0 

              _function_x = 0x00;   // Ext Function set 0000 1 FW BW NW (Ext Regs)
                                    //   NW=0  (1,2 line), NW=1 (4 Line, special mode for KS0078)                                
              break;
          } // switch type

          // init special features
          _writeCommand(0x20 | _function_1);// Function set 001 DL N RE(1) BE 0 (Ext Regs)
                                           //   DL=0 (4 bits bus), DL=1 (8 bits mode)            
                                           //    N=0 (1 line mode), N=1 (2 line mode)
                                           //   RE=1 (Ena Extended Regs, special mode for KS0078)
                                           //   BE=0 (Blink Enable/Disable, CG/SEG RAM, special mode for KS0078)                                
                                           //      0 

          _writeCommand(0x08 | _function_x); // Ext Function set 0000 1 FW BW NW (Ext Regs)
                                           //   FW=0  (5-dot font, special mode for KS0078)
                                           //   BW=0  (Cur BW invert disable, special mode for KS0078)
                                           //   NW=0  (1,2 Line), NW=1 (4 line, special mode for KS0078)                                

          _writeCommand(0x10);             // Scroll/Shift set 0001 DS/HS4 DS/HS3 DS/HS2 DS/HS1 (Ext Regs)
                                           //   Dotscroll/Display shift enable (Special mode for KS0078)

          _writeCommand(0x80);             // Scroll Quantity set 1 0 SQ5 SQ4 SQ3 SQ2 SQ1 SQ0 (Ext Regs)
                                           //   Scroll quantity (Special mode for KS0078)

          _writeCommand(0x20 | _function); // Function set 001 DL N RE(0) DH REV (Std Regs)
                                           //   DL=0  (4 bits bus), DL=1 (8 bits mode)             
                                           //    N=0  (1 line mode), N=1 (2 line mode)
                                           //   RE=0  (Dis. Extended Regs, special mode for KS0078)
                                           //   DH=1  (Disp shift enable/disable, special mode for KS0078)                                
                                           //   REV=0 (Reverse/Normal, special mode for KS0078)
          break; // case KS0078 Controller
              
      case ST7032_3V3:
          // ST7032 controller: Initialise Voltage booster for VLCD. VDD=3V3
      case ST7032_5V:
          // ST7032 controller: Disable Voltage booster for VLCD. VDD=5V

          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2B is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:                                            
//            case LCD20x1:                    
            case LCD24x1:
              _function = 0x00;       // FUNCTION SET 0 0 1 DL=0 (4 bit), N=0 (1-line display mode), F=0 (5*7dot), 0, IS
                                      // Note: 4 bit mode is ignored for native SPI and I2C devices
                                      // Saved to allow switch between Instruction sets at later time
              break;  

            case LCD12x3D:            // Special mode for KS0078 and PCF21XX
            case LCD12x3D1:           // Special mode for KS0078 and PCF21XX
            case LCD12x4D:            // Special mode for KS0078 and PCF21XX
            case LCD16x3G:            // Special mode for ST7036                        
            case LCD24x4D:            // Special mode for KS0078
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  

            default:
              // All other LCD types are initialised as 2 Line displays        
              _function = 0x08;       // FUNCTION SET 0 0 1 DL=0 (4 bit), N=1 (2-line display mode), F=0 (5*7dot), 0, IS              
                                      // Note: 4 bit mode is ignored for native SPI and I2C devices
                                      // Saved to allow switch between Instruction sets at later time
              break;                                                                        
          } // switch type    
                                     
          // init special features 
          _writeCommand(0x20 | _function | 0x01);           // Set function,  0 0 1 DL N F 0 IS=1 Select Instr Set = 1              

          _writeCommand(0x1C);                              // Internal OSC frequency adjustment Framefreq=183HZ, Bias will be 1/4 (Instr Set=1)

          _contrast = LCD_ST7032_CONTRAST;              
          _writeCommand(0x70 | (_contrast & 0x0F));         // Set Contrast Low bits, 0 1 1 1 C3 C2 C1 C0 (IS=1)


          if (_ctrl == ST7032_3V3) {
            _icon_power = 0x04;                             // Icon display off, Booster circuit is turned on  (IS=1)
                                                            // Saved to allow contrast change at later time
          }
          else { 
            _icon_power = 0x00;                             // Icon display off, Booster circuit is turned off  (IS=1)
                                                            // Saved to allow contrast change at later time
          }
          _writeCommand(0x50 | _icon_power | ((_contrast >> 4) & 0x03));  // Set Icon, Booster and Contrast High bits, 0 1 0 1 Ion Bon C5 C4 (IS=1)
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up
          
          _writeCommand(0x68 | (LCD_ST7032_RAB & 0x07));      // Voltage follower, 0 1 1 0 FOn=1, Ampl ratio Rab2=1, Rab1=0, Rab0=0  (IS=1)
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up
          
          _writeCommand(0x20 | _function);                  // Select Instruction Set = 0

          break; // case ST7032_3V3 Controller
                 // case ST7032_5V Controller

      case ST7036_3V3:
          // ST7036 controller: Initialise Voltage booster for VLCD. VDD=3V3
          // Note: supports 1,2 (LCD_T_A) or 3 lines (LCD_T_G)
      case ST7036_5V:
          // ST7036 controller: Disable Voltage booster for VLCD. VDD=5V
          // Note: supports 1,2 (LCD_T_A) or 3 lines (LCD_T_G)
                    
          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2D is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:   
            case LCD24x1:                                                                         
              _function = 0x00;     // Set function, 0 0 1 DL=0 (4-bit Databus), N=0 (1 Line), DH=0 (5x7font), IS2, IS1 (Select Instruction Set)
                                    // Note: 4 bit mode is ignored for native SPI and I2C devices
                                    // Saved to allow switch between Instruction sets at later time
              
              _bias_lines = 0x04;   // Bias: 1/5, 1 or 2-Lines LCD 
              break;  

//            case LCD12x3G:          // Special mode for ST7036
            case LCD16x3G:          // Special mode for ST7036
              _function = 0x08;     // Set function, 0 0 1 DL=0 (4-bit Databus), N=1 (2 Line), DH=0 (5x7font), IS2,IS1 (Select Instruction Set)              
                                    // Note: 4 bit mode is ignored for native SPI and I2C devices
                                    // Saved to allow switch between Instruction sets at later time
              
              _bias_lines = 0x05;   // Bias: 1/5, 3-Lines LCD           
              break;  

//            case LCD12x3D1:           // Special mode for KS0078 and PCF21XX
//            case LCD16x3D1:           // Special mode for SSD1803
            case LCD12x4D:            // Special mode for PCF2116
            case LCD24x4D:            // Special mode for KS0078
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  

            default:
              // All other LCD types are initialised as 2 Line displays (including LCD16x1C and LCD40x4)       
              _function = 0x08;     // Set function, 0 0 1 DL=0 (4-bit Databus), N=1 (2 Line), DH=0 (5x7font), IS2,IS1 (Select Instruction Set)
                                    // Note: 4 bit mode is ignored for native SPI and I2C devices
                                    // Saved to allow switch between Instruction sets at later time
              
              _bias_lines = 0x04;   // Bias: 1/5, 1 or 2-Lines LCD 
              break;                
          } // switch type


          // init special features 
          _writeCommand(0x20 | _function | 0x01);   // Set function, IS2,IS1 = 01 (Select Instr Set = 1)
          _writeCommand(0x10 | _bias_lines);        // Set Bias and 1,2 or 3 lines (Instr Set 1)

          _contrast = LCD_ST7036_CONTRAST;
          _writeCommand(0x70 | (_contrast & 0x0F)); // Set Contrast, 0 1 1 1 C3 C2 C1 C0 (Instr Set 1)
                           
          if (_ctrl == ST7036_3V3) {
            _icon_power = 0x04;                       // Set Icon, Booster, Contrast High bits, 0 1 0 1 Ion=0 Bon=1 C5 C4 (Instr Set 1)
                                                      // Saved to allow contrast change at later time
          } 
          else {
            _icon_power = 0x00;                       // Set Icon, Booster, Contrast High bits, 0 1 0 1 Ion=0 Bon=0 C5 C4 (Instr Set 1)             
          }
          
          _writeCommand(0x50 | _icon_power | ((_contrast >> 4) & 0x03));   // Set Contrast C5, C4 (Instr Set 1)
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up

          _writeCommand(0x68 | (LCD_ST7036_RAB & 0x07));  // Voltagefollower On = 1, Ampl ratio Rab2, Rab1, Rab0 = 1 0 1 (Instr Set 1)
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up

          _writeCommand(0x20 | _function);          // Set function, IS2,IS1 = 00 (Select Instruction Set = 0)
         
          break; // case ST7036_3V3 Controller
                 // case ST7036_5V Controller
        
      case SSD1803_3V3:
          // SSD1803 controller: Initialise Voltage booster for VLCD. VDD=3V3
          // Note: supports 1,2, 3 or 4 lines
//      case SSD1803_5V:
          // SSD1803 controller: No Voltage booster for VLCD. VDD=5V
                    
          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2D is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:   
            case LCD24x1:                                                                         
              _function = 0x00;     //  Set function 0 0 1 DL N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=0 1 Line / 3 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x02;   // Set function, 0 0 1 DL N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=0 1 Line / 3 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803
                                    //   REV=0 Reverse off, special feature of SSD1803            
                        
              _lines = 0x00;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=0 1-Line LCD (N=0)
              break;  

            case LCD12x3D:          // Special mode for KS0078 and PCF21XX                                  
//            case LCD12x3D1:           // Special mode for KS0078 and PCF21XX            
            case LCD16x3D:          // Special mode for KS0078
//            case LCD16x3D1:           // Special mode for SSD1803
//            case LCD20x3D:            // Special mode for SSD1803
              _function = 0x00;     //  Set function 0 0 1 DL N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=0 1 Line / 3 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x02;   // Set function, 0 0 1 DL N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=0 1 Line / 3 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803
                                    //   REV=0 Reverse off, special feature of SSD1803            
                        
              _lines = 0x00;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=1 3-Line LCD (N=0)
              break;  

            case LCD20x4D:          // Special mode for SSD1803
              _function = 0x08;     //  Set function 0 0 1 DL N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=1 4 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x0A;   // Set function, 0 0 1 DL N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=1 4 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803
                                    //   REV=0 Reverse off, special feature of SSD1803            
                        
              _lines = 0x01;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=1 4-Line LCD (N=1)
              break;  

            case LCD16x3G:          // Special mode for ST7036            
            case LCD24x4D:          // Special mode for KS0078
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  

            default:
              // All other LCD types are initialised as 2 Line displays (including LCD16x1C and LCD40x4)       
              _function = 0x08;     //  Set function 0 0 1 DL N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=1 2 line / 4 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x0A;   // Set function, 0 0 1 DL N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 4-bit Databus,
                                    //         Note: 4 bit mode is ignored for native SPI and I2C devices
                                    //     N=1 2 line / 4 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803
                                    //   REV=0 Reverse off, special feature of SSD1803            
                        
              _lines = 0x00;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=0 2-Line LCD (N=1)
              break;                
          } // switch type


          // init special features 
          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 DL N BE RE(1) REV 
                                                    // Select Extended Instruction Set
          
          _writeCommand(0x06);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=1 COM1-32, BDS=0 SEG100-1    "Bottom View" (Ext Instr Set)
//          _writeCommand(0x05);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=0 COM32-1, BDS=1 SEG1-100    "Top View" (Ext Instr Set)          
          ThisThread::sleep_for(5ms);                               // Wait to ensure completion or SSD1803 fails to set Top/Bottom after reset..
         
          _writeCommand(0x08 | _lines);             // Set ext function 0 0 0 0 1 FW BW NW 1,2,3 or 4 lines (Ext Instr Set)

          _writeCommand(0x10);                      // Double Height and Bias, 0 0 0 1 UD2=0, UD1=0, BS1=0 Bias 1/5, DH=0 (Ext Instr Set)

//          _writeCommand(0x76);                      // Set TC Control, 0 1 1 1 0 1 1 0 (Ext Instr Set)
//          _writeData(0x02);                         // Set TC data,    0 0 0 0 0 TC2,TC1,TC0 = 0 1 0 (Ext Instr Set)

          _writeCommand(0x20 | _function | 0x01);   // Set function, 0 0 1 DL N DH RE(0) IS=1 Select Instruction Set 1
                                                    // Select Std Instr set, Select IS=1  

          _contrast = LCD_SSD1_CONTRAST;
          _writeCommand(0x70 | (_contrast & 0x0F)); // Set Contrast 0 1 1 1 C3, C2, C1, C0 (Instr Set 1)
                           
          _icon_power = 0x04;                       // Icon off, Booster on (Instr Set 1)
                                                    // Saved to allow contrast change at later time
          _writeCommand(0x50 | _icon_power | ((_contrast >> 4) & 0x03));   // Set Power, Icon and Contrast, 0 1 0 1 Ion Bon C5 C4 (Instr Set 1)
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up

          _writeCommand(0x68 | (LCD_SSD1_RAB & 0x07));  // Set Voltagefollower 0 1 1 0 Don = 1, Ampl ratio Rab2, Rab1, Rab0 = 1 1 0  (Instr Set 1)
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up

          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 DL N BE RE(1) REV 
                                                    // Select Extended Instruction Set 1
          _writeCommand(0x10);                      // Shift/Scroll enable, 0 0 0 1 DS4/HS4 DS3/HS3 DS2/HS2 DS1/HS1  (Ext Instr Set 1)


          _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                    // Select Std Instr set, Select IS=0
         
          break; // case SSD1803 Controller

          
      // Note1: The PCF21XX family of controllers has several types that dont have an onboard voltage generator for V-LCD.
      //        You must supply this LCD voltage externally and not try to enable VGen. 
      // Note2: The early versions of PCF2116 controllers (eg PCF2116C) can not generate sufficiently negative voltage for the LCD at a VDD of 3V3. 
      //        You must supply this voltage externally and not enable VGen or you must use a higher VDD (e.g. 5V) and enable VGen.
      //        More recent versions of the controller (eg PCF2116K) have an improved VGen that will work with 3V3.
      // Note3: See datasheet, PCF2116 and other types provide a V0 pin to control the LCD contrast voltage that is provided by VGen. This pins allows 
      //        contrast control similar to that of pin 3 on the standard 14pin LCD module connector.
      //        You can disable VGen by connecting Vo to VDD. VLCD will then be used directly as LCD voltage.
      // Note4: PCF2113 and PCF2119 are different wrt to VLCD generator! There is no V0 pin. The contrast voltage is software controlled by setting the VA and VB registers.
      //        Vgen is automatically switched off when the contrast voltage VA or VB is set to 0x00. Note that certain limits apply to allowed values for VA and VB.                    
      // Note5: See datasheet, members of the PCF21XX family support different numbers of rows/columns. Not all can support 3 or 4 rows.
      // Note6: See datasheet, the PCF21XX-C and PCF21XX-K use a non-standard character set. This may result is strange looking text when not corrected..

      case PCF2113_3V3:
          // PCF2113 controller: Initialise Voltage booster for VLCD. VDD=3V3. VA and VB control contrast.
          // Initialise Display configuration
          switch (_type) {
//            case LCD12x1:                                
//              _function = 0x02;       // FUNCTION SET 0 0 1 DL=0 4 bit, 0, M=0 1-line/12 chars display mode, SL=1, IS=0
                                      // Note: 4 bit mode is ignored for I2C mode
            case LCD24x1:                    
              _function = 0x00;       // FUNCTION SET 0 0 1 DL=0 4 bit, 0, M=0 1-line/24 chars display mode, SL=0, IS=0            
                                      // Note: 4 bit mode is ignored for I2C mode
              break;  

//Tested OK for PCF2113
            case LCD12x2:                    
              _function = 0x04;       // FUNCTION SET 0 0 1 DL=0 4 bit, 0, M=1 2-line/12 chars display mode, SL=0, IS=0            
              break;  
             
            default:
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  
                         
          } // switch type    

          // Init special features
          _writeCommand(0x20 | _function | 0x01);          // Set function, Select Instr Set = 1              

          _writeCommand(0x04);                             // Display Conf Set         0000 0, 1, P=0, Q=0               (Instr. Set 1)
          _writeCommand(0x10);                             // Temp Compensation Set    0001 0, 0, TC1=0, TC2=0           (Instr. Set 1)
//          _writeCommand(0x42);                             // HV GEN                   0100 S1=1, S2=0 (2x multiplier)   (Instr. Set 1)
          _writeCommand(0x40 | (LCD_PCF2_S12 & 0x03));     // HV Gen                   0100 S1=1, S2=0 (2x multiplier)   (Instr. Set 1)
          
          _contrast = LCD_PCF2_CONTRAST;              
          _writeCommand(0x80 | 0x00 | (_contrast & 0x3F));      // VLCD_set (Instr. Set 1)  1, V=0, VA=contrast
          _writeCommand(0x80 | 0x40 | (_contrast & 0x3F));      // VLCD_set (Instr. Set 1)  1, V=1, VB=contrast
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up
          
          _writeCommand(0x02);                             // Screen Config            0000 001, L=0  (Instr. Set 1)
          _writeCommand(0x08);                             // ICON Conf                0000 1, IM=0 (Char mode), IB=0 (no icon blink) DM=0 (no direct mode) (Instr. Set 1) 

          _writeCommand(0x20 | _function);                 // Set function, Select Instr Set = 0             

          break; // case PCF2113_3V3 Controller


//      case PCF2113_5V:
          // PCF2113 controller: No Voltage generator for VLCD. VDD=5V. Contrast voltage controlled by VA or VB.
//@TODO                            


      case PCF2116_3V3:
          // PCF2116 controller: Voltage generator for VLCD. VDD=5V. V0 controls contrast voltage.                 
          // Initialise Display configuration
          switch (_type) {
//            case LCD12x1:
//            case LCD12x2:                                                                            
            case LCD24x1:                    
              _writeCommand(0x22);    //FUNCTION SET 4 bit, N/M=0 1-line/24 chars display mode, G=1 Vgen on 
                                      //Note: 4 bit mode is ignored for I2C mode
              ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up                                                    
              break;  

            case LCD12x3D:            // Special mode for KS0078 and PCF21XX                            
            case LCD12x3D1:           // Special mode for PCF21XX                     
            case LCD12x4D:            // Special mode for PCF21XX:
              _writeCommand(0x2E);    //FUNCTION SET 4 bit, N=1/M=1 4-line/12 chars display mode, G=1 VGen on                               
                                      //Note: 4 bit mode is ignored for I2C mode              
              ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up                                                    
              break;  

            case LCD24x2:
              _writeCommand(0x2A);    //FUNCTION SET 4 bit, N=1/M=0 2-line/24 chars display mode, G=1 VGen on
                                      //Note: 4 bit mode is ignored for I2C mode
              ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up   
              break;  
              
            default:
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  
            
          } // switch type    

          break; // case PCF2116_3V3 Controller


//Experimental for cellphone 3-line display, SA=0x74, No Ack supported, Character set C or K, DL = 8 bit, N=0,M=1 (reserved mode !!), external VLCD -2V5
//@TODO                            
      case PCF2116_5V:
          // PCF2116 controller: No Voltage generator for VLCD. VDD=5V. V0 controls contrast voltage.                           
          // Initialise Display configuration
          switch (_type) {
//            case LCD12x1:
//            case LCD12x2:                                                                            
//            case LCD24x1:                    
//              _writeCommand(0x20);    //FUNCTION SET 4 bit, N/M=0 1-line/24 chars display mode 
                                      //Note: 4 bit mode is ignored for I2C mode
//              ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up                                                    
//              break;  

            case LCD12x3D:            // Special mode for KS0078 and PCF21XX                            
            case LCD12x3D1:           // Special mode for PCF21XX                     
            case LCD12x4D:            // Special mode for PCF21XX:
//              _writeCommand(0x34);    //FUNCTION SET 8 bit, N=0/M=1 4-line/12 chars display mode      OK
//              _writeCommand(0x24);    //FUNCTION SET 4 bit, N=0/M=1 4-line/12 chars display mode      OK                                            
              _writeCommand(0x2C);    //FUNCTION SET 4 bit, N=1/M=1 4-line/12 chars display mode        OK                                                   
                                      //Note: 4 bit mode is ignored for I2C mode              
              ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up                                                    
              break;  

//            case LCD24x2:
//              _writeCommand(0x28);    //FUNCTION SET 4 bit, N=1/M=0 2-line/24 chars display mode
                                      //Note: 4 bit mode is ignored for I2C mode
//              ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up   
//              break;  
              
            default:
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  
            
          } // switch type    

          break; // case PCF2116_5V Controller

      case PCF2119_3V3:
          // PCF2119 controller: Initialise Voltage booster for VLCD. VDD=3V3. VA and VB control contrast.
          // Note1: See datasheet, the PCF2119 supports icons and provides separate constrast control for Icons and characters.
          // Note2: Vgen is switched off when the contrast voltage VA or VB is set to 0x00.
                  
//POR or Hardware Reset should be applied
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up   

          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:
//            case LCD12x1:
            case LCD16x1:           
              _function = 0x02;       // FUNCTION SET DL=0 4 bit, 0 , M=0 1-line/16 chars display mode, SL=1
                                      // Note: 4 bit mode is ignored for I2C mode
              break;  
            
            case LCD24x1:                    
//            case LCD32x1:                                
              _function = 0x00;       // FUNCTION SET DL=0 4 bit, 0 , M=0 1-line/32 chars display mode, SL=0
                                      // Note: 4 bit mode is ignored for I2C mode
              break;  

            case LCD8x2:
//            case LCD12x2:            
            case LCD16x2:
              _function = 0x04;       // FUNCTION SET DL=0 4 bit, 0, M=1 2-line/16 chars display mode, SL=0
                                      // Note: 4 bit mode is ignored for I2C mode
              break;  
             
            default:
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  
            
          } // switch type    

          // Init special features 
          _writeCommand(0x20 | _function | 0x01);           // Set function, Select Instruction Set = 1              

          _writeCommand(0x04);    // DISP CONF SET (Instr. Set 1)   0000, 0, 1, P=0, Q=0 
          _writeCommand(0x10);    // TEMP CTRL SET (Instr. Set 1)   0001, 0, 0, TC1=0, TC2=0
//          _writeCommand(0x42);    // HV GEN (Instr. Set 1)          0100, 0, 0, S1=1, S2=0 (2x multiplier)
          _writeCommand(0x40 | (LCD_PCF2_S12 & 0x03));      // HV GEN (Instr. Set 1)          0100, 0, 0, S1=1, S2=0 (2x multiplier)

          _contrast = LCD_PCF2_CONTRAST;              
          _writeCommand(0x80 | 0x00 | (_contrast & 0x3F));      // VLCD_set (Instr. Set 1)    V=0, VA=contrast
          _writeCommand(0x80 | 0x40 | (_contrast & 0x3F));      // VLCD_set (Instr. Set 1)    V=1, VB=contrast
          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up
          
          _writeCommand(0x02);    // SCRN CONF (Instr. Set 1)    L=0
          _writeCommand(0x08);    // ICON CONF (Instr. Set 1)    IM=0 (Char mode) IB=0 (no icon blink) DM=0 (no direct mode)

          _writeCommand(0x20 | _function);                  // Select Instruction Set = 0

          break; // case PCF2119_3V3 Controller

//      case PCF2119_5V:
          // PCF2119 controller: No Voltage booster for VLCD. VDD=3V3. VA and VB control contrast.
          // Note1: See datasheet, the PCF2119 supports icons and provides separate constrast control for Icons and characters.
          // Note2: Vgen is switched off when the contrast voltage VA or VB is set to 0x00.                     
//@TODO                            

      case WS0010:         
          // WS0010 OLED controller: Initialise DC/DC Voltage converter for LEDs
          // Note1: Identical to RS0010  
          // Note2: supports 1 or 2 lines (and 16x100 graphics)
          //        supports 4 fonts (English/Japanese (default), Western European-I, English/Russian, Western European-II)
                           // Cursor/Disp shift set 0001 SC RL  0 0
                           //
                           // Mode and Power set    0001 GC PWR 1 1                           
                           //  GC  = 0 (Graph Mode=1, Char Mode=0)             
                           //  PWR = 1 (DC/DC On/Off)
   
//@Todo: This may be needed to enable a warm reboot
          //_writeCommand(0x13);   // Char mode, DC/DC off              
          //ThisThread::sleep_for(10ms);           // Wait 10ms to ensure powered down                  
          _writeCommand(0x17);   // Char mode, DC/DC on        
          ThisThread::sleep_for(10ms);           // Wait 10ms to ensure powered up        

          // Initialise Display configuration
          switch (_type) {                    
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2B is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:                                            
            case LCD24x1:
              _writeCommand(0x20); // Function set 001 DL N F FT1 FT0
                                   //  DL=0  (4 bits bus)             
                                   //   N=0  (1 line)
                                   //   F=0  (5x7 dots font)
                                   //  FT=00 (00 = Engl/Jap, 01 = WestEur1, 10 = Engl/Russian, 11 = WestEur2
              break;  

            case LCD12x3D:            // Special mode for KS0078 and PCF21XX                            
            case LCD12x3D1:           // Special mode for PCF21XX                     
            case LCD12x4D:            // Special mode for PCF21XX:
            case LCD16x3G:            // Special mode for ST7036            
            case LCD24x4D:            // Special mode for KS0078
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  

            default:
              // All other LCD types are initialised as 2 Line displays (including LCD16x1C and LCD40x4)       
              _writeCommand(0x28); // Function set 001 DL N F FT1 FT0
                                   //  DL=0  (4 bits bus)
                                   //   N=1  (2 lines)
                                   //   F=0  (5x7 dots font)
                                   //  FT=00 (00 = Engl/Jap, 01 = WestEur1, 10 = Engl/Russian, 11 = WestEur2

              break;
           } // switch type
           
           break; // case WS0010 Controller


      case US2066_3V3:
          // US2066/SSD1311 OLED controller, Initialise for VDD=3V3
          // Note: supports 1,2, 3 or 4 lines
//      case USS2066_5V:
          // US2066 controller, VDD=5V
                    
          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2D is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:   
//            case LCD20x1:                                                                         
              _function = 0x00;     //  Set function 0 0 1 X N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead
                                    //     N=0 1 Line / 3 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x02;   // Set function, 0 0 1 X N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead                                    
                                    //     N=0 1 Line / 3 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803, US2066
                                    //   REV=0 Reverse off, special feature of SSD1803, US2066            
                        
              _lines = 0x00;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=0 1-Line LCD (N=0)
              break;  

            case LCD16x1C:
            case LCD8x2:
            case LCD16x2:
            case LCD20x2:            
              _function = 0x08;     //  Set function 0 0 1 X N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead                                                                        
                                    //     N=1 2 line / 4 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x0A;   // Set function, 0 0 1 X N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead                                                                        
                                    //     N=1 2 line / 4 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803, US2066
                                    //   REV=0 Reverse off, special feature of SSD1803, US2066            
                        
              _lines = 0x00;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=0 2-Line LCD (N=1)
              break;                

            case LCD12x3D:          // Special mode for KS0078 and PCF21XX 
//            case LCD12x3D1:           // Special mode for KS0078 and PCF21XX            
            case LCD16x3D:          // Special mode for KS0078, SSD1803 and US2066
//            case LCD16x3D1:           // Special mode for SSD1803, US2066
//            case LCD20x3D:            // Special mode for SSD1803, US2066
              _function = 0x00;     //  Set function 0 0 1 X N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead                                    
                                    //     N=0 1 Line / 3 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x02;   // Set function, 0 0 1 X N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead                                    
                                    //     N=0 1 Line / 3 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803, US2066
                                    //   REV=0 Reverse off, special feature of SSD1803, US2066            
                        
              _lines = 0x00;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=1 3-Line LCD (N=0)
              break;  

            case LCD20x4D:          // Special mode for SSD1803, US2066
              _function = 0x08;     //  Set function 0 0 1 X N DH RE(0) IS 
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=X bit is ignored for US2066. Uses hardwired pins instead
                                    //     N=1 2 line / 4 Line
                                    //    DH=0 Double Height disable 
                                    //    IS=0
          
              _function_1 = 0x0A;   // Set function, 0 0 1 DL N BE RE(1) REV
                                    //  Saved to allow switch between Instruction sets at later time
                                    //    DL=0 bit is ignored for US2066. Uses hardwired pins instead                                    
                                    //     N=1 2 line / 4 Line
                                    //    BE=0 Blink Enable off, special feature of SSD1803, US2066
                                    //   REV=0 Reverse off, special feature of SSD1803, US2066            
                        
              _lines = 0x01;        // Ext function set 0 0 0 0 1 FW BW NW 
                                    //    NW=1 4-Line LCD (N=1)
              break;  

//            case LCD24x1:                                                                         
//            case LCD16x3G:          // Special mode for ST7036            
//            case LCD24x4D:          // Special mode for KS0078
            default:            
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  

          } // switch type


          // init special features 
          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 X N BE RE(1) REV 
                                                    // Select Extended Instruction Set

          _writeCommand(0x71);                      // Function Select A: 0 1 1 1 0 0 0 1 (Ext Instr Set)
          _writeData(0x00);                         // Disable Internal VDD

          _writeCommand(0x79);                      // Function Select OLED:  0 1 1 1 1 0 0 1 (Ext Instr Set)

          _writeCommand(0xD5);                      // Display Clock Divide Ratio: 1 1 0 1 0 1 0 1 (Ext Instr Set, OLED Instr Set)
          _writeCommand(0x70);                      // Display Clock Divide Ratio value: 0 1 1 1 0 0 0 0 (Ext Instr Set, OLED Instr Set)
                    
          _writeCommand(0x78);                      // Function Disable OLED: 0 1 1 1 1 0 0 0 (Ext Instr Set)
          
//          _writeCommand(0x06);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=1 COM1-32, BDS=0 SEG100-1    "Bottom View" (Ext Instr Set)
          _writeCommand(0x05);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=0 COM32-1, BDS=1 SEG1-100    "Top View" (Ext Instr Set)          
         
          _writeCommand(0x08 | _lines);             // Set ext function 0 0 0 0 1 FW BW NW 1,2,3 or 4 lines (Ext Instr Set)

//          _writeCommand(0x1C);                      // Double Height and Bias, 0 0 0 1 UD2=1, UD1=1, X, DH=0 (Ext Instr Set)
//                                                    // Default

          _writeCommand(0x72);                      // Function Select B: 0 1 1 1 0 0 1 0 (Ext Instr Set)
          _writeData(0x01);                         // Select ROM A (CGRAM 8, CGROM 248)

          _writeCommand(0x79);                      // Function Select OLED:  0 1 1 1 1 0 0 1 (Ext Instr Set)

          _writeCommand(0xDA);                      // Set Segm Pins Config:  1 1 0 1 1 0 1 0 (Ext Instr Set, OLED)
          _writeCommand(0x10);                      // Set Segm Pins Config value: Altern Odd/Even, Disable Remap (Ext Instr Set, OLED)

          _writeCommand(0xDC);                      // Function Select C: 1 1 0 1 1 1 0 0 (Ext Instr Set, OLED)
//          _writeCommand(0x00);                      // Set internal VSL, GPIO pin HiZ (always read low)
          _writeCommand(0x80);                      // Set external VSL, GPIO pin HiZ (always read low)

          _contrast = LCD_US20_CONTRAST;
          _writeCommand(0x81);                      // Set Contrast Control: 1 0 0 0 0 0 0 1 (Ext Instr Set, OLED)
          _writeCommand((_contrast << 2) | 0x03);   // Set Contrast Value: 8 bits, use 6 bits for compatibility 

          _writeCommand(0xD9);                      // Set Phase Length: 1 1 0 1 1 0 0 1 (Ext Instr Set, OLED)
          _writeCommand(0xF1);                      // Set Phase Length Value: 

          _writeCommand(0xDB);                      // Set VCOMH Deselect Lvl: 1 1 0 1 1 0 1 1 (Ext Instr Set, OLED)
          _writeCommand(0x30);                      // Set VCOMH Deselect Value: 0.83 x VCC

          ThisThread::sleep_for(10ms);            // Wait 10ms to ensure powered up

//Test Fade/Blinking. Hard Blink on/off, No fade in/out ??
//          _writeCommand(0x23);                      // Set (Ext Instr Set, OLED)
//          _writeCommand(0x3F);                      // Set interval 128 frames
//End Test Blinking

          _writeCommand(0x78);                      // Function Disable OLED: 0 1 1 1 1 0 0 0 (Ext Instr Set)
          
          _writeCommand(0x20 | _function | 0x01);   // Set function, 0 0 1 X N DH RE(0) IS=1 Select Instruction Set 1
                                                    // Select Std Instr set, Select IS=1  

          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 X N BE RE(1) REV 
                                                    // Select Ext Instr Set, IS=1
          _writeCommand(0x10);                      // Shift/Scroll enable, 0 0 0 1 DS4/HS4 DS3/HS3 DS2/HS2 DS1/HS1  (Ext Instr Set, IS=1)


          _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                    // Select Std Instr set, Select IS=0
         
          break; // case US2066/SSD1311 Controller

           
        default:
          // Devices fully compatible to HD44780 that do not use any DC/DC Voltage converters but external VLCD, no icons etc

          // Initialise Display configuration
          switch (_type) {
            case LCD8x1:         //8x1 is a regular 1 line display
            case LCD8x2B:        //8x2B is a special case of 16x1
//            case LCD12x1:                                
            case LCD16x1:                                            
//            case LCD20x1:                                                        
            case LCD24x1:
//            case LCD40x1:            
              _function = 0x00;    // Function set 001 DL N F - -
                                   //  DL=0 (4 bits bus)             
                                   //   N=0 (1 line)
                                   //   F=0 (5x7 dots font)
              _writeCommand(0x20 | _function);             
              break;                                
                                                  
            case LCD12x3D:            // Special mode for KS0078 and PCF21XX                            
            case LCD12x3D1:           // Special mode for KS0078 and PCF21XX                     
            case LCD12x4D:            // Special mode for KS0078 and PCF21XX:
            case LCD16x3D:            // Special mode for KS0078
//            case LCD16x3D1:           // Special mode for KS0078
//            case LCD24x3D:            // Special mode for KS0078
//            case LCD24x3D1:           // Special mode for KS0078            
            case LCD24x4D:            // Special mode for KS0078
              error("Error: LCD Controller type does not support this Display type\n\r"); 
              break;  

            // All other LCD types are initialised as 2 Line displays (including LCD16x1C and LCD40x4)
            default:
              _function = 0x08;    // Function set 001 DL N F - -
                                   //  DL=0 (4 bits bus)
                                   //  Note: 4 bit mode is ignored for native SPI and I2C devices                                 
                                   //   N=1 (2 lines)
                                   //   F=0 (5x7 dots font, only option for 2 line display)
                                   //    -  (Don't care)
              _writeCommand(0x20 | _function);                         
              break;
          } // switch type

          break; // case default Controller
          
    } // switch Controller specific initialisations 
    

    // Controller general initialisations                                          
//    _writeCommand(0x01); // cls, and set cursor to 0
//    ThisThread::sleep_for(10ms);         // The CLS command takes 1.64 ms.
//                         // Since we are not using the Busy flag, Lets be safe and take 10 ms  

    _writeCommand(0x02); // Return Home 
                         //   Cursor Home, DDRAM Address to Origin

    _writeCommand(0x06); // Entry Mode 0000 0 1 I/D S 
                         //   Cursor Direction and Display Shift
                         //   I/D=1 (Cur incr)
                         //     S=0 (No display shift)                        

    _writeCommand(0x14); // Cursor or Display shift 0001 S/C R/L x x 
                         //   S/C=0 Cursor moves
                         //   R/L=1 Right
                         // 

//    _writeCommand(0x0C); // Display Ctrl 0000 1 D C B
//                         //   Display On, Cursor Off, Blink Off   
    setCursor(CurOff_BlkOff);     
    setMode(DispOn);     
}


/** Clear the screen, Cursor home. 
  */
void TextLCD_Base::cls() {

  // Select and configure second LCD controller when needed
  if(_type==LCD40x4) {
    _ctrl_idx=_LCDCtrl_1; // Select 2nd controller

    // Second LCD controller Cursor always Off
    _setCursorAndDisplayMode(_currentMode, CurOff_BlkOff);

    // Second LCD controller Clearscreen
    _writeCommand(0x01);  // cls, and set cursor to 0    
    ThisThread::sleep_for(10ms);          // The CLS command takes 1.64 ms.
                          // Since we are not using the Busy flag, Lets be safe and take 10 ms
  
    _ctrl_idx=_LCDCtrl_0; // Select primary controller
  }
  
  // Primary LCD controller Clearscreen
  _writeCommand(0x01);    // cls, and set cursor to 0
  ThisThread::sleep_for(10ms);            // The CLS command takes 1.64 ms.
                          // Since we are not using the Busy flag, Lets be safe and take 10 ms

  // Restore cursormode on primary LCD controller when needed
  if(_type==LCD40x4) {
    _setCursorAndDisplayMode(_currentMode,_currentCursor);     
  }
                   
  setAddress(0, 0);  // Reset Cursor location
                     // Note: This is needed because some displays (eg PCF21XX) don't use line 0 in the '3 Line' mode.   
}

/** Locate cursor to a screen column and row
  *
  * @param column  The horizontal position from the left, indexed from 0
  * @param row     The vertical position from the top, indexed from 0
  */ 
void TextLCD_Base::locate(int column, int row) {
    
   // setAddress() does all the heavy lifting:
   //   check column and row sanity, 
   //   switch controllers for LCD40x4 if needed
   //   switch cursor for LCD40x4 if needed
   //   set the new memory address to show cursor at correct location
   setAddress(column, row);      
}
   

/** Write a single character (Stream implementation)
  */
int TextLCD_Base::_putc(int value) {
  int addr;
    
    if (value == '\n') {
      //No character to write
      
      //Update Cursor      
      _column = 0;
      _row++;
      if (_row >= rows()) {
        _row = 0;
      }      
    }
    else {
      //Character to write      
      _writeData(value); 
              
      //Update Cursor
      _column++;
      if (_column >= columns()) {
        _column = 0;
        _row++;
        if (_row >= rows()) {
          _row = 0;
        }
      }          
    } //else

    //Set next memoryaddress, make sure cursor blinks at next location
    addr = getAddress(_column, _row);
    _writeCommand(0x80 | addr);
            
    return value;
}


// get a single character (Stream implementation)
int TextLCD_Base::_getc() {
    return -1;
}


// Write a nibble using the 4-bit interface
void TextLCD_Base::_writeNibble(int value) {

// Enable is Low
    this->_setEnable(true);        
    this->_setData(value & 0x0F);   // Low nibble
    wait_us(1); // Data setup time        
    this->_setEnable(false);    
    wait_us(1); // Datahold time

// Enable is Low
}

// Write a byte using the 4-bit interface
void TextLCD_Base::_writeByte(int value) {

// Enable is Low
    this->_setEnable(true);          
    this->_setData(value >> 4);   // High nibble
    wait_us(1); // Data setup time    
    this->_setEnable(false);   
    wait_us(1); // Data hold time
    
    this->_setEnable(true);        
    this->_setData(value >> 0);   // Low nibble
    wait_us(1); // Data setup time        
    this->_setEnable(false);    
    wait_us(1); // Datahold time

// Enable is Low
}

// Write a command byte to the LCD controller
void TextLCD_Base::_writeCommand(int command) {

    this->_setRS(false);        
    wait_us(1);  // Data setup time for RS       
    
    this->_writeByte(command);   
    wait_us(40); // most instructions take 40us            
}

// Write a data byte to the LCD controller
void TextLCD_Base::_writeData(int data) {

    this->_setRS(true);            
    wait_us(1);  // Data setup time for RS 
        
    this->_writeByte(data);
    wait_us(40); // data writes take 40us                
}


// This replaces the original _address() method.
// It is confusing since it returns the memoryaddress or-ed with the set memorycommand 0x80.
// Left it in here for compatibility with older code. New applications should use getAddress() instead.
int TextLCD_Base::_address(int column, int row) {
  return 0x80 | getAddress(column, row);
}


// This is new method to return the memory address based on row, column and displaytype.
//
/** Return the memoryaddress of screen column and row location
   *
   * @param column  The horizontal position from the left, indexed from 0
   * @param row     The vertical position from the top, indexed from 0
   * @param return  The memoryaddress of screen column and row location
   *
   */
int TextLCD_Base::getAddress(int column, int row) {

    switch (_addr_mode) {

        case LCD_T_A:
          //Default addressing mode for 1, 2 and 4 rows (except 40x4)
          //The two available rows are split and stacked on top of eachother. Addressing for 3rd and 4th line continues where lines 1 and 2 were split.          
          //Displays top rows when less than four are used.          
          switch (row) {
            case 0:
              return 0x00 + column;
            case 1:
              return 0x40 + column;
            case 2:
              return 0x00 + _nr_cols + column;
            case 3:
              return 0x40 + _nr_cols + column;
            // Should never get here.
            default:            
              return 0x00;                    
            }
          
        case LCD_T_B:
          // LCD8x2B is a special layout of LCD16x1
          if (row==0) 
            return 0x00 + column;                        
          else   
//            return _nr_cols + column;                                    
            return 0x08 + column;                        

        case LCD_T_C:
          // LCD16x1C is a special layout of LCD8x2
          // LCD32x1C is a special layout of LCD16x2                    
          // LCD40x1C is a special layout of LCD20x2          
#if(0)
          if (column < 8) 
            return 0x00 + column;                        
          else   
            return 0x40 + (column - 8);                        
#else
          if (column < (_nr_cols >> 1)) 
            return 0x00 + column;                        
          else   
            return 0x40 + (column - (_nr_cols >> 1));                        
#endif

// Not sure about this one, seems wrong.
// Left in for compatibility with original library
//        case LCD16x2B:      
//            return 0x00 + (row * 40) + column;

        case LCD_T_D:
          //Alternate addressing mode for 3 and 4 row displays (except 40x4). Used by PCF21XX, KS0078, SSD1803
          //The 4 available rows start at a hardcoded address.                    
          //Displays top rows when less than four are used.
          switch (row) {
            case 0:
              return 0x00 + column;
            case 1:
              return 0x20 + column;
            case 2:
              return 0x40 + column;
            case 3:
              return 0x60 + column;
            // Should never get here.
            default:            
              return 0x00;                    
            }

        case LCD_T_D1:
          //Alternate addressing mode for 3 row displays. Used by PCF21XX, KS0078, SSD1803
          //The 4 available rows start at a hardcoded address.                              
          //Skips top row of 4 row display and starts display at row 1
          switch (row) {
            case 0:
              return 0x20 + column;
            case 1:
              return 0x40 + column;
            case 2:
              return 0x60 + column;
            // Should never get here.
            default:            
              return 0x00;                    
            }
        
        case LCD_T_E:                
          // LCD40x4 is a special case since it has 2 controllers.
          // Each controller is configured as 40x2 (Type A)
          if (row<2) { 
            // Test to see if we need to switch between controllers  
            if (_ctrl_idx != _LCDCtrl_0) {

              // Second LCD controller Cursor Off
              _setCursorAndDisplayMode(_currentMode, CurOff_BlkOff);    

              // Select primary controller
              _ctrl_idx = _LCDCtrl_0;

              // Restore cursormode on primary LCD controller
              _setCursorAndDisplayMode(_currentMode, _currentCursor);    
            }           
            
            return 0x00 + (row * 0x40) + column;          
          }
          else {

            // Test to see if we need to switch between controllers  
            if (_ctrl_idx != _LCDCtrl_1) {
              // Primary LCD controller Cursor Off
              _setCursorAndDisplayMode(_currentMode, CurOff_BlkOff);    

              // Select secondary controller
              _ctrl_idx = _LCDCtrl_1;

              // Restore cursormode on secondary LCD controller
              _setCursorAndDisplayMode(_currentMode, _currentCursor);    
            }           
                                   
            return 0x00 + ((row-2) * 0x40) + column;          
          } 
            
        case LCD_T_F:
          //Alternate addressing mode for 3 row displays.
          //The first half of 3rd row continues from 1st row, the second half continues from 2nd row.                              
          switch (row) {
            case 0:
              return 0x00 + column;
            case 1:
              return 0x40 + column;
            case 2:
              if (column < (_nr_cols >> 1)) // check first or second half of line
                return (0x00 + _nr_cols + column);                        
              else   
                return (0x40 + _nr_cols + (column - (_nr_cols >> 1)));                        
            // Should never get here.
            default:            
              return 0x00;                    
          }

        case LCD_T_G:
          //Alternate addressing mode for 3 row displays. Used by ST7036
          switch (row) {
            case 0:
              return 0x00 + column;
            case 1:
              return 0x10 + column;
            case 2:
              return 0x20 + column;
            // Should never get here.
            default:            
              return 0x00;                    
            }

        // Should never get here.
        default:            
            return 0x00;        

    } // switch _addr_mode
}


/** Set the memoryaddress of screen column and row location
  *
  * @param column  The horizontal position from the left, indexed from 0
  * @param row     The vertical position from the top, indexed from 0
  */
void TextLCD_Base::setAddress(int column, int row) {
   
// Sanity Check column
    if (column < 0) {
      _column = 0;
    }
    else if (column >= columns()) {
      _column = columns() - 1;
    } else _column = column;
    
// Sanity Check row
    if (row < 0) {
      _row = 0;
    }
    else if (row >= rows()) {
      _row = rows() - 1;
    } else _row = row;
    
    
// Compute the memory address
// For LCD40x4:  switch controllers if needed
//               switch cursor if needed
    int addr = getAddress(_column, _row);
    
    _writeCommand(0x80 | addr);
}


/** Return the number of columns
  *
  * @param return  The number of columns
  *
  * Note: some configurations are commented out because they have not yet been tested due to lack of hardware     
  */   
int TextLCD_Base::columns() {
    
  // Columns encoded in b7..b0
  //return (_type & 0xFF);          
  return _nr_cols;           
}

/** Return the number of rows
  *
  * @param return  The number of rows
  *
  * Note: some configurations are commented out because they have not yet been tested due to lack of hardware     
  */
int TextLCD_Base::rows() {

  // Rows encoded in b15..b8  
  //return ((_type >> 8) & 0xFF); 
  return _nr_rows;          
}

/** Set the Cursormode
  *
  * @param cursorMode  The Cursor mode (CurOff_BlkOff, CurOn_BlkOff, CurOff_BlkOn, CurOn_BlkOn)
  */
void TextLCD_Base::setCursor(LCDCursor cursorMode) { 

  // Save new cursor mode, needed when 2 controllers are in use or when display is switched off/on
  _currentCursor = cursorMode;
    
  // Configure only current LCD controller
  _setCursorAndDisplayMode(_currentMode, _currentCursor);    
}

/** Set the Displaymode
  *
  * @param displayMode The Display mode (DispOff, DispOn)
  */
void TextLCD_Base::setMode(LCDMode displayMode) { 

  // Save new displayMode, needed when 2 controllers are in use or when cursor is changed
  _currentMode = displayMode;
    
  // Select and configure second LCD controller when needed
  if(_type==LCD40x4) {
    if (_ctrl_idx==_LCDCtrl_0) {      
      // Configure primary LCD controller
      _setCursorAndDisplayMode(_currentMode, _currentCursor);

      // Select 2nd controller
      _ctrl_idx=_LCDCtrl_1;
  
      // Configure secondary LCD controller    
      _setCursorAndDisplayMode(_currentMode, CurOff_BlkOff);

      // Restore current controller
      _ctrl_idx=_LCDCtrl_0;       
    }
    else {
      // Select primary controller
      _ctrl_idx=_LCDCtrl_0;
    
      // Configure primary LCD controller
      _setCursorAndDisplayMode(_currentMode, CurOff_BlkOff);
       
      // Restore current controller
      _ctrl_idx=_LCDCtrl_1;

      // Configure secondary LCD controller    
      _setCursorAndDisplayMode(_currentMode, _currentCursor);
    }
  }
  else {
    // Configure primary LCD controller
    _setCursorAndDisplayMode(_currentMode, _currentCursor);
  }       
}


/** Low level method to restore the cursortype and display mode for current controller
  */     
void TextLCD_Base::_setCursorAndDisplayMode(LCDMode displayMode, LCDCursor cursorType) { 
    
    // Configure current LCD controller       
    _writeCommand(0x08 | displayMode | cursorType);
}

/** Set the Backlight mode
  *
  *  @param backlightMode The Backlight mode (LightOff, LightOn)
  */
void TextLCD_Base::setBacklight(LCDBacklight backlightMode) {

    if (backlightMode == LightOn) {
      this->_setBL(true);
    }
    else {
      this->_setBL(false);    
    }
} 

/** Set User Defined Characters
  *
  * @param unsigned char c   The Index of the UDC (0..7)
  * @param char *udc_data    The bitpatterns for the UDC (8 bytes of 5 significant bits)     
  */
void TextLCD_Base::setUDC(unsigned char c, char *udc_data) {
  
  // Select and configure second LCD controller when needed
  if(_type==LCD40x4) {
    _LCDCtrl_Idx current_ctrl_idx = _ctrl_idx; // Temp save current controller
   
    // Select primary controller     
    _ctrl_idx=_LCDCtrl_0;
    
    // Configure primary LCD controller
    _setUDC(c, udc_data);

    // Select 2nd controller
    _ctrl_idx=_LCDCtrl_1;
  
    // Configure secondary LCD controller    
    _setUDC(c, udc_data);

    // Restore current controller
    _ctrl_idx=current_ctrl_idx;       
  }
  else {
    // Configure primary LCD controller
    _setUDC(c, udc_data); 
  }
    
}

/** Low level method to store user defined characters for current controller
  */     
void TextLCD_Base::_setUDC(unsigned char c, char *udc_data) {
  
  // Select CG RAM for current LCD controller
  _writeCommand(0x40 + ((c & 0x07) << 3)); //Set CG-RAM address,
                                           //8 sequential locations needed per UDC
  // Store UDC pattern 
  for (int i=0; i<8; i++) {
    _writeData(*udc_data++);
  }
   
  //Select DD RAM again for current LCD controller
  int addr = getAddress(_column, _row);
  _writeCommand(0x80 | addr);  
}


/** Set UDC Blink
  * setUDCBlink method is supported by some compatible devices (eg SSD1803) 
  *
  * @param blinkMode The Blink mode (BlinkOff, BlinkOn)
  */
void TextLCD_Base::setUDCBlink(LCDBlink blinkMode){
  // Blinking UDCs are enabled when a specific controlbit (BE) is set.
  // The blinking pixels in the UDC can be controlled by setting additional bits in the UDC bitpattern.
  // UDCs are defined by an 8 byte bitpattern. The P0..P5 form the character pattern.
  //     P7 P6 P5 P4 P3 P2 P1 P0 
  // 0   B1 B0  x  0  1  1  1  0
  // 1   B1 B0  x  1  0  0  0  1
  //        .............
  // 7   B1 B0  x  1  0  0  0  1
  //
  // Bit 6 and Bit 7 in the pattern will control the blinking mode when Blink is enabled through BE. 
  //     B1 B0  Mode
  //      0  0  No Blinking in this row of the UDC
  //      0  1  Enabled pixels in P4 will blink
  //      1  x  Enabled pixels in P0..P4 will blink

  switch (blinkMode) {
    case BlinkOn: 
      // Controllers that support UDC Blink  
      switch (_ctrl) {
        case KS0078 :            
          _function_1 |= 0x02; // Enable UDC Blink        
          _writeCommand(0x20 | _function_1);        // Function set 0 0 1 DL N RE(1) BE 0 (Ext Regs)

          _writeCommand(0x20 | _function);          // Function set 0 0 1 DL N RE(0) DH REV (Std Regs)
          break; // case KS0078 Controller
    
        case US2066_3V3 :  
        case SSD1803_3V3 :  
          _function_1 |= 0x04; // Enable UDC Blink
          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 DL N BE RE(1) REV 
                                                    // Select Ext Instr Set

          _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                    // Select Std Instr set, Select IS=0
          break; // case SSD1803, US2066
       
        default:
          //Unsupported feature for other controllers        
          break; 
      } //switch _ctrl     
    
      break;      

    case BlinkOff:
      // Controllers that support UDC Blink  
      switch (_ctrl) {
        case KS0078 :            
          _function_1 &= ~0x02; // Disable UDC Blink        
          _writeCommand(0x20 | _function_1);        // Function set 0 0 1 DL N RE(1) BE 0 (Ext Regs)

          _writeCommand(0x20 | _function);          // Function set 0 0 1 DL N RE(0) DH REV (Std Regs)
          break; // case KS0078 Controller
    
        case US2066_3V3 :  
        case SSD1803_3V3 :  
          _function_1 &= ~0x04; // Disable UDC Blink
          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 DL N BE RE(1) REV 
                                                    // Select Ext Instr Set

          _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                    // Select Std Instr set, Select IS=0
          break; // case SSD1803, US2066          
       
        default:
          //Unsupported feature for other controllers        
          break; 
      } //switch _ctrl     
    
      break;        
      
    default:
      break;      
  } // blinkMode
  
} // setUDCBlink()


/** Set Contrast
  * setContrast method is supported by some compatible devices (eg ST7032i) that have onboard LCD voltage generation
  * Initial code for ST70XX imported from fork by JH1PJL
  *
  * @param unsigned char c   contrast data (6 significant bits, valid range 0..63, Value 0 will disable the Vgen)  
  * @return none
  */
//@TODO Add support for 40x4 dual controller
void TextLCD_Base::setContrast(unsigned char c) {

// Function set mode stored during Init. Make sure we dont accidentally switch between 1-line and 2-line mode!
// Icon/Booster mode stored during Init. Make sure we dont accidentally change this!
 
  _contrast = c & 0x3F; // Sanity check
  
  switch (_ctrl) {   
    case PCF2113_3V3 :  
    case PCF2119_3V3 :  
       if (_contrast <  5) _contrast = 0;  // See datasheet. Sanity check for PCF2113/PCF2119
       if (_contrast > 55) _contrast = 55;
      
       _writeCommand(0x20 | _function | 0x01);               // Set function, Select Instruction Set = 1              
       _writeCommand(0x80 | 0x00 | (_contrast & 0x3F));      // VLCD_set (Instr. Set 1)    V=0, VA=contrast
       _writeCommand(0x80 | 0x40 | (_contrast & 0x3F));      // VLCD_set (Instr. Set 1)    V=1, VB=contrast
       _writeCommand(0x20 | _function);                      // Select Instruction Set = 0
       break;
        
    case ST7032_3V3 :  
    case ST7032_5V :      
    case ST7036_3V3 :      
//    case ST7036_5V :          
    case SSD1803_3V3 :      
      _writeCommand(0x20 | _function | 0x01);                        // Select Instruction Set = 1
      _writeCommand(0x70 | (_contrast & 0x0F));                      // Contrast Low bits
      _writeCommand(0x50 | _icon_power | ((_contrast >> 4) & 0x03)); // Contrast High bits 
      _writeCommand(0x20 | _function);                               // Select Instruction Set = 0
      break;

    case US2066_3V3 :      
      _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 DL N BE RE(1) REV 
                                                // Select Extended Instruction Set

      _writeCommand(0x79);                      // Function Select OLED:  0 1 1 1 1 0 0 1 (Ext Instr Set)
         
      _writeCommand(0x81);                      // Set Contrast Control: 1 0 0 0 0 0 0 1 (Ext Instr Set, OLED)
      _writeCommand((_contrast << 2) | 0x03);   // Set Contrast Value: 8 bits. Use 6 bits for compatibility    
      
      _writeCommand(0x78);                      // Function Disable OLED: 0 1 1 1 1 0 0 0 (Ext Instr Set)          

      _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                // Select Std Instr set, Select IS=0
      break;

 #if(0)
 //not yet tested
    case PT6314 :
      // Only 2 significant bits
      //   0x00 = 100%
      //   0x01 =  75%
      //   0x02 =  50%
      //   0x03 =  25%                
      _writeCommand(0x20 | _function | ((~_contrast) >> 4));        // Invert and shift to use 2 MSBs     
      break;
 #endif
            
    default:  
      //Unsupported feature for other controllers
      break;               
  } // end switch     
} // end setContrast()


/** Set Power
  * setPower method is supported by some compatible devices (eg SSD1803) that have power down modes
  *
  * @param bool powerOn  Power on/off   
  * @return none
  */
//@TODO Add support for 40x4 dual controller  
void TextLCD_Base::setPower(bool powerOn) {
  
  if (powerOn) {
    // Switch on  
    setMode(DispOn);       

    // Controllers that supports specific Power Down mode
    switch (_ctrl) {
    
//    case PCF2113_3V3 :  
//    case PCF2119_3V3 :  
//    case ST7032_3V3 :  
//@todo
//    enable Booster Bon

      case WS0010:      
        _writeCommand(0x17);   // Char mode, DC/DC on        
        ThisThread::sleep_for(10ms);           // Wait 10ms to ensure powered up             
        break;

      case KS0078:        
      case SSD1803_3V3 :      
//      case SSD1803_5V :            
        _writeCommand(0x20 | _function_1);                             // Select Ext Instr Set
        _writeCommand(0x02);                                           // Power On
        _writeCommand(0x20 | _function);                               // Select Std Instr Set
        break;
                    
      default:  
        //Unsupported feature for other controllers
        break;              
    } // end switch  
  }  
  else {
    // Switch off        
    setMode(DispOff);       

    // Controllers that support specific Power Down mode
    switch (_ctrl) {
    
//    case PCF2113_3V3 :  
//    case PCF2119_3V3 :  
//    case ST7032_3V3 :  
//@todo
//    disable Booster Bon

      case WS0010:      
        _writeCommand(0x13);   // Char mode, DC/DC off              
        break;
        
      case KS0078:
      case SSD1803_3V3 :      
//      case SSD1803_5V :            
        _writeCommand(0x20 | _function_1);                             // Select Ext Instr Set
        _writeCommand(0x03);                                           // Power Down
        _writeCommand(0x20 | _function);                               // Select Std Instr Set
        break;

      default:  
        //Unsupported feature for other controllers
        break;              
    } // end switch  
  }
} // end setPower()


/** Set Orient
  * setOrient method is supported by some compatible devices (eg SSD1803, US2066) that have top/bottom view modes
  *
  * @param LCDOrient orient Orientation 
  * @return none
  */
void TextLCD_Base::setOrient(LCDOrient orient){

  switch (orient) {
       
    case Top:
      switch (_ctrl) {
        case SSD1803_3V3 :      
//      case SSD1803_5V :
        case US2066_3V3 :      
          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 X N BE RE(1) REV 
                                                    // Select Extended Instruction Set
//          _writeCommand(0x06);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=1 COM1-32, BDS=0 SEG100-1    "Bottom View" (Ext Instr Set)
          _writeCommand(0x05);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=0 COM32-1, BDS=1 SEG1-100    "Top View" (Ext Instr Set)          

          _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                    // Select Std Instr set, Select IS=0       
          break;
          
        default:  
          //Unsupported feature for other controllers
          break;              

      } // end switch _ctrl     
      break; // end Top
                
    case Bottom:
      switch (_ctrl) {
        case SSD1803_3V3 :      
//      case SSD1803_5V :
        case US2066_3V3 :      
          _writeCommand(0x20 | _function_1);        // Set function, 0 0 1 X N BE RE(1) REV 
                                                    // Select Extended Instruction Set
          _writeCommand(0x06);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=1 COM1-32, BDS=0 SEG100-1    "Bottom View" (Ext Instr Set)
//          _writeCommand(0x05);                      // Set ext entry mode, 0 0 0 0 0 1 BDC=0 COM32-1, BDS=1 SEG1-100    "Top View" (Ext Instr Set)          

          _writeCommand(0x20 | _function);          // Set function, 0 0 1 DL N DH RE(0) IS=0 Select Instruction Set 0
                                                    // Select Std Instr set, Select IS=0       
          break;
          
        default:  
          //Unsupported feature for other controllers
          break;              

      } // end switch _ctrl     
    
      break; // end Bottom
  } // end switch orient
} // end setOrient()



//--------- End TextLCD_Base -----------


//--------- Start TextLCD Bus -----------

/* Create a TextLCD interface for using regular mbed pins
 *
 * @param rs     Instruction/data control line
 * @param e      Enable line (clock)
 * @param d4-d7  Data lines for using as a 4-bit interface
 * @param type   Sets the panel size/addressing mode (default = LCD16x2)
 * @param bl     Backlight control line (optional, default = NC)  
 * @param e2     Enable2 line (clock for second controller, LCD40x4 only) 
 * @param ctrl   LCD controller (default = HD44780)   
 */ 
TextLCD::TextLCD(PinName rs, PinName e,
                 PinName d4, PinName d5, PinName d6, PinName d7,
                 LCDType type, PinName bl, PinName e2, LCDCtrl ctrl) :
                 TextLCD_Base(type, ctrl), 
                 _rs(rs), _e(e), _d(d4, d5, d6, d7) {

  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  

  // The hardware Enable2 pin is only needed for LCD40x4. Test and make sure whether it exists or not to prevent illegal access.
  if (e2 != NC) {
    _e2 = new DigitalOut(e2);   //Construct new pin 
    _e2->write(0);              //Deactivate    
  }
  else {
    // No Hardware Enable pin       
    _e2 = NULL;                 //Construct dummy pin     
  }  
                                                                           
  _init();
}

/** Destruct a TextLCD interface for using regular mbed pins
  *
  * @param  none
  * @return none
  */ 
TextLCD::~TextLCD() {
   if (_bl != NULL) {delete _bl;}  // BL pin
   if (_e2 != NULL) {delete _e2;}  // E2 pin
}


/** Set E pin (or E2 pin)
  * Used for mbed pins, I2C bus expander or SPI shiftregister
  * Default PinName value for E2 is NC, must be used as pointer to avoid issues with mbed lib and DigitalOut pins
  *   @param  value true or false
  *   @return none 
  */
void TextLCD::_setEnable(bool value) {

  if(_ctrl_idx==_LCDCtrl_0) {
    if (value) {
      _e  = 1;    // Set E bit 
    }  
    else { 
      _e  = 0;    // Reset E bit  
    }  
  }    
  else { 
    if (value) {
      if (_e2 != NULL) {_e2->write(1);}  //Set E2 bit
    }  
    else { 
      if (_e2 != NULL) {_e2->write(0);}  //Reset E2 bit     
    }  
  }    
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD::_setRS(bool value) {

  if (value) {
    _rs  = 1;    // Set RS bit 
  }  
  else  {
    _rs  = 0;    // Reset RS bit 
  }  
}    

/** Set BL pin
  * Used for mbed pins, I2C bus expander or SPI shiftregister
  * Default PinName value is NC, must be used as pointer to avoid issues with mbed lib and DigitalOut pins
  *   @param  value true or false
  *   @return none  
  */
void TextLCD::_setBL(bool value) {

  if (value) {
    if (_bl != NULL) {_bl->write(1);}  //Set BL bit
  }  
  else { 
    if (_bl != NULL) {_bl->write(0);}  //Reset BL bit  
  }  
}    

// Place the 4bit data on the databus
// Used for mbed pins, I2C bus expander or SPI shifregister
void TextLCD::_setData(int value) {
  _d = value & 0x0F;   // Write Databits 
}    
    
//----------- End TextLCD ---------------


//--------- Start TextLCD_I2C -----------

/** Create a TextLCD interface using an I2C PC8574 (or PCF8574A) or MCP23008 portexpander
  *
  * @param i2c             I2C Bus
  * @param deviceAddress   I2C slave address (PCF8574, PCF8574A or MCP23008, default = 0x40)
  * @param type            Sets the panel size/addressing mode (default = LCD16x2)
  * @param ctrl            LCD controller (default = HD44780)    
  */
TextLCD_I2C::TextLCD_I2C(I2C *i2c, char deviceAddress, LCDType type, LCDCtrl ctrl) :
                         TextLCD_Base(type, ctrl), 
                         _i2c(i2c){
                              
  _slaveAddress = deviceAddress & 0xFE;

  // Setup the I2C bus
  // The max bitrate for PCF8574 is 100kbit, the max bitrate for MCP23008 is 400kbit, 
  _i2c->frequency(100000);
  
#if (MCP23008==1)
  // MCP23008 portexpander Init
  _write_register(IODIR,   0x00);  // All outputs
  _write_register(IPOL,    0x00);  // No reverse polarity 
  _write_register(GPINTEN, 0x00);  // No interrupt 
  _write_register(DEFVAL,  0x00);  // Default value to compare against for interrupts
  _write_register(INTCON,  0x00);  // No interrupt on changes 
  _write_register(IOCON,   0x00);  // Interrupt polarity   
  _write_register(GPPU,    0x00);  // No Pullup 
  _write_register(INTF,    0x00);  //    
  _write_register(INTCAP,  0x00);  //    
  _write_register(GPIO,    0x00);  // Output/Input pins   
  _write_register(OLAT,    0x00);  // Output Latch  
    
  // Init the portexpander bus
  _lcd_bus = D_LCD_BUS_DEF;
  
  // write the new data to the portexpander
  _write_register(GPIO, _lcd_bus);      
#else
  // PCF8574 of PCF8574A portexpander

  // Init the portexpander bus
  _lcd_bus = D_LCD_BUS_DEF;

  // write the new data to the portexpander
  _i2c->write(_slaveAddress, &_lcd_bus, 1);    
#endif

  _init();    
}

// Set E pin (or E2 pin)
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_I2C::_setEnable(bool value) {

  if(_ctrl_idx==_LCDCtrl_0) {
    if (value) {
      _lcd_bus |= D_LCD_E;     // Set E bit 
    }  
    else {                    
      _lcd_bus &= ~D_LCD_E;    // Reset E bit                     
    }  
  }
  else {
    if (value) {
      _lcd_bus |= D_LCD_E2;    // Set E2 bit 
    }  
    else {
      _lcd_bus &= ~D_LCD_E2;   // Reset E2bit                     
    }  
  }    

#if (MCP23008==1)
  // MCP23008 portexpander
  
  // write the new data to the portexpander
  _write_register(GPIO, _lcd_bus);      
#else
  // PCF8574 of PCF8574A portexpander

  // write the new data to the I2C portexpander
  _i2c->write(_slaveAddress, &_lcd_bus, 1);    
#endif
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_I2C::_setRS(bool value) {

  if (value) {
    _lcd_bus |= D_LCD_RS;    // Set RS bit 
  }  
  else {                    
    _lcd_bus &= ~D_LCD_RS;   // Reset RS bit                     
  }

#if (MCP23008==1)
  // MCP23008 portexpander
  
  // write the new data to the portexpander
  _write_register(GPIO, _lcd_bus);      
#else
  // PCF8574 of PCF8574A portexpander

  // write the new data to the I2C portexpander
  _i2c->write(_slaveAddress, &_lcd_bus, 1);    
#endif                  
}    

// Set BL pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_I2C::_setBL(bool value) {

#if (DFROBOT==1)  
  value = !value; // The DFRobot module uses PNP transistor to drive the Backlight. Reverse logic level.
#endif
  
  if (value) {
    _lcd_bus |= D_LCD_BL;    // Set BL bit 
  }  
  else {                    
    _lcd_bus &= ~D_LCD_BL;   // Reset BL bit                     
  }
  
#if (MCP23008==1)
  // MCP23008 portexpander
  
  // write the new data to the portexpander
  _write_register(GPIO, _lcd_bus);      
#else
  // PCF8574 of PCF8574A portexpander

  // write the new data to the I2C portexpander
  _i2c->write(_slaveAddress, &_lcd_bus, 1);    
#endif                 
}    


// Place the 4bit data on the databus
// Used for mbed pins, I2C bus expander or SPI shifregister
void TextLCD_I2C::_setData(int value) {
  int data;

  // Set bit by bit to support any mapping of expander portpins to LCD pins
  
  data = value & 0x0F;
  if (data & 0x01){
    _lcd_bus |= D_LCD_D4;   // Set Databit 
  }  
  else { 
    _lcd_bus &= ~D_LCD_D4;  // Reset Databit
  }  

  if (data & 0x02){
    _lcd_bus |= D_LCD_D5;   // Set Databit 
  }  
  else {
    _lcd_bus &= ~D_LCD_D5;  // Reset Databit
  }  

  if (data & 0x04) {
    _lcd_bus |= D_LCD_D6;   // Set Databit 
  }  
  else {                    
    _lcd_bus &= ~D_LCD_D6;  // Reset Databit
  }  

  if (data & 0x08) {
    _lcd_bus |= D_LCD_D7;   // Set Databit 
  }  
  else {
    _lcd_bus &= ~D_LCD_D7;  // Reset Databit
  }  
                    
#if (MCP23008==1)
  // MCP23008 portexpander
  
  // write the new data to the portexpander
  _write_register(GPIO, _lcd_bus);      
#else
  // PCF8574 of PCF8574A portexpander

  // write the new data to the I2C portexpander
  _i2c->write(_slaveAddress, &_lcd_bus, 1);    
#endif
                 
}    

// Write data to MCP23008 I2C portexpander
void TextLCD_I2C::_write_register (int reg, int value) {
  char data[] = {(char)reg, (char)value};
    
  _i2c->write(_slaveAddress, data, 2); 
}

//---------- End TextLCD_I2C ------------


//--------- Start TextLCD_I2C_N ---------

 /** Create a TextLCD interface using a controller with native I2C interface
   *
   * @param i2c             I2C Bus
   * @param deviceAddress   I2C slave address (default = 0x7C)  
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param bl              Backlight control line (optional, default = NC)     
   * @param ctrl            LCD controller (default = ST7032_3V3)                     
   */
TextLCD_I2C_N::TextLCD_I2C_N(I2C *i2c, char deviceAddress, LCDType type, PinName bl, LCDCtrl ctrl) : 
                               TextLCD_Base(type, ctrl), 

                               _i2c(i2c){
  
  _slaveAddress = deviceAddress & 0xFE;
  
  // Setup the I2C bus
  // The max bitrate for ST7032i is 400kbit, lets stick to default here
  _i2c->frequency(100000);

       
  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  
  
  //Sanity check
  if (_ctrl & LCD_C_I2C) {
    _init();      
  }
  else {
    error("Error: LCD Controller type does not support native I2C interface\n\r");           
  }

}

TextLCD_I2C_N::~TextLCD_I2C_N() {
   if (_bl != NULL) {delete _bl;}  // BL pin
}

// Not used in this mode
void TextLCD_I2C_N::_setEnable(bool value) {
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister and native I2C or SPI
void TextLCD_I2C_N::_setRS(bool value) {
// The controlbyte defines the meaning of the next byte. This next byte can either be data or command.
// Start Slaveaddress+RW  b7 b6 b5 b4 b3 b2 b1 b0   b7...........b0  Stop
//                        Co RS RW  0  0  0  0  0   command or data
//
//   C0=1 indicates that another controlbyte will follow after the next data or command byte 
//   RS=1 means that next byte is data, RS=0 means that next byte is command
//   RW=0 means write to controller. RW=1 means that controller will be read from after the next command. 
//        Many native I2C controllers dont support this option and it is not used by this lib. 
//

  if (value) {
    _controlbyte = 0x40; // Next byte is data, No more control bytes will follow
  }
  else {
    _controlbyte = 0x00; // Next byte is command, No more control bytes will follow     
  }
}    

// Set BL pin
void TextLCD_I2C_N::_setBL(bool value) {
    if (_bl) {
        _bl->write(value);   
    }    
}    
    
// Not used in this mode
void TextLCD_I2C_N::_setData(int value) {
}    

// Write a byte using I2C
void TextLCD_I2C_N::_writeByte(int value) {
// The controlbyte defines the meaning of the next byte. This next byte can either be data or command.
// Start Slaveaddress+RW  b7 b6 b5 b4 b3 b2 b1 b0   b7...........b0  Stop
//                        Co RS RW  0  0  0  0  0   command or data
//
//   C0=1 indicates that another controlbyte will follow after the next data or command byte 
//   RS=1 means that next byte is data, RS=0 means that next byte is command
//   RW=0 means write to controller. RW=1 means that controller will be read from after the next command. 
//        Many native I2C controllers dont support this option and it is not used by this lib. 
//
  char data[] = {_controlbyte, (char)value};
    
#if(LCD_I2C_ACK==1)
//Controllers that support ACK
  _i2c->write(_slaveAddress, data, 2); 
#else  
//Controllers that dont support ACK
  _i2c->start(); 
  _i2c->write(_slaveAddress);   
  _i2c->write(data[0]); 
  _i2c->write(data[1]);     
  _i2c->stop();   
#endif  
}

//-------- End TextLCD_I2C_N ------------


//--------- Start TextLCD_SPI -----------

 /** Create a TextLCD interface using an SPI 74595 portexpander
   *
   * @param spi             SPI Bus
   * @param cs              chip select pin (active low)
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param ctrl            LCD controller (default = HD44780)      
   */
TextLCD_SPI::TextLCD_SPI(SPI *spi, PinName cs, LCDType type, LCDCtrl ctrl) :
                         TextLCD_Base(type, ctrl), 
                         _spi(spi),        
                         _cs(cs) {      
        
  // Init cs
  _setCS(true);  

  // Setup the spi for 8 bit data, low steady state clock,
  // rising edge capture, with a 500KHz or 1MHz clock rate  
  _spi->format(8,0);
  _spi->frequency(500000);    
  //_spi.frequency(1000000);    

  // Init the portexpander bus
  _lcd_bus = D_LCD_BUS_DEF;
  
  // write the new data to the portexpander
  _setCS(false);  
  _spi->write(_lcd_bus);   
  _setCS(true);  

  _init();   
}

// Set E pin (or E2 pin)
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI::_setEnable(bool value) {

  if(_ctrl_idx==_LCDCtrl_0) {
    if (value) {
      _lcd_bus |= D_LCD_E;     // Set E bit 
    }  
    else {                    
      _lcd_bus &= ~D_LCD_E;    // Reset E bit                     
    }  
  }
  else {
    if (value) {
      _lcd_bus |= D_LCD_E2;    // Set E2 bit 
    }  
    else {
      _lcd_bus &= ~D_LCD_E2;   // Reset E2 bit                     
    }  
  }
                  
  // write the new data to the SPI portexpander
  _setCS(false);  
  _spi->write(_lcd_bus);   
  _setCS(true);    
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI::_setRS(bool value) {

  if (value) {
    _lcd_bus |= D_LCD_RS;    // Set RS bit 
  }  
  else {                    
    _lcd_bus &= ~D_LCD_RS;   // Reset RS bit                     
  }
     
  // write the new data to the SPI portexpander
  _setCS(false);  
  _spi->write(_lcd_bus);   
  _setCS(true);     
}    

// Set BL pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI::_setBL(bool value) {

  if (value) {
    _lcd_bus |= D_LCD_BL;    // Set BL bit 
  }  
  else {
    _lcd_bus &= ~D_LCD_BL;   // Reset BL bit                     
  }
      
  // write the new data to the SPI portexpander
  _setCS(false);  
  _spi->write(_lcd_bus);   
  _setCS(true);      
}    

// Place the 4bit data on the databus
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI::_setData(int value) {
  int data;

  // Set bit by bit to support any mapping of expander portpins to LCD pins
    
  data = value & 0x0F;
  if (data & 0x01) {
    _lcd_bus |= D_LCD_D4;   // Set Databit 
  }  
  else {                    
    _lcd_bus &= ~D_LCD_D4;  // Reset Databit                     
  }
  
  if (data & 0x02) {
    _lcd_bus |= D_LCD_D5;   // Set Databit 
  }  
  else {
    _lcd_bus &= ~D_LCD_D5;  // Reset Databit                     
  }
  
  if (data & 0x04) {
    _lcd_bus |= D_LCD_D6;   // Set Databit 
  }  
  else {
    _lcd_bus &= ~D_LCD_D6;  // Reset Databit                     
  }
  
  if (data & 0x08) {
    _lcd_bus |= D_LCD_D7;   // Set Databit 
  }  
  else {
    _lcd_bus &= ~D_LCD_D7;  // Reset Databit
  }  
                    
  // write the new data to the SPI portexpander
  _setCS(false);  
  _spi->write(_lcd_bus);   
  _setCS(true);          
}    

// Set CS line.
// Only used for SPI bus
void TextLCD_SPI::_setCS(bool value) {

  if (value) {   
    _cs  = 1;    // Set CS pin 
  }  
  else {
    _cs  = 0;    // Reset CS pin 
  }
}

//---------- End TextLCD_SPI ------------


//--------- Start TextLCD_SPI_N ---------

 /** Create a TextLCD interface using a controller with a native SPI4 interface
   *
   * @param spi             SPI Bus
   * @param cs              chip select pin (active low)
   * @param rs              Instruction/data control line
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param bl              Backlight control line (optional, default = NC)  
   * @param ctrl            LCD controller (default = ST7032_3V3) 
   */       
TextLCD_SPI_N::TextLCD_SPI_N(SPI *spi, PinName cs, PinName rs, LCDType type, PinName bl, LCDCtrl ctrl) :
                             TextLCD_Base(type, ctrl), 
                             _spi(spi),        
                             _cs(cs),
                             _rs(rs) {      
        
  // Init CS
  _cs = 1;

  // Setup the spi for 8 bit data, low steady state clock,
  // rising edge capture, with a 500KHz or 1MHz clock rate  
  _spi->format(8,0);
  _spi->frequency(1000000);    
  
  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  

  //Sanity check
  if (_ctrl & LCD_C_SPI4) {
    _init();      
  }
  else {
    error("Error: LCD Controller type does not support native SPI4 interface\n\r");           
  }
}

TextLCD_SPI_N::~TextLCD_SPI_N() {
   if (_bl != NULL) {delete _bl;}  // BL pin
}

// Not used in this mode
void TextLCD_SPI_N::_setEnable(bool value) {
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI_N::_setRS(bool value) {
    _rs = value;
}    

// Set BL pin
void TextLCD_SPI_N::_setBL(bool value) {
    if (_bl) {
        _bl->write(value);   
    }    
}    

// Not used in this mode
void TextLCD_SPI_N::_setData(int value) {
}    

// Write a byte using SPI
void TextLCD_SPI_N::_writeByte(int value) {
    _cs = 0;
    wait_us(1);
    _spi->write(value);
    wait_us(1);
    _cs = 1;
}
  
//-------- End TextLCD_SPI_N ------------



#if(1)
//Code checked out on logic analyser. Not yet tested on hardware..

//-------- Start TextLCD_SPI_N_3_9 --------

 /** Create a TextLCD interface using a controller with a native SPI3 9 bits interface
   *
   * @param spi             SPI Bus
   * @param cs              chip select pin (active low)
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param bl              Backlight control line (optional, default = NC)  
   * @param ctrl            LCD controller (default = AIP31068) 
   */       
TextLCD_SPI_N_3_9::TextLCD_SPI_N_3_9(SPI *spi, PinName cs, LCDType type, PinName bl, LCDCtrl ctrl) :
                                     TextLCD_Base(type, ctrl), 
                                     _spi(spi),        
                                     _cs(cs) {      

  // Init CS
  _cs = 1;

  // Setup the spi for 9 bit data, low steady state clock,
  // rising edge capture, with a 500KHz or 1MHz clock rate  
//  _spi->format(9,0);
  _spi->format(9,3);  
  _spi->frequency(1000000);    
  
  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  

  //Sanity check
  if (_ctrl & LCD_C_SPI3_9) {
    _init();      
  }
  else {
    error("Error: LCD Controller type does not support native SPI3 9 bits interface\n\r");           
  }
}

TextLCD_SPI_N_3_9::~TextLCD_SPI_N_3_9() {
   if (_bl != NULL) {delete _bl;}  // BL pin
}

// Not used in this mode
void TextLCD_SPI_N_3_9::_setEnable(bool value) {
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI_N_3_9::_setRS(bool value) {
// The controlbits define the meaning of the next byte. This next byte can either be data or command.
//   b8  b7...........b0 
//   RS  command or data
//
//   RS=1 means that next byte is data, RS=0 means that next byte is command
//

  if (value) {
    _controlbyte = 0x01; // Next byte is data
  }
  else {
    _controlbyte = 0x00; // Next byte is command
  }
   
}    

// Set BL pin
void TextLCD_SPI_N_3_9::_setBL(bool value) {
    if (_bl) {
        _bl->write(value);   
    }    
}    

// Not used in this mode
void TextLCD_SPI_N_3_9::_setData(int value) {
}    

// Write a byte using SPI3 9 bits mode
void TextLCD_SPI_N_3_9::_writeByte(int value) {
    _cs = 0;
    wait_us(1);
    _spi->write( (_controlbyte << 8) | (value & 0xFF));
    wait_us(1);
    _cs = 1;
}
  
//------- End TextLCD_SPI_N_3_9 -----------
#endif


#if(1)
//------- Start TextLCD_SPI_N_3_10 --------

 /** Create a TextLCD interface using a controller with a native SPI3 10 bits interface
   *
   * @param spi             SPI Bus
   * @param cs              chip select pin (active low)
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param bl              Backlight control line (optional, default = NC)  
   * @param ctrl            LCD controller (default = AIP31068) 
   */       
TextLCD_SPI_N_3_10::TextLCD_SPI_N_3_10(SPI *spi, PinName cs, LCDType type, PinName bl, LCDCtrl ctrl) :
                                       TextLCD_Base(type, ctrl), 
                                       _spi(spi),        
                                       _cs(cs) {      
        
  // Init CS
  _cs = 1;

  // Setup the spi for 10 bit data, low steady state clock,
  // rising edge capture, with a 500KHz or 1MHz clock rate  
  _spi->format(10,0);
  _spi->frequency(1000000);    
  
  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  

  //Sanity check
  if (_ctrl & LCD_C_SPI3_10) {
    _init();      
  }
  else {
    error("Error: LCD Controller type does not support native SPI3 10 bits interface\n\r");           
  }
}

TextLCD_SPI_N_3_10::~TextLCD_SPI_N_3_10() {
   if (_bl != NULL) {delete _bl;}  // BL pin
}

// Not used in this mode
void TextLCD_SPI_N_3_10::_setEnable(bool value) {
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI_N_3_10::_setRS(bool value) {
// The controlbits define the meaning of the next byte. This next byte can either be data or command.
//   b9 b8  b7...........b0 
//   RS RW  command or data
//
//   RS=1 means that next byte is data, RS=0 means that next byte is command
//   RW=0 means that next byte is writen, RW=1 means that next byte is read (not used in this lib)
//

  if (value) {
    _controlbyte = 0x02; // Next byte is data
  }
  else {
    _controlbyte = 0x00; // Next byte is command
  }
   
}    

// Set BL pin
void TextLCD_SPI_N_3_10::_setBL(bool value) {
    if (_bl) {
        _bl->write(value);   
    }    
}    

// Not used in this mode
void TextLCD_SPI_N_3_10::_setData(int value) {
}    

// Write a byte using SPI3 10 bits mode
void TextLCD_SPI_N_3_10::_writeByte(int value) {
    _cs = 0;
    wait_us(1);
    _spi->write( (_controlbyte << 8) | (value & 0xFF));
    wait_us(1);
    _cs = 1;
}
  
//------- End TextLCD_SPI_N_3_10 ----------
#endif

#if(0)
//Code to be checked out on logic analyser. Not yet tested on hardware..

//------- Start TextLCD_SPI_N_3_16 --------

 /** Create a TextLCD interface using a controller with a native SPI3 16 bits interface
   *
   * @param spi             SPI Bus
   * @param cs              chip select pin (active low)
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param bl              Backlight control line (optional, default = NC)  
   * @param ctrl            LCD controller (default = PT6314) 
   */       
TextLCD_SPI_N_3_16::TextLCD_SPI_N_3_16(SPI *spi, PinName cs, LCDType type, PinName bl, LCDCtrl ctrl) :
                                       TextLCD_Base(type, ctrl), 
                                       _spi(spi),        
                                       _cs(cs) {      
        
  // Init CS
  _cs = 1;

  // Setup the spi for 8 bit data, low steady state clock,
  // rising edge capture, with a 500KHz or 1MHz clock rate  
  _spi->format(8,0);
  _spi->frequency(1000000);    
  
  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  

  //Sanity check
  if (_ctrl & LCD_C_SPI3_16) {
    _init();      
  }
  else {
    error("Error: LCD Controller type does not support native SPI3 16 bits interface\n\r");           
  }
}

TextLCD_SPI_N_3_16::~TextLCD_SPI_N_3_16() {
   if (_bl != NULL) {delete _bl;}  // BL pin
}

// Not used in this mode
void TextLCD_SPI_N_3_16::_setEnable(bool value) {
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI_N_3_16::_setRS(bool value) {
// The 16bit mode is split in 2 bytes. The first byte is for synchronisation and controlbits. The controlbits define the meaning of the next byte.
// The 8 actual bits represent either a data or a command byte.
//   b15 b14 b13 b12 b11 b10  b9  b8 - b7 b6 b5 b4 b3 b2 b1 b0 
//     1   1   1   1   1  RW  RS   0   d7 d6 d5 d4 d3 d2 d1 d0
//
//   RS=1 means that next byte is data, RS=0 means that next byte is command
//   RW=0 means that next byte is writen, RW=1 means that next byte is read (not used in this lib)
//

  if (value) {
    _controlbyte = 0xFA; // Next byte is data
  }
  else {
    _controlbyte = 0xF8; // Next byte is command
  }  
}    

// Set BL pin
void TextLCD_SPI_N_3_16::_setBL(bool value) {
    if (_bl) {
        _bl->write(value);   
    }    
}    

// Not used in this mode
void TextLCD_SPI_N_3_16::_setData(int value) {
}    

    
// Write a byte using SPI3 16 bits mode
void TextLCD_SPI_N_3_16::_writeByte(int value) {
    _cs = 0;
    wait_us(1);

    _spi->write(_controlbyte);

    _spi->write(value);     

    wait_us(1);
    _cs = 1;
}
  
//------- End TextLCD_SPI_N_3_16 ----------
#endif

#if(1)
//------- Start TextLCD_SPI_N_3_24 --------

 /** Create a TextLCD interface using a controller with a native SPI3 24 bits interface
   *
   * @param spi             SPI Bus
   * @param cs              chip select pin (active low)
   * @param type            Sets the panel size/addressing mode (default = LCD16x2)
   * @param bl              Backlight control line (optional, default = NC)  
   * @param ctrl            LCD controller (default = SSD1803) 
   */       
TextLCD_SPI_N_3_24::TextLCD_SPI_N_3_24(SPI *spi, PinName cs, LCDType type, PinName bl, LCDCtrl ctrl) :
                                       TextLCD_Base(type, ctrl), 
                                       _spi(spi),        
                                       _cs(cs) {      
        
  // Init CS
  _cs = 1;

  // Setup the spi for 8 bit data, low steady state clock,
  // rising edge capture, with a 500KHz or 1MHz clock rate  
  _spi->format(8,0);
  _spi->frequency(1000000);    
  
  // The hardware Backlight pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (bl != NC) {
    _bl = new DigitalOut(bl);   //Construct new pin 
    _bl->write(0);              //Deactivate    
  }
  else {
    // No Hardware Backlight pin       
    _bl = NULL;                 //Construct dummy pin     
  }  

  //Sanity check
  if (_ctrl & LCD_C_SPI3_24) {
    _init();      
  }
  else {
    error("Error: LCD Controller type does not support native SPI3 24 bits interface\n\r");           
  }
}

TextLCD_SPI_N_3_24::~TextLCD_SPI_N_3_24() {
   if (_bl != NULL) {delete _bl;}  // BL pin
}

// Not used in this mode
void TextLCD_SPI_N_3_24::_setEnable(bool value) {
}    

// Set RS pin
// Used for mbed pins, I2C bus expander or SPI shiftregister
void TextLCD_SPI_N_3_24::_setRS(bool value) {
// The 24bit mode is split in 3 bytes. The first byte is for synchronisation and controlbits. The controlbits define the meaning of the next two bytes.
// Each byte encodes 4 actual bits. The 8 actual bits represent either a data or a command byte.
//   b23 b22 b21 b20 b19 b18 b17 b16 -  b15 b14 b13 b12 b11 b10 b9 b8 - b7 b6 b5 b4 b3 b2 b1 b0 
//     1   1   1   1   1  RW  RS   0     d0  d1  d2  d3   0   0  0  0   d4 d5 d6 d7  0  0  0  0
//
//   RS=1 means that next byte is data, RS=0 means that next byte is command
//   RW=0 means that next byte is writen, RW=1 means that next byte is read (not used in this lib)
//
// Note: SPI3_24 expects LSB first. This is inconsistent with regular SPI convention (and hardware) that sends MSB first.

  if (value) {
    _controlbyte = 0xFA; // Next byte is data
  }
  else {
    _controlbyte = 0xF8; // Next byte is command
  }
   
}    

// Set BL pin
void TextLCD_SPI_N_3_24::_setBL(bool value) {
    if (_bl) {
        _bl->write(value);   
    }    
}    

// Not used in this mode
void TextLCD_SPI_N_3_24::_setData(int value) {
}    

//Mapping table to flip the bits around cause SPI3_24 expects LSB first.
const uint8_t map3_24[16] = {0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0};
    
// Write a byte using SPI3 24 bits mode
void TextLCD_SPI_N_3_24::_writeByte(int value) {
    _cs = 0;
    wait_us(1);
    _spi->write(_controlbyte);

    //Map and send the LSB nibble
    _spi->write( map3_24[value & 0x0F]);     

    //Map and send the MSB nibble
    _spi->write( map3_24[(value >> 4) & 0x0F]);     

    wait_us(1);
    _cs = 1;
}
  
//------- End TextLCD_SPI_N_3_24 ----------
#endif