/**
 * Edited by Fábio Henrique (oliveirafhm@gmail.com) - 22/01/2016
 * Last modification: 26/05/2018
 * 
 * This program logs data to a binary file.  Functions are included
 * to convert the binary file to a csv text file.
 *
 * Samples are logged at regular intervals.  The maximum logging rate
 * depends on the quality of your SD card and the time required to
 * read sensor data.  This example has been tested at 500 Hz with
 * good SD card on an Uno.  4000 HZ is possible on a Due.
 *
 * If your SD card has a long write latency, it may be necessary to use
 * slower sample rates.  Using a Mega Arduino helps overcome latency
 * problems since 13 512 byte buffers will be used.
 *
 * Data is written to the file using a SD multiple block write command.
 */
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
//------------------------------------------------------------------------------
// User data functions.  Modify these functions for your data items.
#include "UserDataType.h"  // Edit this include file to change data_t.

// Acquire a data record.
void acquireData(data_t* data) {
  data->time = micros();
  for (int i = 0; i < ADC_DIM; i++) {
    data->adc[i] = analogRead(analogInPinList[i]);
  }
  for (int i = 0; i < DIGITAL_DIM; i++) {
    data->digital[i] = digitalRead(digitalInPinList[i]);
  }
}

// Print a data record.
void printData(Print* pr, data_t* data) {
  pr->print(data->time);
  for (int i = 0; i < ADC_DIM; i++) {
    pr->write(',');
    pr->print(data->adc[i]);
  }
  for (int i = 0; i < DIGITAL_DIM; i++) {
    pr->write(',');
    pr->print(data->digital[i]);
  }
  pr->println();
}

// Print data header.
void printHeader(Print* pr) {
  pr->print(headerList[0]);//time
  for (int i = 1; i < (ADC_DIM+DIGITAL_DIM+1); i++) {
    pr->print(F(","));
    pr->print(headerList[i]);
  }
  pr->println();
}
//==============================================================================
// Start of configuration constants.
//==============================================================================
//Interval between data records in microseconds.
uint32_t LOG_INTERVAL_USEC = (1/sampleRate)*1000000;
//------------------------------------------------------------------------------
// Pin definitions.
//
// SD chip select pin.
const uint8_t SD_CS_PIN = 4;
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 2;
//------------------------------------------------------------------------------
// File definitions.
//
// Maximum file size in blocks.
// The program creates a contiguous file with FILE_BLOCK_COUNT 512 byte blocks.
// This file is flash erased using special SD commands.  The file will be
// truncated if logging is stopped early.
const uint32_t FILE_BLOCK_COUNT = 256000;

// log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "PS"
//------------------------------------------------------------------------------
// Buffer definitions.
//
// The logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional
// buffers.
//
#ifndef RAMEND
// Assume ARM. Use total of nine 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 8;
//
#elif RAMEND < 0X8FF
#error Too little SRAM
//
#elif RAMEND < 0X10FF
// Use total of two 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 1;
//
#elif RAMEND < 0X20FF
// Use total of five 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 4;
//
#else  // RAMEND
// Use total of 13 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 12;
#endif  // RAMEND
//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "tmp_log.bin"

// Size of file base name.  Must not be larger than six.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

SdFat sd;

SdBaseFile binFile;

char binName[13] = FILE_BASE_NAME "000.bin";

// Number of data records in a block.
const uint16_t DATA_DIM = (512 - 4)/sizeof(data_t);

//Compute fill so block size is 512 bytes.  FILL_DIM may be zero.
const uint16_t FILL_DIM = 512 - 4 - DATA_DIM*sizeof(data_t);

struct block_t {
  uint16_t count;
  uint16_t overrun;
  data_t data[DATA_DIM];
  uint8_t fill[FILL_DIM];
};

const uint8_t QUEUE_DIM = BUFFER_BLOCK_COUNT + 2;

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
uint8_t fullHead;
uint8_t fullTail;

