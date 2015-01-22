/* 
 Stepper Motor Control - one revolution
 
 This program drives a unipolar or bipolar stepper motor. 
 The motor is attached to digital pins 8 - 11 of the Arduino.
 
 The motor should revolve one revolution in one direction, then
 one revolution in the other direction.  
 
 
 Created 11 Mar. 2007
 Modified 30 Nov. 2009
 by Tom Igoe
 
 */

//#include <WProgram.h>

#include <Stepper.h>
#include <EEPROM.h>

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// for your motor           

//--------------------------------//
// EEPROM writing/reading support //
//--------------------------------//


const int EEPROM_MIN_ADDR = 0;
const int EEPROM_MAX_ADDR = 511;

//
// Initialize eeprom memory with
// the specified byte.
// Default value is 0xFF.
//
void eeprom_erase_all(byte b = 0xFF) {
  int i;

  for (i = EEPROM_MIN_ADDR; i <= EEPROM_MAX_ADDR; i++) {
    EEPROM.write(i, b);
  }
}

//
// Dump eeprom memory contents over serial port.
// For each byte, address and value are written.
//
void eeprom_serial_dump_column() {
  // counter
  int i;

  // byte read from eeprom
  byte b;

  // buffer used by sprintf
  char buf[10];

  for (i = EEPROM_MIN_ADDR; i <= EEPROM_MAX_ADDR; i++) {
    b = EEPROM.read(i);
    sprintf(buf, "%03X: %02X", i, b);
    Serial.println(buf);
  }
}

//
// Dump eeprom memory contents over serial port in tabular form.
// Each printed row shows the value of bytesPerRow bytes
// (by default 16).
//
void eeprom_serial_dump_table(int bytesPerRow = 16) {
  // address counter
  int i;

  // row bytes counter
  int j;

  // byte read from eeprom
  byte b;

  // temporary buffer for sprintf
  char buf[10];


  // initialize row counter
  j = 0;

  // go from first to last eeprom address
  for (i = EEPROM_MIN_ADDR; i <= EEPROM_MAX_ADDR; i++) {

    // if this is the first byte of the row,
    // start row by printing the byte address
    if (j == 0) {
      sprintf(buf, "%03X: ", i);
      Serial.print(buf);
    }

    // read current byte from eeprom
    b = EEPROM.read(i);

    // write byte in hex form
    sprintf(buf, "%02X ", b);

    // increment row counter
    j++;

    // if this is the last byte of the row,
    // reset row counter and use println()
    // to start a new line
    if (j == bytesPerRow) {
      j = 0;
      Serial.println(buf);
    }
    // else just print the hex value with print()
    else {
      Serial.print(buf);
    }
  }
}

//
// Returns true if the address is between the
// minimum and maximum allowed values,
// false otherwise.
//
// This function is used by the other, higher-level functions
// to prevent bugs and runtime errors due to invalid addresses.
//
boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}

//
// Writes a sequence of bytes to eeprom starting at the specified address.
// Returns true if the whole array is successfully written.
// Returns false if the start or end addresses aren't between
// the minimum and maximum allowed values.
// When returning false, nothing gets written to eeprom.
//
boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
  // counter
  int i;

  // both first byte and last byte addresses must fall within
  // the allowed range  
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }

  return true;
}

//
// Reads the specified number of bytes from the specified address into the provided buffer.
// Returns true if all the bytes are successfully read.
// Returns false if the star or end addresses aren't between
// the minimum and maximum allowed values.
// When returning false, the provided array is untouched.
//
// Note: the caller must ensure that array[] has enough space
// to store at most numBytes bytes.
//
boolean eeprom_read_bytes(int startAddr, byte array[], int numBytes) {
  int i;

  // both first byte and last byte addresses must fall within
  // the allowed range  
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    array[i] = EEPROM.read(startAddr + i);
  }

  return true;
}

//
// Writes an int variable at the specified address.
// Returns true if the variable value is successfully written.
// Returns false if the specified address is outside the
// allowed range or too close to the maximum value
// to store all of the bytes (an int variable requires
// more than one byte).
//
boolean eeprom_write_int(int addr, int value) {
  byte *ptr;

  ptr = (byte*)&value;
  return eeprom_write_bytes(addr, ptr, sizeof(value));
}

//
// Reads an integer value at the specified address.
// Returns true if the variable is successfully read.
// Returns false if the specified address is outside the
// allowed range or too close to the maximum vlaue
// to hold all of the bytes (an int variable requires
// more than one byte).
//
boolean eeprom_read_int(int addr, int* value) {
  return eeprom_read_bytes(addr, (byte*)value, sizeof(int));
}

