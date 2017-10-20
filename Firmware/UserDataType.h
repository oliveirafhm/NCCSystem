//* Edited by Fábio Henrique (oliveirafhm@gmail.com) - 22/01/2016
#ifndef UserDataType_h
#define UserDataType_h
const String headerList[] = {"u_time","battery_status","v1a","v1b","v2a","v2b","analog_pulse_input_a","digital_input_a","digital_input_b"};
const uint8_t analogInPinList[] = {0,1,2,3,4,6};
const uint8_t digitalInPinList[] = {8,9};
const uint8_t ADC_DIM = sizeof(analogInPinList)/sizeof(uint8_t);
const uint8_t DIGITAL_DIM = sizeof(digitalInPinList)/sizeof(uint8_t);
bool binaryToCsvFlag = false;
// Output pins: Analogic out , Digital out , Digital out
const uint8_t outputPinList[] = {DAC0,6,7};
bool outputSignal[] = {false, false, false};
//const uint8_t redLedPin = 2;// The same as ERROR_LED_PIN defined in main .ino file
const uint8_t greenLedPin = 3;
float sampleRate = 200;
//Adjusted to Arduino Due (12 bits ADC) ~2.1v (after voltage divider)
const uint16_t batteryLevelThreshold = 2600;
unsigned short initBatteryLevel;
struct data_t {
  unsigned long time;
  unsigned short adc[ADC_DIM];
  unsigned short digital[DIGITAL_DIM];
};
#endif  // UserDataType_h
