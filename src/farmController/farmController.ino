#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <DHT.h>
#include <ModbusMaster.h>


// ========== ПИНЫ ==========
// MOSFET (ШИМ для светодиодов)
const int mosfetPins[4] = {7, 6, 5, 4};
// DHT11
const int dhtPins[2] = {2, 3};
// Реле
const int relayPowerPin = 11;   // реле питания датчиков Проверить что подключение и включение\выключение ТОЧНО так же как и для реkе realyLampPin
const int relayPumpPin = 10;    // реле помпы Проверить что подключение и включение\выключение ТОЧНО так же как и для реkе realyLampPin
const int relayCameraPin = 9;   // реле камеры(Рабочее)
const int relayLamp1Pin = 12;     // реле лампочки(норм)
const int relayLamp2Pin = 25;     // реле лампочки(норм)
// Датчик расхода
const int flowSensorPin = 8;
// Датчик давления
const int pressurePin = A5;
// Емкостные датчики почвы
const int NUM_CAP_SENSORS = 5;
const int soilPins[NUM_CAP_SENSORS] = {A0, A1, A2, A3, A4}; // добавим A6 для шестого датчика
// SD-карта
const int sdCSPin = 53;
// RS485
const int rs485TxPin = 18;  // TX1
const int rs485RxPin = 19;  // RX1

// ========== НАСТРОЙКИ ДАТЧИКОВ ==========
// Датчик расхода
#define PULSES_PER_LITER   553.0
// Давление
#define VREF           5.0
// Емкостные датчики почвы (калибровка)
const int soilAirValue[NUM_CAP_SENSORS] = {620, 625, 610, 615, 475};
const int soilWaterValue[NUM_CAP_SENSORS] = {275, 280, 265, 272, 199};
// DHT
#define DHT_TYPE DHT11
// Modbus
#define MODBUS_SLAVE_ID  1
#define MODBUS_BAUD      9600

// ========== НАСТРОЙКИ ПОЛИВА ==========
#define LOW_MOISTURE_THRESHOLD  60   // ниже этого включаем помпу (%)
#define HIGH_MOISTURE_THRESHOLD 80   // выше этого выключаем помпу (%)
#define MEASURE_INTERVAL         10800 // измерение раз в 3 часа (сек)
#define WATERING_CHECK_INTERVAL  10 // проверка влажности во время полива 10 секунд (сек)
#define MAX_WATERING_TIME        600 // макс время полива 10 минут (сек)
#define SENSOR_STABILIZE_DELAY   2  // задержка после включения питания датчиков 2секунд (сек)
#define PHOTO_DURATION         180 // время работы камеры 5 минут (сек)

// ========== ОБЪЕКТЫ ==========
RTC_DS3231 rtc;
DHT dht1(dhtPins[0], DHT_TYPE);
DHT dht2(dhtPins[1], DHT_TYPE);
ModbusMaster modbusNode;
File dataFile;

// ========== ДАТА НАЧАЛА ЭКСПЕРИМЕНТА ==========
DateTime experimentStart;  // дата начала эксперимента

// Структура для настройки фотосессии в рамках фазы
struct PhotoConfig {
  uint16_t DurationSec;
  bool enableStart;
  uint8_t startDelayMin;   // за сколько минут до включения (утром)
  
  bool enableMiddle;
   // в 16:00
  bool enableEnd;
  uint8_t endBeforeMin;       // через сколько минут после выключения (вечером)
  
};

void initExperimentStart() {
  // Установите здесь реальную дату начала вашего эксперимента гг мм дд чч мм сс
  experimentStart = DateTime(2026, 4, 4, 0, 0, 0);
}

int getExperimentDay() {
  DateTime now = rtc.now();
  TimeSpan diff = now - experimentStart;
  return diff.days() + 1; // дни с 1
}

