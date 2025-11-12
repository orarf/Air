#include <Arduino.h>
#include <Preferences.h>
#include "TBmanager.h"
#include "commandline.h"
#include "ModbusRTUMaster.h"

// ---------------- Modbus ----------------
#define MAX485_DE 4
#define MAX485_RE 4
#define LED1 23 // กำหนดขาที่จะเทส
#define LED2 19
#define LED3 18
#define LED4 5
#define SOIL_PIN 34


// Preferences prefs;
SemaphoreHandle_t xMutex;    // สร้าง Mutex
bool ledShouldBeOn1 = false; // ตัวแปรที่ใช้ร่วมกัน
bool ledShouldBeOn2 = false;
bool ledShouldBeOn3 = false;
bool ledShouldBeOn4 = false;
HardwareSerial RS485(2); // Serial2 (RX=16, TX=17)
// ModbusRTUMaster modbus(RS485, MAX485_DE, MAX485_RE);
ModbusRTUMaster modbus(RS485);
uint8_t slaveId = 10;
uint8_t slaveId2 = 5;
uint8_t slaveId3 = 1;
uint8_t slaveId4 = 15;
uint32_t baud = 4800;

void preTransmission()
{
  // digitalWrite(MAX485_DE, HIGH);
  // digitalWrite(MAX485_RE, HIGH);
  delay(30);
  // delayMicroseconds(100);
  
  
}

void postTransmission()
{
  // digitalWrite(MAX485_DE, LOW);
  // digitalWrite(MAX485_RE, LOW);
  delay(30);
  // delayMicroseconds(100);
}

// ---------------- RPC Example ----------------
TBmanager *tbManager;
CommandLineManager *cliManager;
Preferences preferences;

