#include <Arduino.h>
#include "ModbusRTUMaster.h"

#define MAX485_DE 4
#define MAX485_RE 4
HardwareSerial RS485(2); // Serial2 (RX=16, TX=17)
// ModbusRTUMaster modbus(RS485, MAX485_DE, MAX485_RE);
ModbusRTUMaster modbus(RS485);
uint8_t slaveId = 1;
uint32_t baud = 4800;
void preTransmission()
{
  delay(30);
}

void postTransmission()
{
  delay(30);
}
void TaskModbus(void *pvParameters) {
   RS485.begin(baud, SERIAL_8N1, 16, 17);
    modbus.begin(baud, SERIAL_8N1);

  for (;;) {
    uint16_t buf[10];

    ModbusRTUMasterError result = modbus.readHoldingRegisters(slaveId, 0x0000, buf, 10);
    

    if (result == MODBUS_RTU_MASTER_SUCCESS) {
      Serial.println("Read Holding Registers success:");
      for (int i = 0; i < 10; i++) {
        Serial.print("Register ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(buf[i]);
      } 
    } else {
      Serial.print("Read fail. Error code: ");
      Serial.println(result);
      }
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
void setup() {
  Serial.begin(9600);


  xTaskCreate(
    TaskModbus,
    "ModbusTask",
    4096,
    NULL,
    1,
    NULL
  );
}

void loop() {
  // ว่างไว้ เพราะเราจะทำงานผ่าน FreeRTOS Task
}