// ========== БЛОК УПРАВЛЕНИЯ СВЕТОМ (ДВЕ СЕКЦИИ) ==========
// Структура фазы для одной секции (без времени, оно общее)
struct PhaseSec {
  uint8_t startDay;
  uint8_t endDay;        // 0 = бесконечно
  uint8_t brightness[2]; // яркость для двух каналов секции
};

// Массивы фаз для двух секций
PhaseSec phasesSec1[] = {
  {1, 0, {255, 255}},   // секция 1: дни 1-10, яркость 100%
};

PhaseSec phasesSec2[] = {
  {1, 5,  {100, 100}},   // секция 2: дни 1-5
  {6, 15, {150, 150}},   // дни 6-15
  {16, 0, {200, 200}}    // с 16 дня
};

// Количество фаз в каждой секции
const int numPhasesSec1 = sizeof(phasesSec1) / sizeof(phasesSec1[0]);
const int numPhasesSec2 = sizeof(phasesSec2) / sizeof(phasesSec2[0]);

// Текущие фазы для каждой секции
int currentPhaseSec1 = -1;
int currentPhaseSec2 = -1;

// Общее время включения/выключения света (для обеих секций)
#define LIGHT_ON_HOUR    8
#define LIGHT_ON_MINUTE  0
#define LIGHT_OFF_HOUR   24
#define LIGHT_OFF_MINUTE 0



// Привязка фотосессий к дням
struct PhotoSchedule {
  uint8_t startDay;
  uint8_t endDay;
  PhotoConfig config;
};

PhotoSchedule photoSchedules[] = {
  {1, 10, {PHOTO_DURATION,true, 5, true, true, 5}},
  {11, 20, {PHOTO_DURATION,true, 5, true, true, 5}},
  {21, 0, {PHOTO_DURATION,true, 5, true, true, 5}}
};
const int numPhotoSchedules = sizeof(photoSchedules) / sizeof(photoSchedules[0]);

// Переменные состояния для фото
enum LightState { LIGHT_NORMAL, LIGHT_PHOTO };
LightState lightState = LIGHT_NORMAL;
unsigned long photoStartTime = 0;
unsigned long photoDuration = 0;
bool photoStartDone = false, photoMiddleDone = false, photoEndDone = false;
uint8_t prevBrightness[4]; // резерв яркости перед фото

// Функция получения текущей фото-конфигурации по дню
PhotoConfig getPhotoConfigForDay(int day) {
  for (int i = 0; i < numPhotoSchedules; i++) {
    if (day >= photoSchedules[i].startDay && (photoSchedules[i].endDay == 0 || day <= photoSchedules[i].endDay)) {
      return photoSchedules[i].config;
    }
  }
  PhotoConfig empty = {0, false, 0, false, false, 0};
  return empty;
}

// Функции обновления фазы для каждой секции
void updatePhaseSec1(int day) {
  for (int i = 0; i < numPhasesSec1; i++) {
    if (day >= phasesSec1[i].startDay && (phasesSec1[i].endDay == 0 || day <= phasesSec1[i].endDay)) {
      if (currentPhaseSec1 != i) {
        currentPhaseSec1 = i;
        Serial.print("Section1 phase: "); Serial.println(i);
      }
      return;
    }
  }
  if (currentPhaseSec1 == -1 && numPhasesSec1 > 0) currentPhaseSec1 = 0;
}

void updatePhaseSec2(int day) {
  for (int i = 0; i < numPhasesSec2; i++) {
    if (day >= phasesSec2[i].startDay && (phasesSec2[i].endDay == 0 || day <= phasesSec2[i].endDay)) {
      if (currentPhaseSec2 != i) {
        currentPhaseSec2 = i;
        Serial.print("Section2 phase: "); Serial.println(i);
      }
      return;
    }
  }
  if (currentPhaseSec2 == -1 && numPhasesSec2 > 0) currentPhaseSec2 = 0;
}

