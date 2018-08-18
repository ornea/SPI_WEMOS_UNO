/*
   filename: SPI_Wemos_Master.ino (to be used in conjunction with SPI_Uno_Slave
   target: WEMOS (ESP8266) used as the MASTER SPI device (connected to a UNO SPI SLAVE)
   Date: 13 Aug 2018
   Author J.Richards

   Desc: Test sketch for MASTER SPI connection between a ESP8266 as MASTER and Arduino Uno as SLAVE.
         I struggled to understand other demo sketches as often it was not clear what device
         was performing which role.
         It was also not immediately clear as I saw SPI.transfer calls on both the MASTER
         and the SLAVE and could not see how the MASTER was requesting data from the slave.
         So I present this for those that may be in a similar situation as I was.
         If hooked up as shown a LED connected to pin 5 on the UNO SLAVE should flash
         on for half a second then off for half a second as it rec TOGGLE cmds from the MASTER.
         The MASTER Serial session should give responses of 48 or 49 for ASCII '0' and ASCII '1'
         which is the current state of pin 5 on the UNO.
         If it is not connected to anything the MASTER Console Terminal window will fill with 0's
         This is an indication that the MASTER is clocking in and reading 0's as there
         is no SLAVE controlling the MISO pin. i.e it will always be 0volts 


    Connect the SPI Master (ESP8266) to SLAVE (UNO) device. I used 1k resistors between the WEMOS
    and the UNO.

            MASTER          |  SLAVE
    GPIO    NodeMCU   Name  |   Uno
   ===================================
     16       D0       SS   |   D10
     13       D7      MOSI  |   D11
     12       D6      MISO  |   D12
     14       D5      SCK   |   D13

                            |   D5 -/\/\/--- LED->|- --- GND

  WARNING: I used 1k resistors to connect the 4 lines between the ESP8266 and the UNO.
         This is because the ESP8266 is 3.3v tolerant and the UNO is 5v
         I have plans to use a funky FET level converter circuit between the lines as I have found on youtubes but
         the FETs have not yet arrived.  However, I can verify that the 1k resistors work flawlessly
         over short distances with

              SPI.setClockDivider(SPI_CLOCK_DIV8);//WEMOS 2MHz Clock Maximum for the UNO

         however, initially you may wish to use the following slower clock setting

              SPI.setClockDivider(SPI_CLOCK_DIV128); WEMOS 123kHz

  Note1: Experiements clearly indicate the following:-
  1. You have to decide which pin the MASTER (ESP8266) will use to Select the SPI device. I used D0.
   You have to take care of asserting a LOW on this line to select the slave (UNO)
   You have to take care of asserting a HIGH on the line to deselect the Slave.
   I assumed it was part of the SPI library as the Arduino reference for SPI.begin() states
    Description
    Initializes the SPI bus by setting SCK, MOSI, and SS to outputs, pulling SCK and MOSI low, and SS high.
   Therefore I figured SS pin D8 on the ESP8266 would be configured as output and controlled accordingly.
   This is not the case. 

  2. When the UNO is configured as slave, you have no choice but to use the pins above
  i.e. D10,D11,D12,D13 for SS,MOSI,MISO, SCK respectively

  3. When the ESP8266 issues a transfer ie. SPI.transfer(byte); two things happen concurrently
  a. The value of byte is clocked out of (ESP8266) MOSI MSB bit first
  b. The value currently in the UNO SPDR register is also clocked out of the MISO UNO and clocked into the ESP8266
     If you never modify the SPDR register it will contain the value that was previously sent from the ESP8266
     So doing this on the ESP8266
        x = SPI.transfer(0xAA); //First Time x will most likely contain 0 and the (UNO) SPDR = 0xAA
        x = SPI.transfer(0xBB); //This time x will contain 0xAA and (UNO) SPDR = 0xBB

   Therefore, my solution below is to transfer the 4 bytes (32 bit command) as 4 separate transfers with
   a final dummy transfer like SPI.transfer(0); to get the final response from the UNO.

  Note2: I use an addressing scheme in case I run out of IO on the WEMOS for SS.  The idea is that it
  maybe possible to have many UNOs sitting on the SPI bus with thier SS pins tied low.  Then they will
  only respond if they see thier address how ever, it is a cludge and if the right combinations of
  bytes is sent out the SPI port the wrong UNO may respond. Un likley but it is worth mentioning.

   My reason for wanting to do this.
   I use WEMOS boards for most of my projects and more recently these projects are tech solutions for Escape Room
   props.
   The WEMOS is a great device with built in wifi but lacks IO.  My solution is to use a UNO as intelligent
   IO expansion.
   One such Escape Room project/prop required 5 x RFID readers, 1 x Reed switch, 1 x Relay and 1 x Audio Module.
   5 RFID readers required 5 pins for SS (D8,D3,D4,D2,D0) and D5,D6 & D7 for SPI Clock, MISO and MOSI respectively
   Relay required D1
   Audio module shared D8 used for serial.swap()
   and needed one more pin for the reed switch. Dang it.

   If I was to do it again I would use an ESP32. Job done, but the plug in ESP32 board manager
   for ARDUINO was still in its infancy at the time

*/
#include <SPI.h>

// Define the hierarchical command structure
// command consists of 32bits 0xXXXXYYZZ
// XXXX == "U1" in hex 0x5531 for Uno1 .  (This is for fun but maye helpful if you have run out of SS pins See Note2
// YY can be 'R' 'H' 'L' 'T' 'O' or 'I' or make up your own
// ZZ is typically a a pin number between 2 and 13.
// So to set pin 6 High on the UNO insert the following command -> send_cmd(cmdUNO1PINHIGH | 6);
// This will send HEX pattern 0x55314805 i.e. 'U' = 0x55, '1' = 0x31, 'H' = 0x48 and 6 is 06.

