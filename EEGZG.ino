/*
    Brainwave-Controlled Zen Garden
    by Jeff Highsmith
    with communications code from NeuroSky
    
    This code helps an Arduino read the Meditation score from a Mindwave Headset and
    use that value to demonstrate the level of meditation by drawing in the sand.
*/

#include <LiquidCrystal.h>
#include <Servo.h> 

#define LOWERLIMIT 0
#define UPPERLIMIT 175
#define STEPSIZE 1
 
Servo rakeservo;
                
unsigned long steptimer = 0;
int stepdelay = 0;

boolean climbing = true; // direction of servo position number; increment if true
int pos = LOWERLIMIT;    // variable to store the servo position 

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

#define BAUDRATE 57600 
#define DEBUGOUTPUT 0

#define powercontrol 10

// checksum variables
byte generatedChecksum = 0;
byte checksum = 0; 
int payloadLength = 0;
byte payloadData[64] = {0};
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;

long lastReceivedPacket = 0;
boolean bigPacket = false;

//////////////////////////
// Microprocessor Setup //
//////////////////////////
void setup() {
  
  Serial.begin(9600);
  rakeservo.attach(3);
  Serial.begin(BAUDRATE);           
  
  lcd.begin(16, 2);
  lcd.print("   Brain Wave");
  lcd.setCursor(0,1);
  lcd.print("   Zen Garden");
  
  steptimer = millis(); // holds the time of the last servo movement
}

////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
byte ReadOneByte() {
  int ByteRead;

  while(!Serial.available());
  ByteRead = Serial.read();

#if DEBUGOUTPUT  
  Serial.print((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif

  return ByteRead;
}

/////////////
//MAIN LOOP//
/////////////
void loop() {

  if (stepdelay < (millis() - steptimer)) {
    steptimer = millis();
    rakeservo.write(pos);
    if (climbing) {
      pos += STEPSIZE;
      if (pos > UPPERLIMIT) climbing = false;
    } else {
      pos -= STEPSIZE;
      if (pos < LOWERLIMIT) climbing = true;
    }
  }
  
  // Look for sync bytes
  if(ReadOneByte() == 170) {
    if(ReadOneByte() == 170) {

      payloadLength = ReadOneByte();
      if(payloadLength > 169)                      //Payload length can not be greater than 169
          return;

      generatedChecksum = 0;        
      for(int i = 0; i < payloadLength; i++) {  
        payloadData[i] = ReadOneByte();            //Read payload into memory
        generatedChecksum += payloadData[i];
      }   

      checksum = ReadOneByte();                      //Read checksum byte from stream      
      generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum

        if(checksum == generatedChecksum) {    

        poorQuality = 200;
        attention = 0;
        meditation = 0;

        for(int i = 0; i < payloadLength; i++) {    // Parse the payload
          switch (payloadData[i]) {
          case 2:
            i++;            
            poorQuality = payloadData[i];
            bigPacket = true;            
            break;
          case 4:
            i++;
            attention = payloadData[i];                        
            break;
          case 5:
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i = i + 3;
            break;
          case 0x83:
            i = i + 25;      
            break;
          default:
            break;
          } // switch
        } // for loop

#if !DEBUGOUTPUT

        if (bigPacket) {  // The meditation value must only be read in bigPackets
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Attention: ");
          lcd.print(attention);
          lcd.setCursor(0, 1);
          lcd.print("Meditation: ");
          lcd.print(meditation);
          
          // The math here serves to accentuate the difference between meditating and not
          if (meditation < 20) stepdelay = (meditation);
          else if (meditation < 40) stepdelay = ((meditation - 20) * 2);
          else if (meditation < 60) stepdelay = ((meditation - 40) * 3);
          else stepdelay = (meditation * 4);
        }

#endif        
        bigPacket = false;        
      }
      else {
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte
}

