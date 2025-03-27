#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

#define DEVICE_NAME "PowerAdapter1"      // Name for the BLE device
#define ADC_PIN 4                        // ADC pin number for voltage measurement
#define LED_PIN 21                       // LED pin for status indication
#define ADV_INTERVAL 5000                // Advertisement interval in milliseconds
#define MAX_VOLTAGE 2.5                  // Maximum measurable voltage (in Volts)
#define ADC_REF_VOLTAGE 3.3              // ADC reference voltage

// BTHome v2 configuration constants
const uint16_t BTHOME_SERVICE_UUID = 0xFCD2;    // BTHome Service UUID
const uint8_t BTHOME_HEADER = 0x40;               // Single header for all objects in the packet
const uint8_t BTHOME_OBJECT_ID_VOLTAGE = 0x0C;      // Object ID for voltage sensor (value in 0.001 V)
const uint8_t BTHOME_OBJECT_ID_BATTERY = 0x01;      // Object ID for battery sensor (value as a percentage in uint8)

// Function prototypes
void updateAdvertisement(uint16_t voltage_mv);
float readVoltage();

void setup() {
  Serial.begin(115200);                     // Initialize serial communication for debugging
  pinMode(LED_PIN, OUTPUT);                 // Set LED pin as output
  pinMode(ADC_PIN, INPUT);                  // Set ADC pin as input
  digitalWrite(LED_PIN, LOW);               // Ensure LED is off initially

  // Configure ADC with 11dB attenuation (suitable for a 0-3.3V range)
  analogReadResolution(12);                 // Set ADC resolution to 12 bits
  analogSetPinAttenuation(ADC_PIN, ADC_11db); // Set the attenuation level

  // Initialize BLE device with the specified device name
  BLEDevice::init(DEVICE_NAME);

  // Get the BLE advertising instance and update the advertisement data
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  updateAdvertisement(0);
  pAdvertising->start();

  Serial.println("BTHome v2 Voltage/Battery Sensor started");
}

// readVoltage()
// Reads the raw ADC value from the defined ADC_PIN, converts it to voltage using the
// ADC reference voltage and resolution, and constrains the result to MAX_VOLTAGE.
float readVoltage() {
  int adcValue = analogRead(ADC_PIN);                     // Read the raw ADC value
  float voltage = (adcValue * ADC_REF_VOLTAGE) / 4095.0;    // Convert ADC value to voltage
  return constrain(voltage, 0.0, MAX_VOLTAGE);              // Limit voltage to MAX_VOLTAGE (2.5V)
}

// updateAdvertisement(uint16_t voltage_mv)
// Updates the BLE advertisement packet with sensor data using the BTHome v2 format.
// The packet includes two sensor objects: one for voltage and one for battery percentage.
// The battery percentage is derived from the measured voltage.
void updateAdvertisement(uint16_t voltage_mv) {
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advertisementData;
  std::string serviceData;

  // Add a single header for the entire packet (BTHome v2 requires only one header)
  serviceData += (char)BTHOME_HEADER;

  // 1) Voltage sensor (Object ID 0x0C): Send voltage in millivolts as two bytes (LSB, then MSB)
  serviceData += (char)BTHOME_OBJECT_ID_VOLTAGE;
  serviceData += (char)(voltage_mv & 0xFF);             // Least Significant Byte
  serviceData += (char)((voltage_mv >> 8) & 0xFF);        // Most Significant Byte

  // 2) Battery sensor (Object ID 0x01): Send battery percentage as a single byte
  // Calculate percentage based on 0V (0%) to MAX_VOLTAGE (2.5V equals 100%)
  int percent = (voltage_mv * 100) / 2500;
  if (percent < 0)   percent = 0;
  if (percent > 100) percent = 100;

  serviceData += (char)BTHOME_OBJECT_ID_BATTERY;
  serviceData += (char)percent;                           // Battery percentage as uint8_t

  // Configure BLE advertisement data with flags, service UUID, and the service data packet
  advertisementData.setFlags(0x06);
  advertisementData.setCompleteServices(BLEUUID(BTHOME_SERVICE_UUID));
  advertisementData.setServiceData(BLEUUID(BTHOME_SERVICE_UUID), serviceData);

  pAdvertising->setAdvertisementData(advertisementData);
}

void loop() {
  float voltage = readVoltage();                      // Read the voltage from ADC
  uint16_t voltage_mv = voltage * 1000;                 // Convert voltage (V) to millivolts

  // Update BLE advertisement by stopping, updating, and restarting the advertising process
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->stop();
  updateAdvertisement(voltage_mv);
  pAdvertising->start();

  // Blink LED for visual indication of an update
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  // Print ADC value and voltage to the serial monitor for debugging
  Serial.printf("ADC: %d | Voltage: %.3f V\n", analogRead(ADC_PIN), voltage);

  // Wait for the next advertisement update
  delay(ADV_INTERVAL);
}
