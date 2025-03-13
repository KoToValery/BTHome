#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

#define DEVICE_NAME "PowerAdapter1"
#define ADC_PIN 4
#define LED_PIN 21
#define ADV_INTERVAL 5000       // Интервал на рекламации
#define MAX_VOLTAGE 2.5         // Максимално измервано напрежение
#define ADC_REF_VOLTAGE 3.3     // Референционно напрежение на ADC

// BTHome v2 конфигурация
const uint16_t BTHOME_SERVICE_UUID = 0xFCD2;
const uint8_t BTHOME_HEADER = 0x40;         // Един общ хедър за всички обекти
const uint8_t BTHOME_OBJECT_ID_VOLTAGE = 0x0C; // Напрежение (0.001 V)
const uint8_t BTHOME_OBJECT_ID_BATTERY = 0x01; // Батерия (uint8, %)

void updateAdvertisement(uint16_t voltage_mv);
float readVoltage();

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);

  // Конфигуриране на ADC с 11dB attenuation (0-3.3V диапазон)
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  // Инициализация на BLE
  BLEDevice::init(DEVICE_NAME);

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  updateAdvertisement(0);
  pAdvertising->start();

  Serial.println("BTHome v2 Voltage/Battery Sensor started");
}

float readVoltage() {
  int adcValue = analogRead(ADC_PIN);
  float voltage = (adcValue * ADC_REF_VOLTAGE) / 4095.0;
  return constrain(voltage, 0.0, MAX_VOLTAGE); // Ограничаване до 2.5V
}

void updateAdvertisement(uint16_t voltage_mv) {
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advertisementData;
  std::string serviceData;

  // Добавяме само един път хедъра за целия пакет
  serviceData += (char)BTHOME_HEADER;

  // 1) Сензор за напрежение (ID 0x0C) – 2 байта (LSB, MSB)
  serviceData += (char)BTHOME_OBJECT_ID_VOLTAGE;
  serviceData += (char)(voltage_mv & 0xFF);       // LSB
  serviceData += (char)((voltage_mv >> 8) & 0xFF); // MSB

  // 2) Сензор за батерия (ID 0x01) – 1 байт (0–100%)
  int percent = (voltage_mv * 100) / 2500; 
  if (percent < 0)   percent = 0;
  if (percent > 100) percent = 100;

  serviceData += (char)BTHOME_OBJECT_ID_BATTERY;
  serviceData += (char)percent; // uint8_t, интерпретира се като %

  // Подготовка на BLE рекламата
  advertisementData.setFlags(0x06);
  advertisementData.setCompleteServices(BLEUUID(BTHOME_SERVICE_UUID));
  advertisementData.setServiceData(BLEUUID(BTHOME_SERVICE_UUID), serviceData);

  pAdvertising->setAdvertisementData(advertisementData);
}

void loop() {
  float voltage = readVoltage();
  uint16_t voltage_mv = voltage * 1000; // Преобразуване в миливолтове

  // Обновяване на рекламата
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->stop();
  updateAdvertisement(voltage_mv);
  pAdvertising->start();

  // LED индикация
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  Serial.printf("ADC: %d | Voltage: %.3f V\n", analogRead(ADC_PIN), voltage);

  delay(ADV_INTERVAL);
}