void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response)
{
  if (xSemaphoreTake(xMutex, portMAX_DELAY))
  {
    if (data == "on1")
    {
      ledShouldBeOn1 = true; // เปิดไฟ
    }
    else if (data == "off1")
    {
      ledShouldBeOn1 = false; // ดับไฟ
    }
    else if (data == "on2")
    {
      ledShouldBeOn2 = true; // ดับไฟ
    }
    else if (data == "off2")
    {
      ledShouldBeOn2 = false; // ดับไฟ
    }
    else if (data == "on3")
    {
      ledShouldBeOn3 = true; // ดับไฟ
    }
    else if (data == "off3")
    {
      ledShouldBeOn3 = false; // ดับไฟ
    }
    else if (data == "on4")
    {
      ledShouldBeOn4 = true; // ดับไฟ
    }
    else if (data == "off4")
    {
      ledShouldBeOn4 = false; // ดับไฟ
    }
    xSemaphoreGive(xMutex);
  }
  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process2(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received RPC_TEST_process2");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> inner;
  inner["string"] = "exampleResponseString RPC_TEST_process2";
  inner["int"] = 5;
  inner["float"] = 5.0f;
  inner["bool"] = true;
  response["json_data"] = inner;
}

void RPC_TEST_process3(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received RPC_TEST_process3");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> inner;
  inner["string"] = "exampleResponseString";
  inner["int"] = 5;
  inner["float"] = 5.0f;
  inner["bool"] = true;
  response["json_data"] = inner;
}

const RPC_Callback xcallbacks[3] = {
    {"USER_CONTROL_01", RPC_TEST_process1},
    {"USER_CONTROL_02", RPC_TEST_process2},
    {"USER_CONTROL_03", RPC_TEST_process3}};

const char *getDirectionName(uint16_t degree)
    {
        degree %= 360; // ทำให้อยู่ในช่วง 0-359

        if (degree >= 337.5 || degree < 22.5)
            return "เหนือ";
        else if (degree >= 22.5 && degree < 67.5)
            return "ตะวันออกเฉียงเหนือ";
        else if (degree >= 67.5 && degree < 112.5)
            return "ตะวันออก";
        else if (degree >= 112.5 && degree < 157.5)
            return "ตะวันออกเฉียงใต้";
        else if (degree >= 157.5 && degree < 202.5)
            return "ใต้";
        else if (degree >= 202.5 && degree < 247.5)
            return "ตะวันตกเฉียงใต้";
        else if (degree >= 247.5 && degree < 292.5)
            return "ตะวันตก";
        else if (degree >= 292.5 && degree < 337.5)
            return "ตะวันตกเฉียงเหนือ";
        return "ไม่ทราบทิศ";
  }
  const char* globalDirName1 = nullptr;
  uint16_t Air1 = 0, Air2 = 0;
  float temp = 0.0;
  float hum = 0.0;
  uint8_t isLight = 0;
  uint16_t previousCount = 0;
  float totalRainfall = 0.0;
  const float resolution = 0.2;
  uint16_t currentCount1 = 0;
  float wind_mps = 0.0;
  uint32_t lux_value = 0;
  void sentder(TimerHandle_t xTimer)
  {
   
        uint16_t increment;
        if (currentCount1 < previousCount) {
            // กรณี overflow
            increment = (65535 - previousCount) + currentCount1;
        } else {
            increment = currentCount1 - previousCount;
        }

        float rainfall = increment * resolution;
        totalRainfall = rainfall;  

        // Serial.printf("ปริมาณน้ำฝนในช่วงเวลานี้: %.2f mm\n", totalRainfall);

        previousCount = currentCount1;  // เก็บค่าไว้ใช้รอบหน้าคำนวณต่อ
    
    

    tbManager->sendTelemetryData("Temperature", temp);
    tbManager->sendTelemetryData("Humidity", hum);
    tbManager->sendTelemetryData("Light", isLight);
    tbManager->sendTelemetryData("Airname", Air1);
    tbManager->sendTelemetryData("Airname", globalDirName1);
    tbManager->sendTelemetryData("Railfall", totalRainfall);
    tbManager->sendTelemetryData("WindSpeed", wind_mps);
    tbManager->sendTelemetryData("lux_value", lux_value);
    tbManager->sendAttributeData("Temperature", temp);
    tbManager->sendAttributeData("Humidity", hum);
    tbManager->sendAttributeData("Light", isLight);
    tbManager->sendAttributeData("Airname", Air1);
    tbManager->sendAttributeData("Airname", globalDirName1);
    tbManager->sendAttributeData("Railfall", totalRainfall);
    tbManager->sendAttributeData("WindSpeed", wind_mps);
    tbManager->sendAttributeData("lux_value", lux_value);
   
}

void reader(void *pvParameters)
{
  RS485.begin(baud, SERIAL_8N1, 16, 17);
  modbus.begin(baud, SERIAL_8N1);
  while (1)
  {
  uint16_t _Air1 = 0, _Air2 = 0;
  uint16_t _rawTemp = 0;
  uint16_t _rawHum = 0;
  uint16_t _buf[2];
  uint16_t currentCount = 0;
  uint16_t _WindRaw = 0;
  preTransmission();
  ModbusRTUMasterError errWind = modbus.readHoldingRegisters(slaveId4, 0, &_WindRaw, 1);
  ModbusRTUMasterError errAir1 = modbus.readHoldingRegisters(slaveId, 1, &_Air1, 1);
  ModbusRTUMasterError errTemp = modbus.readHoldingRegisters(slaveId2, 501, &_rawTemp, 1);   // register 1 = temp
  ModbusRTUMasterError errLight = modbus.readHoldingRegisters(slaveId2, 506,_buf, 2); // register 2 = light
  ModbusRTUMasterError errHum = modbus.readHoldingRegisters(slaveId2, 500, &_rawHum, 1);     // register 3 = humidity
  ModbusRTUMasterError err = modbus.readHoldingRegisters(slaveId3, 1, &currentCount, 1);
  postTransmission();

   if (errAir1 == MODBUS_RTU_MASTER_SUCCESS)
        {
            Air1 = _Air1;
            globalDirName1 = getDirectionName(_Air1);

            // Serial.printf("S: %d° (%s)\n", _Air1, dirName1);
            // Serial.println("Press q to main menu\n");
        }
        else
        {
            // Serial.printf("Modbus Error S:%d\n", errAir1);
            // Serial.println("Press q to main menu\n");
        }

   if (errTemp == MODBUS_RTU_MASTER_SUCCESS &&
            errLight == MODBUS_RTU_MASTER_SUCCESS &&
            errHum == MODBUS_RTU_MASTER_SUCCESS)
        {
             temp = _rawTemp / 10.0;
             hum = _rawHum / 10.0;
            uint16_t lux_high = _buf[0];
            uint16_t lux_low = _buf[1];
            lux_value = ((uint32_t)lux_high << 16) | lux_low; // รวมเป็น 32-bit
            uint8_t _isLight = (lux_value > 50) ? 1 : 0;
            isLight = _isLight;

            // Serial.printf("Temp: %.1f C, Humidity: %.1f %%, Light: %u\n", temp_c, hum_percent, _isLight);
            // Serial.println("Press q to main menu\n");
        }
        else
        {
            // Serial.printf("Modbus Error Temp:%d Light:%d Humidity:%d\n", errTemp, errLight, errHum);
            // Serial.println("Press q to main menu\n");
        }
        
        if (err == MODBUS_RTU_MASTER_SUCCESS) {
          currentCount1 = currentCount;
         }
    else {
        // Serial.printf("Modbus reading error: %d\n", err);
        // ในกรณี error ค่าจะคงที่ ไม่เพิ่ม พร้อมแสดงค่าก่อนหน้า
    }

    if (errWind == MODBUS_RTU_MASTER_SUCCESS) {
       wind_mps = _WindRaw / 10.0; // จะได้ m/s จริง
      // Serial.printf("Wind Speed: %.1f m/s\n", wind_mps);
    } else {
      // Serial.printf("Modbus Error Wind:%d\n", errWind);
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// ---------------- Setup ----------------
void setup()
{
  Serial.begin(9600);
  delay(1000);
  xMutex = xSemaphoreCreateMutex();

  // pinMode(MAX485_DE, OUTPUT);
  // pinMode(MAX485_RE, OUTPUT);
  // postTransmission();

  

  Serial.println("\n===== System Booting Up =====");

  // โหลดค่าเก่าก่อน
  cliManager = new CommandLineManager(nullptr, &preferences);
  cliManager->loadSettings();
  Serial.println("--- Loaded Stored Settings ---");
  Serial.print("  SSID: ");
  Serial.println(cliManager->getSsid());
  Serial.print("  Server: ");
  Serial.println(cliManager->getServer());
  Serial.print("  Token: ");
  Serial.println(cliManager->getToken());
  Serial.println("------------------------------");

  // สร้าง TBmanager
  tbManager = new TBmanager(
      cliManager->getSsid(),
      cliManager->getPassword(),
      cliManager->getServer(),
      cliManager->getToken());

  // เชื่อม CLI กับ TBmanager
  cliManager = new CommandLineManager(tbManager, &preferences);
  cliManager->loadSettings();

  // Register RPC callbacks
  tbManager->RPCRoute(xcallbacks);

  // ตรวจสอบว่ากดเข้าเมนูตั้งค่าไหม
  cliManager->begin();
  tbManager->begin();
  TimerHandle_t sentDer = xTimerCreate("sensorTimer", pdMS_TO_TICKS(60000), pdTRUE, (void *)0, sentder);
  // TimerHandle_t reaDer = xTimerCreate("readTimer", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, reader);
  // // เริ่ม Timer
  if (sentDer != NULL)
    xTimerStart(sentDer, 0);
  // if (reaDer != NULL)
  //   xTimerStart(reaDer, 0);
  xTaskCreate(reader, "Read Sensor Task", 4096, NULL, 1, NULL);
}

// ---------------- Loop ----------------
void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
  // ทำงานผ่าน FreeRTOS task
}