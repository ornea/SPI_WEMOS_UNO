# SPI_WEMOS_UNO
Demo sketches for SPI with WEMOS master and UNO SLAVE

See Code comments for additional details.  Would always welcome suggestions on better ways to do this or if I have made any mistakes

Test sketches for SPI connection between a ESP8266 as master and Arduino Uno as slave.
 *       I struggled to understand other demo sketches as which device was which was ambiguous
 *       and it was not clear what was actually happening
 *       So I present this for those that may be in a similar situation as I was.
 *       If hooked up as shown a LED connected to pin 5 on the UNO should flash 
 *       on for a second then off for a second.
 *       The Serial session should give responses of 48 or 49 for ASCII '0' and ASCII '1'
 *       which is the current state of pin 5 on the UNO.
 *       If it is not connected to anything the Console Terminal window will fill with 0's