// Функция проверки, должен ли свет гореть (общее время)
bool isLightTime(DateTime now) {
  uint32_t nowMin = now.hour() * 60 + now.minute();
  uint32_t onMin = LIGHT_ON_HOUR * 60 + LIGHT_ON_MINUTE;
  uint32_t offMin = LIGHT_OFF_HOUR * 60 + LIGHT_OFF_MINUTE;
  if (offMin == 0) offMin = 24 * 60; // 24:00 -> 1440
  if (onMin <= offMin) {
    return (nowMin >= onMin && nowMin < offMin);
  } else {
    return (nowMin >= onMin || nowMin < offMin);
  }
}

// Установка яркости для секции 1 (пины 7,6)
void setSection1(uint8_t ch0, uint8_t ch1) {
  analogWrite(mosfetPins[0], ch0);
  analogWrite(mosfetPins[1], ch1);
}

// Установка яркости для секции 2 (пины 5,4)
void setSection2(uint8_t ch0, uint8_t ch1) {
  analogWrite(mosfetPins[2], ch0);
  analogWrite(mosfetPins[3], ch1);
}

// Установка всех каналов
void setAllBrightness(uint8_t s1ch0, uint8_t s1ch1, uint8_t s2ch0, uint8_t s2ch1) {
  setSection1(s1ch0, s1ch1);
  setSection2(s2ch0, s2ch1);
}

// Применить яркость согласно текущим фазам
void applyBrightness() {
  if (currentPhaseSec1 >= 0) {
    setSection1(phasesSec1[currentPhaseSec1].brightness[0], phasesSec1[currentPhaseSec1].brightness[1]);
  } else {
    setSection1(0, 0);
  }
  if (currentPhaseSec2 >= 0) {
    setSection2(phasesSec2[currentPhaseSec2].brightness[0], phasesSec2[currentPhaseSec2].brightness[1]);
  } else {
    setSection2(0, 0);
  }
}

// Получить текущие яркости всех каналов (для сохранения перед фото)
void getCurrentBrightness(uint8_t *buf) {
  if (currentPhaseSec1 >= 0) {
    buf[0] = phasesSec1[currentPhaseSec1].brightness[0];
    buf[1] = phasesSec1[currentPhaseSec1].brightness[1];
  } else {
    buf[0] = buf[1] = 0;
  }
  if (currentPhaseSec2 >= 0) {
    buf[2] = phasesSec2[currentPhaseSec2].brightness[0];
    buf[3] = phasesSec2[currentPhaseSec2].brightness[1];
  } else {
    buf[2] = buf[3] = 0;
  }
}