#define UNO1_ADDR ('U'<<24UL)| ('1'<<16UL) //Unique address for UNO See Note2
//For those unitiated with macros and << and >> the macro below decodes as follows:
// cmdUNO1PINREAD is the name of the macro. the keyword 'pin' is replaced by whatever is placed in the () when it is invoked
// whereever the keyword 'pin' appears in the macro definition it is replaced by the value it was invoked with
// A macro invoked as cmdUNO1PINREAD(5) would decode as send_cmd("U1R5")
// assuming p = "05" (UNO1_ADDR|('R'<<8UL)| pin) would equate to ("55310000" bit wise OR with "5200" bitwise OR with "05") resulting in "55315205"
// The 'UL' in '8UL' tells the compiler to upcast the literal '8' to a 32 bit unsigned integer.  This ensures the result is 32 bit.

#define cmdUNO1PINREAD(pin) send_cmd(UNO1_ADDR|('R'<<8UL)| pin) // ASCII R means want want to (R)ead a pin state
#define cmdUNO1PINHIGH(pin) send_cmd(UNO1_ADDR|('H'<<8UL)| pin) // ASCII H, set a pin (H)igh
#define cmdUNO1PINLOW(pin) send_cmd(UNO1_ADDR|('L'<<8UL)| pin) // ASCII L, set a Pin (L)ow
#define cmdUNO1PINTOG(pin) send_cmd(UNO1_ADDR|('T'<<8UL)| pin) // ASCII T, (T)oggle the state of a pin
#define cmdUNO1PINOUT(pin) send_cmd(UNO1_ADDR|('O'<<8UL)| pin) // ASCII 'O' set pin to (O)utput
#define cmdUNO1PININP(pin) send_cmd(UNO1_ADDR|('I'<<8UL)| pin) // ASCII 'I' set pin to (I)nput
#define cmdUNO1PINPWM(pin) send_cmd(UNO1_ADDR|('P'<<8UL)| pin) // ASCII 'P' set pin to (P)WM
#define cmdUNO1PINPWMW(val) send_cmd(UNO1_ADDR|('W'<<8UL)| val) // ASCII 'W' write the PWM value
#define cmdUNO1PINANALHIGH(pin) send_cmd(UNO1_ADDR|('A')<<8UL| pin) //ASCII 'A' get the (H)igh byte for Analog read 
#define cmdUNO1PINANALLOW(pin) send_cmd(UNO1_ADDR|('a')<<8UL| pin)   //ASCII 'a' get the (L)ow byte for Analog read


#define SS D0 //The Slave Select pin

void setup()
{
  Serial.begin(115200);
  pinMode(SS, OUTPUT); //On the ESP8266 in normal SPI mode we need to decide on SS and set it to output.
  digitalWrite(SS, HIGH); //deselect SPI device
  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV8);// works ok with the UNO but using a conservative slower speed.
  SPI.setClockDivider(SPI_CLOCK_DIV128);  //experiments indicate SPI.setClockDivider(SPI_CLOCK_DIV8) works
  delay(1000);
  //cmdUNO1PINPWM(5); //Uncomment this if you want to set pin 5 as a PWM output 
}

byte pwmval = 0;  //keep track of of a pwmval for writing used for experimenting 
void loop()
{
  Serial.println(cmdUNO1PINTOG(5)); //This sends the toggle pin 5 command to UNO
  
  //Serial.println(cmdUNO1PINPWMW(pwmer++)); //uncomment this to send analogWrite commands to the analog pin configured in setup
  // * Used for debugging on the scope.
  /*
    //See Note Above
    digitalWrite(SS, LOW);
    Serial.println(SPI.transfer(0xAF), HEX);
    digitalWrite(SS, HIGH);
  */
  //
  delay(500);
}
//Sends the 4 bytes in cmd MSB first.
//Then sends a final 0 to clock the response out of the UNO.

byte send_cmd(uint32_t cmd)
{
  byte resp = 0xFF; //Load with dummy 0xFF which indicates an error of some sort.
  //Serial.println(cmd, HEX);
  digitalWrite(SS, LOW);
  //Uncomment these and comment out the 4 lines below that for more debug goodness
  //Serial.println(SPI.transfer((cmd >> 24) & 0xFF), HEX);//Send the Left most Byte. Byte No 3
  //Serial.println(SPI.transfer((cmd >> 16) & 0xFF), HEX);//Send byte No 2
  //Serial.println(SPI.transfer((cmd >> 8) & 0xFF), HEX); //Send byte No 1
  //Serial.println(SPI.transfer(cmd & 0xFF), HEX); //Send the right most byte, byte No 0
  SPI.transfer((cmd >> 24) & 0xFF);//Send the Left most Byte. Byte No 3
  SPI.transfer((cmd >> 16) & 0xFF);//Send byte No 2
  SPI.transfer((cmd >> 8) & 0xFF); //Send byte No 1
  SPI.transfer(cmd & 0xFF); //Send the right most byte, byte No 0

  delay(10); //give the UNO time to prepare its response
  resp = (SPI.transfer(0)); //Send a dummy 0 value to clock out the response from the UNO into the resp variable
  digitalWrite(SS, HIGH); //De-select the UNO SPI SLAVE
  //Serial.print("The response:");
  //Serial.println(resp);
  return (resp);
}
