// Edited by Fábio Henrique (oliveirafhm@gmail.com) - 22/01/2016
// Last modification: 26/05/2018
#ifndef UserDataType_h
#define UserDataType_h
const String headerList[] = {"u_time","battery_status","v1","v2","analog_pulse_input_a","digital_input_a","digital_input_b"};
const uint8_t analogInPinList[] = {0,1,2,3};
const uint8_t digitalInPinList[] = {8,9};
const uint8_t ADC_DIM = sizeof(analogInPinList)/sizeof(uint8_t);
const uint8_t DIGITAL_DIM = sizeof(digitalInPinList)/sizeof(uint8_t);
bool binaryToCsvFlag = false;
// Output pins: Analogic out , Digital out , Digital out
const uint8_t outputPinList[] = {DAC0,6,7};
bool outputSignal[] = {false, false, false};
//const uint8_t redLedPin = 2;// The same as ERROR_LED_PIN defined in main .ino file
const uint8_t greenLedPin = 3;
float sampleRate = 3000;
//Adjusted to Arduino Due (12 bits ADC) ~1.3v (after voltage divider)
const uint16_t batteryLevelThreshold = 0;//Test//1600;
unsigned short initBatteryLevel;
// 4 bytes, 2 bytes * 4 = 8 bytes, 2 bytes * 2 = 4 bytes -> 16 Bytes + commas
// = 16 + 6 = 22 Bytes per row (sample)
// 66000 Bytes per second (3000 Hz) = 528000 bits per second (also at 3 kHz)
struct data_t {
  unsigned long time;
  unsigned short adc[ADC_DIM];
  unsigned short digital[DIGITAL_DIM];
};
#endif  // UserDataType_h