void handleLighting() {
  DateTime now = rtc.now();
  int day = getExperimentDay();

  // Обновляем фазы при смене дня
  static int lastDay = 0;
  if (day != lastDay) {
    updatePhaseSec1(day);
    updatePhaseSec2(day);
    lastDay = day;
    // Сбрасываем флаги фото для нового дня
    photoStartDone = photoMiddleDone = photoEndDone = false;
  }

  bool lightShouldBeOn = isLightTime(now);
  PhotoConfig pc = getPhotoConfigForDay(day);

  // Если не в режиме фото
  if (lightState == LIGHT_NORMAL) {
    // Устанавливаем яркость в соответствии с фазами (если свет должен гореть)
    if (lightShouldBeOn) {
      applyBrightness();
    } else {
      setAllBrightness(0, 0, 0, 0);
    }

    uint32_t nowMinutes = now.hour() * 60 + now.minute();
    uint32_t onMinutes = LIGHT_ON_HOUR * 60 + LIGHT_ON_MINUTE;
    uint32_t offMinutes = LIGHT_OFF_HOUR * 60 + LIGHT_OFF_MINUTE;
    if (offMinutes == 0) offMinutes = 24 * 60;

    // Фото в начале (за startDelayMin минут до включения)
    if (pc.enableStart && !photoStartDone) {
      int32_t startPhotoTime = onMinutes - pc.startDelayMin;
      // Если уходим в предыдущий день
      if (startPhotoTime < 0) startPhotoTime += 24*60;
      if (nowMinutes == startPhotoTime) {
        getCurrentBrightness(prevBrightness);
        setAllBrightness(0, 0, 0, 0);
        digitalWrite(relayCameraPin, LOW);
        digitalWrite(relayLamp1Pin, LOW);
        delay(3000);
        digitalWrite(relayLamp2Pin, LOW);
        photoStartTime = millis();
        photoDuration = pc.DurationSec * 1000UL;
        lightState = LIGHT_PHOTO;
        photoStartDone = true;
        Serial.println("Morning photo");
        return;
      }
    }

    // Фото в середине (16:00)
    if (pc.enableMiddle && !photoMiddleDone) {
      uint32_t middleTime = 16 * 60; // 16:00
      if (nowMinutes == middleTime) {
        getCurrentBrightness(prevBrightness);
        setAllBrightness(0, 0, 0, 0);
        digitalWrite(relayCameraPin, LOW);
        digitalWrite(relayLamp1Pin, LOW);
        delay(3000);
        digitalWrite(relayLamp2Pin, LOW);
        photoStartTime = millis();
        photoDuration = pc.DurationSec * 1000UL;
        lightState = LIGHT_PHOTO;
        photoMiddleDone = true;
        Serial.println("Middle photo");
        return;
      }
    }

    // Фото в конце (через endBeforeMin минут после выключения)
    if (pc.enableEnd && !photoEndDone) {
      uint32_t endPhotoTime = offMinutes + pc.endBeforeMin;
      if (endPhotoTime >= 24*60) endPhotoTime -= 24*60;
      if (nowMinutes == endPhotoTime) {
        getCurrentBrightness(prevBrightness);
        setAllBrightness(0, 0, 0, 0);
        digitalWrite(relayCameraPin, LOW);
        digitalWrite(relayLamp1Pin, LOW);
        delay(3000);
        digitalWrite(relayLamp2Pin, LOW);
        photoStartTime = millis();
        photoDuration = pc.DurationSec * 1000UL;
        lightState = LIGHT_PHOTO;
        photoEndDone = true;
        Serial.println("Evening photo");
        return;
      }
    }
  }
  else if (lightState == LIGHT_PHOTO) {
    if (millis() - photoStartTime >= photoDuration) {
      digitalWrite(relayCameraPin, HIGH); //ВЫКЛЮЧИЛ СВЕТ
      digitalWrite(relayLamp1Pin, HIGH);
      digitalWrite(relayLamp2Pin, HIGH);
      // Восстанавливаем яркость, если свет должен гореть
      if (isLightTime(now)) {
        setAllBrightness(prevBrightness[0], prevBrightness[1], prevBrightness[2], prevBrightness[3]);
      } else {
        setAllBrightness(0, 0, 0, 0);
      }
      lightState = LIGHT_NORMAL;
      Serial.println("Photo ended");
    }
  }
}

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
// Данные датчиков (последние считанные значения)
struct SensorData {
  float temp1, hum1, temp2, hum2;
  float pressureMPa;
  float flowRate;
  float totalVolume;
  int soilMoisture[NUM_CAP_SENSORS];          // 0..100%
  float N, P, K, pH, soilMoistureModbus, soilTemp, EC;
  bool modbusValid;
} data;

// Для прерывания расходомера
volatile unsigned long flowPulseCount = 0;
bool flowInterruptEnabled = false;

// Состояния конечного автомата полива
enum SystemState {
  IDLE,
  MEASURE,
  WATERING
};
SystemState state = IDLE;

// Таймеры
unsigned long lastMeasureTime = 0;
unsigned long wateringStartTime = 0;
unsigned long lastWateringCheck = 0;

// Флаг, что мы уже выполнили измерение перед поливом (чтобы не дублировать)
bool measuredBeforeWatering = false;

