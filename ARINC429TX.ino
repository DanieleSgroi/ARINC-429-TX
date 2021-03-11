/**************************************************************************************************************************************
**
** Project: ARINC 429 DME (DME-40) Simulator 
** File:    ARINC429TX.INO
** Purpose: Main SW File
** 
** (C) 2019 by Daniele Sgroi - daniele.sgroi@gmail.com
**
** VERSION:
**  - December 08, 2019 - ALPHA 0.1 - D. Sgroi
**
** TARGET HW:
** - ATMEGA 328P @ 16 MHz / 5V (Arduino Pro Mini)
**
** SOFTWARE:
** - Arduino 1.8.9+ IDE
** 
**************************************************************************************************************************************/

#define _DEBUG_                          // enable verbose debug messages on console   

/**************************************************************************************************************************************
** DEFINES
**************************************************************************************************************************************/

#define VERSION                                 "1.0" // SW Version
#define DEBUG_BAUD                             115200 // Serial Port Baud rate
#define DELAY_CYCLES(n) __builtin_avr_delay_cycles(n) // 1 cycle @ 16 MHz = 62.5 nSec

/**************************************************************************************************************************************
** HW DEFINES - ARDUINO PRO MINI PINOUT
**************************************************************************************************************************************/

#define RXD                0
#define TXD                1
//                         2
//                         3
//                         4
//                         5
//                         6
//                         7
#define TXA_429_PIN        8 // PB1
#define TXB_429_PIN        9 // PB0
#define CLK_561_PIN       10 // PB2
#define DTA_561_PIN       11 // PB3
#define SYN_561_PIN       12 // PB4
//#define LED_BUILTIN     13 // PB5, no need to redefine LED_BUILTIN
//                        A0
//                        A1
//                        A2
//                        A3
#define I2C_SDA           A4
#define I2C_SCL           A5
//                        A6
//                        A7

/**************************************************************************************************************************************
** ARINC 429 DEFINES AND TYPES
**************************************************************************************************************************************/

#define TXA_429          PB1 // Pin 8
#define TXB_429          PB0 // Pin 9
#define HI_SPEED           1
#define LO_SPEED           0

union t429Word {
  struct {
     unsigned char ucLabel        :  8; 
     unsigned char ucSdi          :  2;
     unsigned long int ulData     : 19; 
     unsigned char ucStatus       :  2; 
     unsigned char ucParity       :  1;
  } tFields;
  struct {
     unsigned char ucLabel        :  8; // 202 octal
     unsigned char ucSdi          :  2;
     unsigned char ucMemory       :  1; // 1 = memory
     unsigned char ucForeground   :  1;
     unsigned int  ulDistance     : 16; // NM
     unsigned char ucSign         :  1; // Always 0, positive
     unsigned char ucSsm          :  2; // 0 = FW, 1 = NCD, 2 = TEST, 3 = NO 
     unsigned char ucParity       :  1; // ODD
  } tDmeDistance;
  unsigned long int ul429Data      : 32; // must be exact size of tFields struct above
};

static const unsigned char aucInvByte[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,     /* 0x00 - 0x07 */
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,     /* 0x08 - 0x0f */
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,     /* 0x10 - 0x17 */
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,     /* 0x18 - 0x1f */
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,     /* 0x20 - 0x27 */
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,     /* 0x28 - 0x2f */
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,     /* 0x30 - 0x37 */
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,     /* 0x38 - 0x3f */
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,     /* 0x40 - 0x47 */
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,     /* 0x48 - 0x4f */
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,     /* 0x50 - 0x57 */
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,     /* 0x58 - 0x5f */
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,     /* 0x60 - 0x67 */
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,     /* 0x68 - 0x6f */
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,     /* 0x70 - 0x77 */
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,     /* 0x78 - 0x7f */
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,     /* 0x80 - 0x87 */
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,     /* 0x88 - 0x8f */
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,     /* 0x90 - 0x97 */
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,     /* 0x98 - 0x9f */
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,     /* 0xa0 - 0xa7 */
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,     /* 0xa8 - 0xaf */
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,     /* 0xb0 - 0xb7 */
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,     /* 0xb8 - 0xbf */
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,     /* 0xc0 - 0xc7 */
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,     /* 0xc8 - 0xcf */
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,     /* 0xd0 - 0xd7 */
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,     /* 0xd8 - 0xdf */
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,     /* 0xe0 - 0xe7 */
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,     /* 0xe8 - 0xef */
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,     /* 0xf0 - 0xf7 */
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff      /* 0xf8 - 0xff */
};

