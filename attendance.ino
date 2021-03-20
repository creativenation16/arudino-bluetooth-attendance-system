 /* Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 */

#include <SPI.h>
#include <MFRC522.h>
#include <LCD_I2C.h>
#include <Time.h>
#include <SoftwareSerial.h>
#include <CmdParser.hpp>

SoftwareSerial BTSerial(5, 4); // RX | TX
LCD_I2C lcd(0x27); // Default address of most PCF8574 modules, change according

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

#define buttonPin       7          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;
CmdParser cmdParser;
int h = 12;
int m = 0;
int s = 0;
int flag = 0;
int TIME = 0;
boolean writemode = false;
byte writebuffer[32];
boolean inmode = true;
int buttonState = 0;         // variable for reading the pushbutton status
int oldbuttonState = 0;         // variable for reading the pushbutton status

void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  lcd.begin(); // If you are using more I2C devices using the Wire library use lcd.begin(false)
                 // this stop the library(LCD_I2C) from calling Wire.begin()
  lcd.backlight();
  BTSerial.begin(9600);
  pinMode(buttonPin, INPUT);

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
}

void writename()
{
    byte sector         = 1;
    byte blockAddr      = 4;
    byte trailerBlock   = 7;
    byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
    MFRC522::StatusCode status;
    byte buffer[32];
    byte size = sizeof(buffer);    
     // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    for (size_t i = 0; i < 16; i++) {
      dataBlock[i] = writebuffer[i];
    }
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr+1);
    Serial.println(F(" ..."));
    for (size_t i = 0; i < 16; i++) {
      dataBlock[i] = writebuffer[16+i];
    }
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr+1, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();    
    //dump_byte_array(writebuffer,32);
    
    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Show the whole sector as it currently is
    //Serial.println(F("Current data in sector:"));
   // mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    //Serial.println();

    // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();    
    // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr+1);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr+1, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr+1); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();        

    lcd.setCursor(0, 1);
    lcd.println("Write success   ");      
}

void readname(){
    byte sector         = 1;
    byte blockAddr      = 4;
    byte trailerBlock   = 7;
    byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
    MFRC522::StatusCode status;
    byte buffer[32];
    byte size = sizeof(buffer);      
    String namestr = "";
   // Authenticate using key A
    //Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Show the whole sector as it currently is
    //Serial.println(F("Current data in sector:"));
    //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    //Serial.println();

    // Read data from the block
    //Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    //Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    //Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    //dump_byte_array(buffer, 16); Serial.println();
    //Serial.println();    
    
    // Read data from the block
    //Serial.print(F("Reading data from block ")); Serial.print(blockAddr+1);
    //Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr+1, buffer+16, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    //Serial.print(F("Data in block ")); Serial.print(blockAddr+1); Serial.println(F(":"));
    //dump_byte_array(buffer+16, 16); Serial.println();
    //Serial.println();        
    namestr = String((char *)buffer);
    //Serial.println(namestr);
    lcd.setCursor(0, 1);
    lcd.println(namestr);
    String hexstring = "";

    for(int i = 0; i < 4; i++) {
      if(mfrc522.uid.uidByte[i] < 0x10) {
        hexstring += '0';
      }
      hexstring += String(mfrc522.uid.uidByte[i], HEX);
    }
    if (inmode)
      BTSerial.println(hexstring+","+namestr+",in");
    else
      BTSerial.println(hexstring+","+namestr+",out");
    //BTSerial.println("hello\n");
}


/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void showtime(){
    lcd.setCursor(0, 0);
    lcd.print(int(h/10));
    lcd.print(h%10);
    lcd.print(":"); 
    lcd.print(int(m/10));
    lcd.print(m%10);
    lcd.print(":");
    lcd.print(int(s/10));
    lcd.print(s%10);
    //if (flag < 12) lcd.print(" AM");
    //if (flag == 12) lcd.print(" PM");
    //if (flag > 12) lcd.print(" PM");
    //if (flag == 24) flag = 0;
    if (inmode){
      lcd.setCursor(13, 0);
      lcd.println("IN "); 
    } else {
      lcd.setCursor(13, 0);
      lcd.println("OUT"); 
    }
}

void showwritemode(){
    lcd.setCursor(0, 0);
    lcd.blink();
    lcd.println("Write Card Mode:");    
    lcd.setCursor(0, 1);
    lcd.blink();
    lcd.println("Tap Card !      ");      
}
void loop() {
  const int BUFF_SIZE = 32; // make it big enough to hold your longest command
  static char buffer[BUFF_SIZE+1]; // +1 allows space for the null terminator
  static int length = 0; // number of characters currently in the buffer

  
  buttonState = digitalRead(buttonPin);
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if ((buttonState == LOW)&&(oldbuttonState == HIGH)) {
    if (writemode==false){
      if (inmode){
        inmode = false;
      } else {
        inmode = true;
      }
    } else {
      writemode = false;
    }
  }
  oldbuttonState = buttonState;

  if (writemode==false){
    showtime();
  } else {
    showwritemode();
  }

  if (BTSerial.available())
  {
      char c = BTSerial.read();
      if((c == '\r') || (c == '\n'))
      {
          // end-of-line received
          if(length > 0)
          {
              if (cmdParser.parseCmd(buffer) != CMDPARSER_ERROR) {
                if (cmdParser.equalCommand_P(PSTR("TIME"))) { // set time
                 char *p = cmdParser.getCmdParam(1);
                 char *str;
                 int i = 0;
                   while ((str = strtok_r(p, ":", &p)) != NULL) {
                    if (i==0)
                      h = atoi(str);
                    else if (i==1)
                      m = atoi(str);
                    else if (i==2)
                      s = atoi(str);
                    i++;
                   }
                } else if (cmdParser.equalCommand_P(PSTR("NAME"))) { // enter write card mode and save the name
                  writemode = true;
                  const size_t count = cmdParser.getParamCount()+1;
                  for (size_t i = 0; i < 32; i++) {
                    writebuffer[i] = 0; // clear write buffer
                  }
                  int charcount = 0;
                  for (size_t i = 1; i < count; i++) {
                      String strword = cmdParser.getCmdParam(i);
                      for (size_t j = 0; j < strword.length(); j++) {
                        writebuffer[charcount] = strword[j];
                        charcount++;
                      }
                      if (i < count){
                        writebuffer[charcount] = byte(' '); // insert space
                        charcount++;
                      }
                  }
                } else if (cmdParser.equalCommand_P(PSTR("STOP"))) { // stop write card mode
                  writemode = false;
                  lcd.noBlink();
                  lcd.clear();
                }
              }
          }
          length = 0;
      }
      else
      {
          if(length < BUFF_SIZE)
          {
              buffer[length++] = c; // append the received character to the array
              buffer[length] = 0; // append the null terminator
          }
          else
          {
              // buffer full - discard the received character
          }
      }
  }

  delay(700);//delay(1000);
  lcd.clear();
  s = s + 1;
  if (s == 60)
  {
    s = 0;
    m = m + 1;
  }
  if (m == 60)
  {
    m = 0;
    h = h + 1;
    flag = flag + 1;
  }
  if (h == 24)
  {
    h = 0;
  }
  lcd.setCursor(0, 1);
  
   // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();  

    if (writemode){
      writename();
      writemode = false;
    } else {
      readname();
    }

    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}