// ========== ПРОТОТИПЫ ==========
void setupPins();
void initModules();
bool initSD();
float getAverageMoisture();
String getTimeString();
void powerSensorsOn();
void powerSensorsOff();
void readAllSensors();
void readDHT();
void readPressure();
void readFlow();
void readSoilCapacitive();
void readModbus();
void logToSD();

// Прерывание расходомера
void flowPulseCounter() {
  if (flowInterruptEnabled) {
    flowPulseCount++;
  }
}

void enableFlowInterrupt() {
  if (!flowInterruptEnabled) {
    flowInterruptEnabled = true;
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowPulseCounter, RISING);
  }
}

void disableFlowInterrupt() {
  if (flowInterruptEnabled) {
    flowInterruptEnabled = false;
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(57600);
  while (!Serial) delay(10);
  Serial.println("=== Farm Controller Starting ===");

  setupPins();
  initModules();
  initExperimentStart();   // установка даты начала эксперимента

  // Начальное состояние
  state = IDLE;
  lastMeasureTime = millis() - MEASURE_INTERVAL * 1000UL; // чтобы сразу запустить измерение
  Serial.println("Setup complete.");
}

void setupPins() {
  for (int i = 0; i < 4; i++) {
    pinMode(mosfetPins[i], OUTPUT);
    analogWrite(mosfetPins[i], 0);
  }
  pinMode(relayPowerPin, OUTPUT);
  pinMode(relayPumpPin, OUTPUT);
  pinMode(relayCameraPin, OUTPUT);
  pinMode(relayLamp1Pin, OUTPUT);
  pinMode(relayLamp2Pin, OUTPUT);
  digitalWrite(relayPowerPin, LOW);
  digitalWrite(relayPumpPin, LOW);
  digitalWrite(relayCameraPin, HIGH);
  digitalWrite(relayLamp1Pin, HIGH);
  digitalWrite(relayLamp2Pin, HIGH);
  pinMode(flowSensorPin, INPUT_PULLUP);
}

void initModules() {
  // RTC
  Serial.print("RTC...");
  if (!rtc.begin()) {
    Serial.println(" not found!");
  } else {
    Serial.println(" OK");
    if (rtc.lostPower()) {
      Serial.println("RTC lost power, setting compile time");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  // DHT
  dht1.begin();
  dht2.begin();
  // Modbus
  Serial1.begin(MODBUS_BAUD);
  modbusNode.begin(MODBUS_SLAVE_ID, Serial1);
  // SD инициализируем позже, перед записью
}

bool initSD() {
  if (!SD.begin(sdCSPin)) {
    Serial.println("SD init failed");
    return false;
  }
  // Создаём заголовок, если файла нет
  if (!SD.exists("datalog.csv")) {
    dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println("DateTime,Temp1,Hum1,Temp2,Hum2,PressureMPa,FlowRate,TotalVolume,Soil1,Soil2,Soil3,Soil4,Soil5,N,P,K,pH,SoilMoistureModbus,SoilTemp,EC");
      dataFile.close();
      Serial.print("DATA:");
      Serial.println("DateTime,Temp1,Hum1,Temp2,Hum2,PressureMPa,FlowRate,TotalVolume,Soil1,Soil2,Soil3,Soil4,Soil5,N,P,K,pH,SoilMoistureModbus,SoilTemp,EC");
    } else {
      return false;
    }
  }
  return true;
}

String getTimeString() {
  DateTime now = rtc.now();
  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(buffer);
}

void powerSensorsOn() {
  digitalWrite(relayPowerPin, HIGH);
  delay(SENSOR_STABILIZE_DELAY * 1000UL); // ждём стабилизацию
}

void powerSensorsOff() {
  digitalWrite(relayPowerPin, LOW);
  // Отключаем прерывание расходомера, так как датчик обесточен
  disableFlowInterrupt();
}

// ========== ЧТЕНИЕ ДАТЧИКОВ ==========
void readAllSensors() {
  readDHT();
  readPressure();
  readSoilCapacitive();
  readModbus();
  // Расход читаем отдельно по прерыванию, но можем получить текущее значение
  // (flowRate обновляется в readFlow)
}

void readDHT() {
  data.temp1 = dht1.readTemperature();
  data.hum1 = dht1.readHumidity();
  if (isnan(data.temp1) || isnan(data.hum1)) {
    data.temp1 = data.hum1 = -999;
  }
  data.temp2 = dht2.readTemperature();
  data.hum2 = dht2.readHumidity();
  if (isnan(data.temp2) || isnan(data.hum2)) {
    data.temp2 = data.hum2 = -999;
  }
}

void readPressure() {
  int raw = analogRead(pressurePin);
  float voltage = raw * VREF / 1023.0;
  data.pressureMPa = (voltage - 0.5) / 3.75;
}

void readFlow() {
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last >= 1000) {
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    data.flowRate = (pulses * 60.0) / PULSES_PER_LITER;
    data.totalVolume += pulses / PULSES_PER_LITER;
    last = now;
    if (flowInterruptEnabled) {
      attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowPulseCounter, RISING);
    }
  }
}