/**************************************************************************************************************************************
** GLOBALS
**************************************************************************************************************************************/

t429Word DmeDistance;

/**************************************************************************************************************************************
** A429Out
**
** Shift Out 32 bit data in ARINC 429 format
** LSB first
** 12,5 kbps lo speed, 100 kbps hi speed
**
** TXA_429           8 // PB1 // PORTB |= _BV(PB1); // High // PORTB &= ~_BV(PB1); // Low
** TXB_429           9 // PB0 // PORTB |= _BV(PB0); // High // PORTB &= ~_BV(PB0); // Low 
** 
** spd : 0 = 12,5 kbps, 1 = 100 kbps 
**
**************************************************************************************************************************************/

void A429Out(uint32_t val, unsigned char spd) {
         
    noInterrupts();                                                        // Begin of time critical section
    //PORTB &= ~_BV(SYN_561) & ~_BV(DTA_561) & ~_BV(CLK_561);              // All Low
    for (uint8_t i = 0; i < 32; i++)  {
        if (val & (0x1)) {PORTB |= _BV(TXA_429);  PORTB &= ~_BV(TXB_429);} 
        else {PORTB |= _BV(TXB_429);  PORTB &= ~_BV(TXA_429);}             // LSB shiftout  
        (spd) ? DELAY_CYCLES(73) : DELAY_CYCLES(633);                      // Experimental fine tuning, 1 cycle = 0.0625 uSec                                 
        PORTB &= ~_BV(TXA_429) & ~_BV(TXB_429);                            // All Low
        (spd) ? DELAY_CYCLES(68) : DELAY_CYCLES(628);                      // Experimental fine tuning, 1 cycle = 0.0625 uSec          
        val = val >> 1;                                                    // LSB shiftout 
    }
    PORTB &= ~_BV(TXA_429) & ~_BV(TXB_429);                                // All Low
    interrupts();                                                          // End of time critical section

}

/**************************************************************************************************************************************
** setup
**
** run once at startup
**
**************************************************************************************************************************************/

void setup() {

  // setup debug serial port (FTDI)
  Serial.begin(DEBUG_BAUD, SERIAL_8N1);
  while (!Serial);       // Wait for the DEBUG serial port to come online

  // Print Version
  Serial.print(F("ARINC429TX - V"));
  Serial.print(VERSION);
  Serial.println(F("(C)2019 by daniele.sgroi@gmail.com"));

  // Init discretes
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TXA_429_PIN, OUTPUT);
  pinMode(TXB_429_PIN, OUTPUT);
 
  // SET Output discretes Defaults
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(TXA_429_PIN, LOW);
  digitalWrite(TXB_429_PIN, LOW);

}

/**************************************************************************************************************************************
** LOOP
**
** Run continuosly
**
**************************************************************************************************************************************/

void loop() {

  static unsigned int uiTxCount = 0;

  digitalWrite(LED_BUILTIN, HIGH);     

  // TODO: update data accordingly

  DmeDistance.tDmeDistance.ucLabel  = aucInvByte[0202]; // ARINC 429 DME Distance Label, octal 202, reversed to comply with standard
  DmeDistance.tDmeDistance.ucSdi = 1;                   // channel 1
  DmeDistance.tDmeDistance.ucMemory = 0;                // not in memory
  DmeDistance.tDmeDistance.ucForeground = 1;            // foreground
  DmeDistance.tDmeDistance.ulDistance = uiTxCount++;    // NM
  DmeDistance.tDmeDistance.ucSsm = 3;                   // NO
  DmeDistance.tDmeDistance.ucParity = __builtin_parity(DmeDistance.ul429Data); // ODD parity, Slooow...

#ifdef _DEBUG_
  Serial.print(F("ARINC 429 TX: 0x"));
  Serial.println(DmeDistance.ul429Data, HEX); 
#endif

  A429Out(DmeDistance.ul429Data, HI_SPEED);

  digitalWrite(LED_BUILTIN, LOW);

  delay(199);

}

/**************************************************************************************************************************************
** EOF
**************************************************************************************************************************************/
