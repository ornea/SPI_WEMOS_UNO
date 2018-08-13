// Writen by Written by Nick Gammon, modified lots by J.Richards
/*
   filename: SPI_Uno_Slave_v7.ino (to be used in conjunction with SPI_Uno_Slave_v6)
   target: Arduino UNO used as the SLAVE SPI device
   Date: 13 Aug 2018
   Author J.Richards

   Desc: Test sketch for SPI connection between a ESP8266 as SPI MASTER and Arduino Uno as SPI SLAVE.
         See SPI_Wemos_Master_v6.ino for details and explanation
         This code is intended to run on a Uno acting as a SPI SLAVE device

         It simply waits for bytes to be sent to it.  It then checks if a sequence of bytes matches as particular command as 
         described in the define statements above.

         If a match is found it then extracts out which pin is referenced and then sets SPDR register accordingly which
         will be read by the WEMOS on a subsequent SPI transfer.

*/
#include <SPI.h>

#define UNO1_ADDR (((uint32_t)'U')<<24)| (((uint32_t)'1')<<16)
#define cmdUNO1PINREAD (((uint32_t)'R')<<8) | UNO1_ADDR
#define cmdUNO1PINHIGH (((uint32_t)'H')<<8)|UNO1_ADDR
#define cmdUNO1PINLOW (((uint32_t)'L')<<8)|UNO1_ADDR
#define cmdUNO1PINTOG (((uint32_t)'T')<<8)|UNO1_ADDR
#define cmdUNO1PINOUT (((uint32_t)'O')<<8)|UNO1_ADDR
#define cmdUNO1PININP (((uint32_t)'I')<<8)|UNO1_ADDR
#define cmdUNO1PINANALHIGH (((uint32_t)'A')<<8)|UNO1_ADDR
#define cmdUNO1PINANALLOW (((uint32_t)'a')<<8)|UNO1_ADDR

uint32_t cmd = 0xffffffff;
byte c;
volatile boolean process_it;


void setup (void)
{
  Serial.begin (115200);   // debugging
  // turn on SPI in slave mode
  //SPCR |= bit (SPE);
  SPCR = (1 << SPE);
  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);

  Serial.println("Were Running");//Something to let you know the sketch is running

  // now turn on interrupts
  SPI.attachInterrupt();

}  // end of setup


// SPI interrupt routine
ISR (SPI_STC_vect)
{
  //This routine get the current SPI byte and left shifts it into the 32bit cmd variable

  c = SPDR;  // grab byte from SPI Data Register
  cmd = (cmd << 8) | c;
  process_it = true;


}  // end of interrupt routine SPI_STC_vect

// main loop - wait for flag set in interrupt routine
void loop (void)
{
  if (process_it)
  {
    //Uses a big SWITCH control structure.
    //Conventional wisdom is that we do the minimal in a Interrupt Routine so I do the checking here
 
    //I have commented the first Case statement. Hopefully it is obvious what the others are doing.
    switch (cmd & 0xFFFFFF00) //Examine the upper 3 bytes and see if they match only of the commands in the defines from above
    {
      case cmdUNO1PINREAD:  // Do the upper 3 bytes look like it was a PIN Read request.
        {
          byte pin = (cmd & 0xFF); //Extract out which pin is getting read
          if (pin >= 2 && pin <= 13) //Make sure it is a valid pin
          {
            pinMode(pin, INPUT);      //As this is a SPI test demo, just make sure it works i.e this is a read command set the pin to INPUT
            SPDR = digitalRead(pin) + '0'; //Setting this register with the current state of the PIN in question' Add '0' to send as ASCII
                                           // This value will get read by the ESP8266 on the subsequent SPI transfer that it does.
          }
          else
          {
            SPDR = 0xFF; // pin not in range, send and error value FF
          }
        }
        break;
      case cmdUNO1PINHIGH:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 2 && pin <= 13)
          {
            digitalWrite(pin, HIGH);
            SPDR = 1; //OK
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;
      case cmdUNO1PINLOW:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 2 && pin <= 13)
          {
            digitalWrite(pin, LOW);
            SPDR = 1; //OK
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;
      case cmdUNO1PINOUT:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 2 && pin <= 13)
          {
            pinMode(pin, OUTPUT);
            SPDR = 1; //OK
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;
      case cmdUNO1PININP:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 2 && pin <= 13)
          {
            pinMode(pin, INPUT);
            SPDR = 1; //OK
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;
      case cmdUNO1PINTOG:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 2 && pin <= 13)
          {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, !digitalRead(pin));
            SPDR = digitalRead(pin) + '0';
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;
      case cmdUNO1PINANALHIGH:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 0 && pin <= 5) //Analog pins are 0 - 5 on the UNO
          {
            SPDR = (analogRead(pin) >> 8 );
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;

      case cmdUNO1PINANALLOW:
        {
          byte pin = (cmd & 0xFF);
          if (pin >= 0 && pin <= 5)
          {
            SPDR = (analogRead(pin) & 0x00FF);
          }
          else
          {
            SPDR = 0xFF; //ERR
          }
        }
        break;
      default:
        // if nothing else matches, do the default
        {
          SPDR = c;//0xFF;
        }
        // default is optional
        break;
    }
    process_it = false;
  }  // end of flag set

}  // end of loop