void readSoilCapacitive() {
  for (int i = 0; i < NUM_CAP_SENSORS; i++) {
    int raw = analogRead(soilPins[i]);
    int percent = map(raw, soilAirValue[i], soilWaterValue[i], 0, 100);
    data.soilMoisture[i] = constrain(percent, 0, 100);
  }
}

float getAverageMoisture() {
  float sum = 0;
  int count = 0;
  for (int i = 0; i < NUM_CAP_SENSORS; i++) {
    sum += data.soilMoisture[i];
    count++;
  }
  if (data.modbusValid) {
    sum += data.soilMoistureModbus;
    count++;
  }
  return sum / count;
}

void readModbus() {

  // Объявляем структуру с параметрами точно как в test_only_data.ino

  struct SensorParam {

    const char* name;

    uint16_t addr;

    float (*converter)(uint16_t);

  };



  // Вспомогательные функции-конвертеры

  auto asIs = [](uint16_t val) -> float { return val; };

  auto div10 = [](uint16_t val) -> float { return val / 10.0; };

  auto div100 = [](uint16_t val) -> float { return val / 100.0; };



  SensorParam params[] = {

    {"N", 0x001E, asIs},

    {"P", 0x001F, asIs},

    {"K", 0x0020, asIs},

    {"pH", 0x0006, div100},

    {"Moist", 0x0012, div10},

    {"Temp", 0x0013, div10},

    {"EC", 0x0015, asIs}

  };

  const int numParams = sizeof(params) / sizeof(params[0]);



  // Флаг успешности (хотя бы один параметр прочитан)

  bool anySuccess = false;



  for (int i = 0; i < numParams; i++) {

    uint8_t result = modbusNode.readHoldingRegisters(params[i].addr, 1);



    if (result == modbusNode.ku8MBSuccess) {

      uint16_t raw = modbusNode.getResponseBuffer(0);

      float value = params[i].converter(raw);



      // Записываем в соответствующее поле структуры data

      switch (i) {

        case 0: data.N = value; break;

        case 1: data.P = value; break;

        case 2: data.K = value; break;

        case 3: data.pH = value; break;

        case 4: data.soilMoistureModbus = value; break;

        case 5: data.soilTemp = value; break;

        case 6: data.EC = value; break;

      }

      anySuccess = true;

    } else {

      // При ошибке записываем -999 в соответствующее поле

      switch (i) {

        case 0: data.N = -999; break;

        case 1: data.P = -999; break;

        case 2: data.K = -999; break;

        case 3: data.pH = -999; break;

        case 4: data.soilMoistureModbus = -999; break;

        case 5: data.soilTemp = -999; break;

        case 6: data.EC = -999; break;

      }

      // Для отладки можно вывести ошибку (раскомментируйте при необходимости)

      // Serial.print("Modbus err at "); Serial.print(params[i].addr, HEX);

      // Serial.print(": "); Serial.println(result, HEX);

    }

    delay(50); // пауза между запросами (как в test_only_data.ino)

  }



  // Устанавливаем флаг валидности Modbus-данных

  data.modbusValid = anySuccess;

}