//
// Writes a string starting at the specified address.
// Returns true if the whole string is successfully written.
// Returns false if the address of one or more bytes
// fall outside the allowed range.
// If false is returned, nothing gets written to the eeprom.
//
boolean eeprom_write_string(int addr, const char* string) {
  // actual number of bytes to be written
  int numBytes;

  // we'll need to write the string contents
  // plus the string terminator byte (0x00)
  numBytes = strlen(string) + 1;

  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

//
// Reads a string starting from the specified address.
// Returns true if at least one byte (even only the
// string terminator one) is read.
// Returns false if the start address falls outside
// or declare buffer size os zero.
// the allowed range.
// The reading might stop for several reasons:
// - no more space in the provided buffer
// - last eeprom address reached
// - string terminator byte (0x00) encountered.
// The last condition is what should normally occur.
//
boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
  // byte read from eeprom
  byte ch;

  // number of bytes read so far
  int bytesRead;

  // check start address
  if (!eeprom_is_addr_ok(addr)) {
    return false;
  }

  // how can we store bytes in an empty buffer ?
  if (bufSize == 0) {
    return false;
  }

  // is there is room for the string terminator only,
  // no reason to go further
  if (bufSize == 1) {
    buffer[0] = 0;
    return true;
  }

  // initialize byte counter
  bytesRead = 0;

  // read next byte from eeprom
  ch = EEPROM.read(addr + bytesRead);

  // store it into the user buffer
  buffer[bytesRead] = ch;

  // increment byte counter
  bytesRead++;

  // stop conditions:
  // - the character just read is the string terminator one (0x00)
  // - we have filled the user buffer
  // - we have reached the last eeprom address
  while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
    // if no stop condition is met, read the next byte from eeprom
    ch = EEPROM.read(addr + bytesRead);

    // store it into the user buffer
    buffer[bytesRead] = ch;

    // increment byte counter
    bytesRead++;
  }

  // make sure the user buffer has a string terminator
  // (0x00) as its last byte
  if ((ch != 0x00) && (bytesRead >= 1)) {
    buffer[bytesRead - 1] = 0;
  }

  return true;
}

//
// A pair of functions to show how long it takes to work with eeprom.
//
int start_time;
int stop_time;

void start_timing() {
  start_time = millis();
}

void print_elapsed() {
  stop_time = millis();
  Serial.print("Time elapsed (ms): ");
  Serial.println(stop_time - start_time);
}

const int BUFSIZE = 50;
char buf[BUFSIZE];

void erase_buffer() {
  for (int i = 0; i < BUFSIZE; i++) {
    buf[i] = 0;
  }
}

void writeDiagnostic(const char* diag) {
  strcpy(buf, diag);
  eeprom_write_string(200, buf);
  erase_buffer();
}

void readDiagnostic() {
  eeprom_read_string(200, buf, BUFSIZE);
  Serial.print("Diagnostic: '");
  Serial.print(buf);
  Serial.println("'");
  erase_buffer();
}

void writeRelativePosition(int steps) {
  eeprom_write_int(100, steps);
}

int readRelativePosition() {
  int relpos;
  eeprom_read_int(100, &relpos);
  return relpos;
}

//-----------------//
// Stepper support //
//-----------------//

int relative_steps = readRelativePosition();

void run(Stepper stepper, int um_distance, int rpm) {
  stepper.setSpeed(rpm);
  int steps = um_distance/5;
  stepper.step(steps);
  relative_steps = relative_steps+steps;
  writeRelativePosition(relative_steps);

  Serial.print(steps*5);
  Serial.print(" ");
  Serial.println(readRelativePosition()*5);
}

void toPosition(Stepper stepper, int um_relpos, int rpm) {
  run(stepper, um_relpos-relative_steps*5, rpm);
}

//--------------------------------------------//
// Running the Arduino for use with a stepper //
//--------------------------------------------//


Stepper myStepper(stepsPerRevolution, 8,9,10,11);

char WaitForChar()
{
  int c;

  do {
    c = Serial.read();
  } 
  while(c == -1);

  return (char)c;
}

int loopPrompt(Stepper myStepper, int sp)
{
  signed long int value = 0;

  boolean negative = false;
  boolean s = false;
  boolean test = false;
  boolean datainprocess = false;

  while(1)
  {
    /*Read a character as it comes in:*/
    char byteBuffer = WaitForChar();

    if (byteBuffer=='s') {
      if (!s && !datainprocess) {
        s = true;
        continue;
      }
      else
        return sp;
    }

    if (byteBuffer=='t') {
      if (!test && !datainprocess) {
        test = true;
        Serial.println("test");
        return sp;
      }
      else
        return sp;
    }

    if (byteBuffer=='-') {
      if (!negative && !datainprocess) {
        negative = true;
        continue;
      }
      else
        return sp;
    }

    if(byteBuffer >= '0' && byteBuffer <= '9') { //Is the character a digit?
      /*Yes, shift left 1 place (in decimal), and add integer value of character (ASCII value - 48)*/
      value = (value * 10) + (byteBuffer - '0');
      datainprocess = true;
    }
    else {
      /*No, stop and give us the value we have so far*/
      if (s) {
        signed int difference = value-sp;
        sp = value;
        Serial.print(difference);
        Serial.print(" ");
        Serial.println(sp);
        return sp;
      }
      if (negative) {
        value = 0 - value;
      }
      run(myStepper, value, sp);
      return sp;
    }
  }
}

int sp = 60;

void setup() {
  // initialize the serial port:
  Serial.begin(9600);
}

void loop() {
  sp = loopPrompt(myStepper, sp);
}