// Advance queue index.
inline uint8_t queueNext(uint8_t ht) {
  return ht < (QUEUE_DIM - 1) ? ht + 1 : 0;
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) errorFlash(F(msg))
//------------------------------------------------------------------------------
void errorFlash(const __FlashStringHelper* msg) {
  sd.errorPrint(msg);
  fatalBlink();
}
//------------------------------------------------------------------------------
//
void fatalBlink() {
  while (true) {
    if (ERROR_LED_PIN >= 0) {
      digitalWrite(ERROR_LED_PIN, HIGH);
      delay(200);
      digitalWrite(ERROR_LED_PIN, LOW);
      delay(200);
    }//else break;
  }
}
//==============================================================================
// Convert binary file to csv file.
void binaryToCsv() {
  uint8_t lastPct = 0;
  block_t block;
  uint32_t t0 = millis();
  uint32_t syncCluster = 0;
  SdFile csvFile;
  char csvName[13];

  if (!binFile.isOpen()) {
    Serial.println();
    Serial.println(F("No current binary file"));
    return;
  }
  binFile.rewind();
  // Create a new csvFile.
  strcpy(csvName, binName);
  strcpy(&csvName[BASE_NAME_SIZE + 4], "csv");//Test

  if (!csvFile.open(csvName, O_WRITE | O_CREAT | O_TRUNC)) {
    error("open csvFile failed");
  }
  //Serial.println();
  Serial.print(F("Writing: "));
  Serial.print(csvName);
  Serial.println(F(" - type any character to stop"));
  printHeader(&csvFile);
  uint32_t tPct = millis();
  while (!Serial.available() && binFile.read(&block, 512) == 512) {
    uint16_t i;
    if (block.count == 0) {
      break;
    }
    if (block.overrun) {
      csvFile.print(F("OVERRUN,"));
      csvFile.println(block.overrun);
    }
    for (i = 0; i < block.count; i++) {
      printData(&csvFile, &block.data[i]);
    }
    if (csvFile.curCluster() != syncCluster) {
      csvFile.sync();
      syncCluster = csvFile.curCluster();
    }
    if ((millis() - tPct) > 1000) {
      uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
      if (pct != lastPct) {
        tPct = millis();
        lastPct = pct;
        Serial.print(pct, DEC);
        Serial.println('%');
      }
    }
    if (Serial.available()) {
      break;
    }
  }
  csvFile.close();
  Serial.print(F("Done: "));
  Serial.print(0.001*(millis() - t0));
  Serial.println(F(" Seconds"));
  binaryToCsvFlag = false;
}
//------------------------------------------------------------------------------
// read data file and check for overruns
void checkOverrun() {
  bool headerPrinted = false;
  block_t block;
  uint32_t bgnBlock, endBlock;
  uint32_t bn = 0;

  if (!binFile.isOpen()) {
    Serial.println();
    Serial.println(F("No current binary file"));
    return;
  }
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
    error("contiguousRange failed");
  }
  binFile.rewind();
  Serial.println();
  Serial.println(F("Checking overrun errors - type any character to stop"));
  while (binFile.read(&block, 512) == 512) {
    if (block.count == 0) {
      break;
    }
    if (block.overrun) {
      if (!headerPrinted) {
        Serial.println();
        Serial.println(F("Overruns:"));
        Serial.println(F("fileBlockNumber,sdBlockNumber,overrunCount"));
        headerPrinted = true;
      }
      Serial.print(bn);
      Serial.print(',');
      Serial.print(bgnBlock + bn);
      Serial.print(',');
      Serial.println(block.overrun);
    }
    bn++;
  }
  if (!headerPrinted) {
    Serial.println(F("No errors found"));
  } else {
    Serial.println(F("Done"));
  }
}
//------------------------------------------------------------------------------
// dump data file to Serial
void dumpData() {
  block_t block;
  if (!binFile.isOpen()) {
    Serial.println();
    Serial.println(F("No current binary file"));
    return;
  }
  binFile.rewind();
  Serial.println();
  Serial.println(F("Type any character to stop"));
  delay(1000);
  printHeader(&Serial);
  while (!Serial.available() && binFile.read(&block , 512) == 512) {
    if (block.count == 0) {
      break;
    }
    if (block.overrun) {
      Serial.print(F("OVERRUN,"));
      Serial.println(block.overrun);
    }
    for (uint16_t i = 0; i < block.count; i++) {
      printData(&Serial, &block.data[i]);
    }
  }
  Serial.println(F("Done"));
}
//------------------------------------------------------------------------------
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;
void logData() {
  uint32_t bgnBlock, endBlock;

  // Allocate extra buffer space.
  block_t block[BUFFER_BLOCK_COUNT];
  block_t* curBlock = 0;
  Serial.println();

  // Find unused file name.
  // Max file number is 999, after that you should delete older files by hand
  // Test code: https://repl.it/@oliveirafhm/GrumpyGrowingLoop
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(binName)) {
    if (binName[BASE_NAME_SIZE + 2] != '9') {
      binName[BASE_NAME_SIZE + 2]++;      
    } else if (binName[BASE_NAME_SIZE + 1] != '9') {
      binName[BASE_NAME_SIZE + 1]++;
      binName[BASE_NAME_SIZE + 2] = '0';      
    } else {
      binName[BASE_NAME_SIZE + 1] = '0';
      binName[BASE_NAME_SIZE + 2] = '0';
      if (binName[BASE_NAME_SIZE] == '9') {
        error("Can't create file name");
      }
      binName[BASE_NAME_SIZE]++;      
    }
  }
  // Delete old tmp file.
  if (sd.exists(TMP_FILE_NAME)) {
    Serial.println(F("Deleting tmp file"));
    if (!sd.remove(TMP_FILE_NAME)) {
      error("Can't remove tmp file");
    }
  }
  // Create new file.
  Serial.println(F("Creating new file"));
  binFile.close();
  if (!binFile.createContiguous(sd.vwd(),
                                TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) {
    error("createContiguous failed");
  }
  // Get the address of the file on the SD.
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
    error("contiguousRange failed");
  }
  // Use SdFat's internal buffer.
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) {
    error("cacheClear failed");
  }

  // Flash erase all data in the file.
  Serial.println(F("Erasing all data"));
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) {
    endErase = bgnErase + ERASE_SIZE;
    if (endErase > endBlock) {
      endErase = endBlock;
    }
    if (!sd.card()->erase(bgnErase, endErase)) {
      error("erase failed");
    }
    bgnErase = endErase + 1;
  }
  // Start a multiple block write.
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) {
    error("writeBegin failed");
  }
  // Initialize queues.
  emptyHead = emptyTail = 0;
  fullHead = fullTail = 0;

  // Use SdFat buffer for one block.
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);

  // Put rest of buffers in the empty queue.
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) {
    emptyQueue[emptyHead] = &block[i];
    emptyHead = queueNext(emptyHead);
  }
  // Give SD time to prepare for big write.
  delay(1000);
  // Serial.println(F("Type 1 to start logging | After, type 2 to stop"));
  Serial.println(F("Type 1 to start logging"));
  while(1){
    if (Serial.available()) {
      char c = tolower(Serial.read());
      if (c == '1'){
        break;
      }
    }
  }
  Serial.println(F("Logging... - type 2 to stop\n"));
  // Wait for Serial Idle.
  Serial.flush();
  delay(10);
  for(uint8_t i = 0; i < sizeof(outputSignal)/sizeof(bool); i++){
    if(outputSignal[i] && i == 0){
      analogWrite(outputPinList[i], 4095);//3.3v
    }else if(outputSignal[i]){
      digitalWrite(outputPinList[i], HIGH);//3.3v
    }
  }
  uint32_t bn = 0;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overrun = 0;
  uint32_t overrunTotal = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;
  int32_t diff;
  // Start at a multiple of interval.
  uint32_t logTime = micros()/LOG_INTERVAL_USEC + 1;
  logTime *= LOG_INTERVAL_USEC;
  bool closeFile = false;
  while (1) {
    // Time for next data record.
    logTime += LOG_INTERVAL_USEC;
    if (Serial.available()) {
      char c = tolower(Serial.read());
      if (c == '2'){
        closeFile = true;
        for(uint8_t i = 0; i < sizeof(outputSignal)/sizeof(bool); i++){
          if(outputSignal[i] && i == 0){
            analogWrite(outputPinList[i], 0);//0v
          }else if(outputSignal[i]){
            digitalWrite(outputPinList[i], LOW);//0v
          }
        }
      }
    }

    if (closeFile) {
      if (curBlock != 0 && curBlock->count >= 0) {
        // Put buffer in full queue.
        fullQueue[fullHead] = curBlock;
        fullHead = queueNext(fullHead);
        curBlock = 0;
      }
    } else {
      if (curBlock == 0 && emptyTail != emptyHead) {
        curBlock = emptyQueue[emptyTail];
        emptyTail = queueNext(emptyTail);
        curBlock->count = 0;
        curBlock->overrun = overrun;
        overrun = 0;
      }
      do {
        diff = logTime - micros();
      } while(diff > 0);
      if (diff < -10) {
        error("LOG_INTERVAL_USEC too small");
      }
      if (curBlock == 0) {
        overrun++;
      } else {
        acquireData(&curBlock->data[curBlock->count++]);
        if (curBlock->count == DATA_DIM) {
          fullQueue[fullHead] = curBlock;
          fullHead = queueNext(fullHead);
          curBlock = 0;
        }
      }
    }

    if (fullHead == fullTail) {
      // Exit loop if done.
      if (closeFile) {
        break;
      }
    } else if (!sd.card()->isBusy()) {
      // Get address of block to write.
      block_t* pBlock = fullQueue[fullTail];
      fullTail = queueNext(fullTail);
      // Write block to SD.
      uint32_t usec = micros();
      if (!sd.card()->writeData((uint8_t*)pBlock)) {
        error("write data failed");
      }
      usec = micros() - usec;
      t1 = millis();
      if (usec > maxLatency) {
        maxLatency = usec;
      }
      count += pBlock->count;

      // Add overruns and possibly light LED.
      if (pBlock->overrun) {
        overrunTotal += pBlock->overrun;
        if (ERROR_LED_PIN >= 0) {
          digitalWrite(ERROR_LED_PIN, HIGH);
        }
      }
      // Move block to empty queue.
      emptyQueue[emptyHead] = pBlock;
      emptyHead = queueNext(emptyHead);
      bn++;
      if (bn == FILE_BLOCK_COUNT) {
        // File full so stop
        break;
      }
    }
  }
  if (!sd.card()->writeStop()) {
    error("writeStop failed");
  }
  // Truncate file if recording stopped early.
  if (bn != FILE_BLOCK_COUNT) {
    Serial.println(F("Truncating file"));
    if (!binFile.truncate(512L * bn)) {
      error("Can't truncate file");
    }
  }
  if (!binFile.rename(sd.vwd(), binName)) {
    error("Can't rename file");
  }
  Serial.print(F("File renamed: "));
  Serial.println(binName);
  Serial.print(F("Max block write usec: "));
  Serial.println(maxLatency);
  Serial.print(F("Record time sec: "));
  Serial.println(0.001*(t1 - t0), 3);
  Serial.print(F("Sample count: "));
  Serial.println(count);
  Serial.print(F("Samples/sec: "));
  Serial.println((1000.0)*count/(t1-t0));
  Serial.print(F("Overruns: "));
  Serial.println(overrunTotal);
  Serial.println(F("Done"));
  binaryToCsvFlag = true;
}
//------------------------------------------------------------------------------
void setup(void) {
  if (ERROR_LED_PIN >= 0) {
    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, LOW);
  }
  if(greenLedPin >= 0){
    pinMode(greenLedPin, OUTPUT);
    digitalWrite(greenLedPin, HIGH);
  }
  // Modify ADC and DAC resolution (it works only for arduino due)
  analogReadResolution(12);
  analogWriteResolution(12);
  //Check initial battery level
  initBatteryLevel = analogRead(analogInPinList[0]);
  if(initBatteryLevel < batteryLevelThreshold){
    fatalBlink();
  }
  //Sets digital pins to input
  for (uint8_t i = 0; i < (sizeof(digitalInPinList)/sizeof(uint8_t)); i++)
  {
    pinMode(digitalInPinList[i], INPUT);
  }
  Serial.begin(115200);
  while (!Serial) {}
  // Serial.println(initBatteryLevel);//Test
  Serial.println(F("Ready"));
  // Serial.println(F("Initial setup\n"));
  // Serial.println(F("Waiting for setup parameters, please use the follow format (without spaces):\n"));
  // //|on or off using binary digit|integer value to represent sample rate|#
  // Serial.println(F("| output signal (3 bytes) | sample rate (integer value up to 4000 Hz) |#\n"));
  // Serial.println();

  while(!Serial.available()) {}
  const uint8_t setupLength = 11;
  char setupBuffer[setupLength];
  Serial.readBytesUntil('#', setupBuffer, setupLength);
  //Serial.println(setupBuffer);
  //Serial.println("ok2");

  char sampleRateBuffer[4];
  for(uint8_t i = 0; i < setupLength; i++){
    if(setupBuffer[i] == '|'){
      continue;
    }
    // Output signal configuration
    if(i == 1 && setupBuffer[i] == '1' && setupBuffer[i-1] == '|'){
      outputSignal[0] = true;
      Serial.println(F("- Analog output pulse turned on (DAC0)"));
    }else if(i == 2 && setupBuffer[i] == '1' && setupBuffer[i-2] == '|'){
      outputSignal[1] = true;
      pinMode(outputPinList[1], OUTPUT);
      digitalWrite(outputPinList[1], LOW);
      Serial.println(F("- Digital output pulse turned on (Pin 6)"));
    }else if(i == 3 && setupBuffer[i] == '1' && setupBuffer[i-3] == '|'){
      outputSignal[2] = true;
      pinMode(outputPinList[2], OUTPUT);
      digitalWrite(outputPinList[2], LOW);
      Serial.println(F("- Digital output pulse turned on (Pin 7)"));
    }
    // End of output signal configuration
    // Sample rate configuration
    else if(i > 4){
      sampleRateBuffer[i-5] = setupBuffer[i];
    }
  }
  sampleRate = atof(sampleRateBuffer);
  if (sampleRate == 0){
      sampleRate = 3000;
  }
  LOG_INTERVAL_USEC = (1/sampleRate)*1000000;
  // Serial.println();
  // Serial.println(sampleRate);
  // Serial.println(LOG_INTERVAL_USEC);
  // End of sample rate configuration

  //Create another file than bin and csv to store details about the trial received from QT GUI

  // Serial.print(F("\nFreeRam: "));
  // Serial.println(FreeRam());
  // Serial.print(F("Records/block: "));
  // Serial.println(DATA_DIM);
  if (sizeof(block_t) != 512) {
    error("Invalid block size");
  }
  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
    sd.initErrorPrint();
    fatalBlink();
  }
}
//------------------------------------------------------------------------------
void loop(void) {
  // discard any input
  while (Serial.read() >= 0) {}
  if(binaryToCsvFlag){
    Serial.println();
    binaryToCsv();// Convert .bin file to .csv
  }
  initBatteryLevel = analogRead(analogInPinList[0]);
  // Serial.println(initBatteryLevel);
  if(initBatteryLevel < batteryLevelThreshold){
    //Serial.println(F("Low battery, change it and reset to continue."));
    fatalBlink();
  }
  Serial.println(F("Loop menu - type:"));
  Serial.println(F("d - dump data to Serial"));
  Serial.println(F("e - overrun error details"));
  Serial.println(F("r - record data"));
  while(!Serial.available()) {}
  char c = tolower(Serial.read());
  // Discard extra Serial data.
  do {
    delay(10);
  } while (Serial.read() >= 0);
  if (ERROR_LED_PIN >= 0) {
    digitalWrite(ERROR_LED_PIN, LOW);
  }
  if (c == 'd') {
    dumpData();
  } else if (c == 'e') {
    checkOverrun();
  } else if (c == 'r') {
    logData();
  } else {
    Serial.println(F("Invalid entry"));
  }
}