// ========== ЛОГГИРОВАНИЕ ==========
void logToSD() {
  if (!initSD()) {
    Serial.println("SD error, cannot log");
    return;
  }
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (!dataFile) {
    Serial.println("Cannot open file");
    return;
  }

  String line = getTimeString();
  line += "," + String(data.temp1) + "," + String(data.hum1);
  line += "," + String(data.temp2) + "," + String(data.hum2);
  line += "," + String(data.pressureMPa);
  line += "," + String(data.flowRate) + "," + String(data.totalVolume);
  for (int i = 0; i < NUM_CAP_SENSORS; i++) {
    line += "," + String(data.soilMoisture[i]);
  }
  if (data.modbusValid) {
    line += "," + String(data.N) + "," + String(data.P) + "," + String(data.K);
    line += "," + String(data.pH) + "," + String(data.soilMoistureModbus);
    line += "," + String(data.soilTemp) + "," + String(data.EC);
  } else {
    line += ",-999,-999,-999,-999,-999,-999,-999";
  }

  dataFile.println(line);
  dataFile.close();
  Serial.print("DATA:");
  Serial.println(line);
  Serial.println("Data logged to SD");
}

// ========== LOOP (КОНЕЧНЫЙ АВТОМАТ) ==========
void loop() {
  unsigned long now = millis();

  // 1. Управление светом (не зависит от состояния полива)
  handleLighting();

  // 2. Конечный автомат полива
  switch (state) {
    case IDLE:
      if (now - lastMeasureTime >= MEASURE_INTERVAL * 1000UL) {
        Serial.println("State: MEASURE");
        powerSensorsOn();
        readAllSensors();
        lastMeasureTime = now;
        if (getAverageMoisture() < LOW_MOISTURE_THRESHOLD) {
          state = WATERING;
          wateringStartTime = now;
          lastWateringCheck = now;
          digitalWrite(relayPumpPin, HIGH);
          enableFlowInterrupt();
          Serial.println("Starting watering");
        } else {
          logToSD();
          powerSensorsOff();
          state = IDLE;
        }
      }
      break;

    case MEASURE:
      // Не используется, оставлено для совместимости
      state = IDLE;
      break;

    case WATERING:
      if (now - lastWateringCheck >= WATERING_CHECK_INTERVAL * 1000UL) {
        lastWateringCheck = now;
        readSoilCapacitive();
        readModbus();

        Serial.print("Moisture: ");
        for (int i = 0; i < NUM_CAP_SENSORS; i++) Serial.print(data.soilMoisture[i]); Serial.print(" ");
        if (data.modbusValid) Serial.println(data.soilMoistureModbus); else Serial.println("Modbus invalid");

        if (getAverageMoisture() >= HIGH_MOISTURE_THRESHOLD) {
          Serial.println("Moisture target reached, stopping pump");
          digitalWrite(relayPumpPin, LOW);
          disableFlowInterrupt();
          readAllSensors();
          logToSD();
          powerSensorsOff();
          state = IDLE;
          break;
        }

        if (now - wateringStartTime >= MAX_WATERING_TIME * 1000UL) {
          Serial.println("Max watering time reached, stopping pump");
          digitalWrite(relayPumpPin, LOW);
          disableFlowInterrupt();
          readAllSensors();
          logToSD();
          powerSensorsOff();
          state = IDLE;
          break;
        }
      }

      if (flowInterruptEnabled) {
        readFlow();
      }
      break;
  }

  delay(10); // небольшая задержка для стабильности
}