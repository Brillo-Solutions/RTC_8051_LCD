/*DS3231 & DS1307 compatible driver program shows content on LCD via PCF8574A extender*/

#define DEV_ADDR_LCD 0x70    // I2C address of PCF8574A extender
#define DEV_ADDR_RTC 0xD0    // I2C address of DS3231 and DS1307

sbit sclPin at P3.B0;        // I2C serial clock line
sbit sdaPin at P3.B1;        // I2C serial data line

char backLight, mArr[8];
const char *dayOfWeek[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"}; // Array of pointers contains days of week

void startI2C()          // I2C start condition
{
  sdaPin = 1;
  sclPin = 1;
  sdaPin = 0;
  sclPin = 0;
}

void stopI2C()           // I2C stop condition
{
  sclPin = 0;
  sdaPin = 0;
  sclPin = 1;
  sdaPin = 1;
}

void writeToSlave(char mByte) // Writing byte to I2C slave
{
 char k;
 for(k = 0; k <= 8; k++)
 {
   sdaPin = mByte & 0x80 ? 1 : 0;
   sclPin = 1;
   sclPin = 0;
   mByte <<= 1;
 }
}

char readFromSlave(char mByte) // Reading byte from I2C slave
{
 char k;
 mByte = 0;
 for(k = 0; k < 8; k++)
 {
   sdaPin = 1;
   sclPin = 1;
   mByte <<= 1;
   mByte |= sdaPin;
   sclPin = 0;
 }
 return mByte;
}

void writeToI2C(char mAddr, char mByte)
{
  startI2C();
  writeToSlave(mAddr);
  writeToSlave(mByte);
  stopI2C();
}

char readFromI2C(char mAddr, char mIndex)
{
  char mByte;
  startI2C();
  writeToSlave(mAddr);
  writeToSlave(mIndex);
  startI2C();
  writeToSlave(mAddr + 1);
  mByte = readFromSlave(mIndex);
  stopI2C();
  delay_ms(5);
  return mByte;
}

void sendToDisplay(char mByte)
{
  writeToI2C(DEV_ADDR_LCD, mByte);
}

void brkInstByte(char mByte)
{
  char nByte;
  nByte = mByte & 0xF0;
  sendToDisplay(nByte | (backLight | 0x04));
  delay_ms(1);
  sendToDisplay(nByte | (backLight | 0x00));
  nByte = ((mByte << 4) & 0xF0);
  sendToDisplay(nByte | (backLight | 0x04));
  delay_ms(1);
  sendToDisplay(nByte | (backLight | 0x00));
}

void brkDataByte(char mByte)
{
  char nByte;
  nByte = mByte & 0xF0;
  sendToDisplay(nByte | (backLight | 0x05));
  delay_ms(1);
  sendToDisplay(nByte | (backLight | 0x01));
  nByte = ((mByte << 4) & 0xF0);
  sendToDisplay(nByte | (backLight | 0x05));
  delay_ms(1);
  sendToDisplay(nByte | (backLight | 0x01));
}

void putChar(char mByte, char rn, char cp)
{
  char rowAddr;

  if (rn == 1)
    rowAddr = 0x80; // First row of 16x2 LCD
  else
    rowAddr = 0xC0; // Second row of 16x2 LCD

  brkInstByte(rowAddr + (cp - 1));  // Send 8-bit LCD commands in 4-bit mode
  brkDataByte(mByte);  // Send 8-bit LCD data in 4-bit mode
}

void putString(const char *p, char rn, char cp)
{
  char rowAddr;

  if (rn == 1)
    rowAddr = 0x80; // First row of 16x2 LCD
  else
    rowAddr = 0xC0; // Second row of 16x2 LCD

  brkInstByte(rowAddr + (cp - 1));  // Send 8-bit LCD commands in 4-bit mode

  while (*p != '\0')
  {
    brkDataByte(*p);  // Send 8-bit LCD data in 4-bit mode
    p++;
  }
}

void initDisplay()
{
  char k, mArr[4] = {0x02, 0x28, 0x01, 0x0C};  // LCD 4-bit mode initialization commands
  for (k = 0; k < 4; k++)
    brkInstByte(mArr[k]);  // Send 8-bit LCD commands in 4-bit mode
    
  putString("--:--:--", 1, 5);
  putString("--/--/-- ---", 2, 3);
}

void setBackLight(int mBool)
{
  if (mBool)
    backLight = 0x08;  // Turn ON backlight of LCD
  else
    backLight = 0x00;  // Turn OFF backlight of LCD
}

void readRtc()
{
  char k, kt;
  
  for (k = 0; k < 7; k++)
      mArr[k] = readFromI2C(DEV_ADDR_RTC, k);
      
  kt = mArr[0];

  for (k = 1; k < 7; k++)
      mArr[k - 1] = mArr[k];

  mArr[k - 1] = kt;
}

void showTime()
{
  char m, n;
  for (m = 0, n = 12; m < 3; m++) // Showing time on 1st row of 16x2 LCD
  {
    putChar(((mArr[m] & 0x0F) | 0x30), 1, n--);
    putChar(((mArr[m] >> 4) | 0x30), 1, n--);
    if (n > 4)
      putChar(':', 1, n--);
  }
}

void showDate()
{
  char m, n;
  for (m = 4, n = 3; m < 7; m++)  // Showing date on 2nd row of 16x2 LCD
  {
    putChar(((mArr[m] >> 4) | 0x30), 2, n++);
    putChar(((mArr[m] & 0x0F) | 0x30), 2, n++);
    if (n < 11)
      putchar('/', 2, n++);
  }
}

void showDayOfWeek()  // showing day-of-week on 2nd row of LCD
{
  putString(dayOfWeek[mArr[3] - 1], 2, 12);
}

void main()
{
   setBackLight(1);           // Turning ON backlight
   initDisplay();             // Initializing LCD for 4-bit mode
   for (;;)
   {
     readRtc();               // Reading bytes from DS3231 or DS1307
     showTime();
     showDate();
     showDayOfWeek();
   }
}