#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "TBmanager.h"
#include "ModbusRTUMaster.h"

// --- ค่าเริ่มต้น ---
#define DEFAULT_WIFI_SSID "gateway4"
#define DEFAULT_WIFI_PASSWORD "Trinergy@2023"
#define DEFAULT_TB_SERVER "thingsboard.tricommtha.com"
#define DEFAULT_TB_TOKEN "jo3X29an34Esr4cSGie6"

// สำหรับ Modbus
extern ModbusRTUMaster modbus; // ต้องประกาศ global ใน main
extern uint8_t slaveId;
extern uint8_t slaveId2;
extern uint8_t slaveId3;
extern uint8_t slaveId4;
extern void preTransmission();
extern void postTransmission();

class CommandLineManager {
private:
    TBmanager* tbManager;
    Preferences* prefs;

    // State machine
    enum MenuState {
        MAIN_MENU,
        SHOW_STATUS,
        SCAN_WIFI,
        CONNECTION_SETTINGS_MENU,
        SET_SSID,
        SET_PASSWORD,
        SET_SERVER,
        SET_TOKEN,
        SAVE_AND_REBOOT,
        SHOW_NPK,
        SHOW_SENSOR,
        SHOW_RAINFALL,
        SHOW_WIND
        
    };

    MenuState currentState = MAIN_MENU;
    bool menuNeedsDisplay = true;

    // Temp settings
    String tempSsid;
    String tempPass;
    String tempServer;
    String tempToken;

    void displayMainMenu() {
        Serial.println("\n===== Main Menu =====");
        Serial.println("1: Check System Status");
        Serial.println("2: Connection Settings");
        Serial.println("3: Scan for WiFi");
        Serial.println("4: Show Air values"); // ตัวเลือกใหม่
        Serial.println("5: Show Sensor values"); // ตัวเลือกใหม่
        Serial.println("6: Show Rain Fall"); 
        Serial.println("7: Show Wind Speed");
        Serial.print("Please enter your choice and press Enter: ");
        menuNeedsDisplay = false;
    }

    void displayConnectionSettingsMenu() {
        Serial.println("\n--- Connection Settings ---");
        Serial.print("  1: Set SSID     (current: '"); Serial.print(tempSsid); Serial.println("')");
        Serial.print("  2: Set Password   (current: '"); Serial.print(tempPass); Serial.println("')");
        Serial.print("  3: Set Server     (current: '"); Serial.print(tempServer); Serial.println("')");
        Serial.print("  4: Set Token      (current: '"); Serial.print(tempToken); Serial.println("')");
        Serial.println("---------------------------");
        Serial.println("  5: Save and Reboot");
        Serial.println("  0: Back to Main Menu");
        Serial.print("Please enter your choice: ");
        menuNeedsDisplay = false;
    }

    String readSerialInput() {
        while (Serial.available() > 0) Serial.read();
        String input = "";
        while (true) {
            if (Serial.available()) {
                char c = Serial.read();
                if (c == '\r' || c == '\n') {
                    Serial.println();
                    break;
                } else if (c == 127 || c == 8) {
                    if (input.length() > 0) {
                        input.remove(input.length() - 1);
                        Serial.print("\b \b");
                    }
                } else {
                    input += c;
                    Serial.print(c);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        input.trim();
        return input;
    }

    void showStatus() {
        Serial.println("\n--- System Status ---");
        if (tbManager && tbManager->get_wifiStatus()) {
            Serial.print("WiFi Status: Connected to '"); Serial.print(tbManager->get_SSID()); Serial.println("'");
            Serial.print("IP Address: "); Serial.println(tbManager->get_ipAddr());
            if (tbManager->get_thingsboardStatus()) {
                Serial.println("ThingsBoard: Connected");
                if (tbManager->get_rpcStatus()) Serial.println("RPC Service: Active");
                else Serial.println("RPC Service: Not Subscribed");
            } else Serial.println("ThingsBoard: Disconnected");
        } else Serial.println("WiFi Status: Disconnected");
        Serial.println("-----------------------");
        Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");
    }

    void scanForWiFi() {
        Serial.println("\nStarting WiFi Scan...");
        int n = WiFi.scanNetworks();
        Serial.println("Scan done.");
        if (n == 0) Serial.println("No networks found.");
        else {
            Serial.print(n); Serial.println(" networks found:");
            Serial.println("----------------------------------------------------");
            for (int i = 0; i < n; ++i) {
                Serial.print(i+1); Serial.print(": "); Serial.print(WiFi.SSID(i));
                Serial.print(" ("); Serial.print(WiFi.RSSI(i)); Serial.print(")");
                Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : " *");
            }
            Serial.println("----------------------------------------------------");
        }
        Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");
    }

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

    void showNPKLoop()
    {
        uint16_t _Air1 = 0, _Air2 = 0;

        preTransmission();
        ModbusRTUMasterError errAir1 = modbus.readHoldingRegisters(slaveId, 1, &_Air1, 1);
        postTransmission();
        
        if (errAir1 == MODBUS_RTU_MASTER_SUCCESS)
        {

            const char *dirName1 = getDirectionName(_Air1);

            Serial.printf("S: %d° (%s)\n", _Air1, dirName1);
            Serial.println("Press q to main menu\n");
        }
        else
        {
            Serial.printf("Modbus Error S:%d\n", errAir1);
            Serial.println("Press q to main menu\n");
        }
    }
        uint16_t _rawTemp = 0;
        uint16_t _rawHum = 0;
        uint16_t _buf[2];
    void showSensorLoop()
    {
        uint16_t _rawTemp = 0;
        uint16_t _rawHum = 0;
        uint16_t _buf[2];
        preTransmission();
        ModbusRTUMasterError errTemp = modbus.readHoldingRegisters(slaveId2, 501, &_rawTemp, 1); // register 1 = temp
        ModbusRTUMasterError errLight = modbus.readHoldingRegisters(slaveId2, 506, _buf, 2);      // register 2 = light
        ModbusRTUMasterError errHum = modbus.readHoldingRegisters(slaveId2, 500, &_rawHum, 1);   // register 3 = humidity
        postTransmission();

        if (errTemp == MODBUS_RTU_MASTER_SUCCESS &&
            errLight == MODBUS_RTU_MASTER_SUCCESS &&
            errHum == MODBUS_RTU_MASTER_SUCCESS)
        {
            float temp_c = _rawTemp / 10.0;
            float hum_percent = _rawHum / 10.0;
            uint16_t lux_high = _buf[0];
            uint16_t lux_low = _buf[1];
            uint32_t lux_value = ((uint32_t)lux_high << 16) | lux_low; // รวมเป็น 32-bit
            uint8_t _isLight = (lux_value > 50) ? 1 : 0;

            Serial.printf("Temp: %.1f C, Humidity: %.1f %%, Light: %u\n", temp_c, hum_percent, _isLight);
            Serial.println("Press q to main menu\n");
        }
        else
        {
            Serial.printf("Modbus Error Temp:%d Light:%d Humidity:%d\n", errTemp, errLight, errHum);
            Serial.println("Press q to main menu\n");
        }
    }

   // ตัวแปร global เก็บค่า previous pulse count
uint16_t previousCount = 0;
// ตัวแปร global เก็บค่า rainfall สะสมโดยรวม
float totalRainfall = 0.0;
// ค่าความละเอียดน้ำฝน (mm/pulse)
const float resolution = 0.2;

void showRainfall() {
    uint16_t currentCount = 0;
    preTransmission();
    ModbusRTUMasterError err = modbus.readHoldingRegisters(slaveId3, 1, &currentCount, 1);
    postTransmission();

    if (err == MODBUS_RTU_MASTER_SUCCESS) {
        uint16_t increment;
        if (currentCount < previousCount) {
            // กรณี overflow
            increment = (65535 - previousCount) + currentCount;
        } else {
            increment = currentCount - previousCount;
        }

        float rainfall = increment * resolution;
        totalRainfall = rainfall;  // บวกเพิ่มปริมาณฝนสะสม

        Serial.printf("ปริมาณน้ำฝนในช่วงเวลานี้: %.2f mm\n", totalRainfall);
        Serial.println("Press q to main menu\n");

        previousCount = currentCount;  // เก็บค่าไว้ใช้รอบหน้าคำนวณต่อ
    }
    else {
        Serial.printf("Modbus reading error: %d\n", err);
        Serial.println("Press q to main menu\n");
        // ในกรณี error ค่าจะคงที่ ไม่เพิ่ม พร้อมแสดงค่าก่อนหน้า
    }
}

void showWind() {
    uint16_t _WindRaw = 0; 
    preTransmission();
    ModbusRTUMasterError errWind = modbus.readHoldingRegisters(slaveId4, 0, &_WindRaw, 1);
    postTransmission();
    if (errWind == MODBUS_RTU_MASTER_SUCCESS) {
       float wind_mps = _WindRaw / 10.0; // จะได้ m/s จริง
       Serial.printf("Wind Speed: %.1f m/s\n", wind_mps);
       Serial.println("Press q to main menu\n");
    } else {
      Serial.printf("Modbus Error Wind:%d\n", errWind);
      Serial.println("Press q to main menu\n");
    }
}

    
    void cliTask() {
        while(true){
            switch(currentState){
                case MAIN_MENU:
                    if(menuNeedsDisplay) displayMainMenu();
                    if(Serial.available()>0){
                        String choice = readSerialInput();
                        if(choice=="1") currentState = SHOW_STATUS;
                        else if(choice=="2") currentState = CONNECTION_SETTINGS_MENU;
                        else if(choice=="3") currentState = SCAN_WIFI;
                        else if(choice=="4") currentState = SHOW_NPK;
                        else if(choice=="5") currentState = SHOW_SENSOR;
                        else if(choice=="6") currentState = SHOW_RAINFALL;
                        else if(choice=="7") currentState = SHOW_WIND;
                        else if(choice.length()>0) Serial.println("Invalid choice");
                        menuNeedsDisplay = true;
                    }
                    break;

                case SHOW_STATUS:
                    showStatus();
                    while(currentState==SHOW_STATUS){
                        String returnChoice = readSerialInput();
                        if(returnChoice=="0"){ currentState=MAIN_MENU; menuNeedsDisplay=true; }
                        else if(returnChoice.length()>0) Serial.print("Press 0 to return: ");
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;

                case SCAN_WIFI:
                    scanForWiFi();
                    while(currentState==SCAN_WIFI){
                        String returnChoice = readSerialInput();
                        if(returnChoice=="0"){ currentState=MAIN_MENU; menuNeedsDisplay=true; }
                        else if(returnChoice.length()>0) Serial.print("Press 0 to return: ");
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;

                case CONNECTION_SETTINGS_MENU:
                    if(menuNeedsDisplay) displayConnectionSettingsMenu();
                    if(Serial.available()>0){
                        String choice = readSerialInput();
                        if(choice=="1") currentState=SET_SSID;
                        else if(choice=="2") currentState=SET_PASSWORD;
                        else if(choice=="3") currentState=SET_SERVER;
                        else if(choice=="4") currentState=SET_TOKEN;
                        else if(choice=="5") currentState=SAVE_AND_REBOOT;
                        else if(choice=="0") currentState=MAIN_MENU;
                        else if(choice.length()>0) Serial.println("Invalid choice");
                        menuNeedsDisplay=true;
                    }
                    break;

                case SET_SSID:
                    Serial.print("Enter new WiFi SSID: ");
                    tempSsid = readSerialInput();
                    Serial.println("SSID updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SET_PASSWORD:
                    Serial.print("Enter new WiFi Password: ");
                    tempPass = readSerialInput();
                    Serial.println("Password updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SET_SERVER:
                    Serial.print("Enter new ThingsBoard Server: ");
                    tempServer = readSerialInput();
                    Serial.println("Server updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SET_TOKEN:
                    Serial.print("Enter new Device Token: ");
                    tempToken = readSerialInput();
                    Serial.println("Token updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SAVE_AND_REBOOT:
                    Serial.println("\nSaving settings...");
                    prefs->begin("conn-info", false);
                    prefs->putString("ssid", tempSsid);
                    prefs->putString("password", tempPass);
                    prefs->putString("server", tempServer);
                    prefs->putString("token", tempToken);
                    prefs->end();
                    Serial.println("Saved. Rebooting in 3s...");
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    // ESP.restart();
                    break;

                case SHOW_NPK:
                    showNPKLoop(); // แสดงค่าตามปกติ
                    // เช็กเฉพาะว่ามี input ใหม่ตอนอยู่ใน state นี้
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        { // กด q เพื่อออก
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }

                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ ก่อนอ่านค่าใหม่
                    break;

                case SHOW_SENSOR:
                    showSensorLoop(); // แสดงค่าตามปกติ
                    // เช็กเฉพาะว่ามี input ใหม่ตอนอยู่ใน state นี้
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        { // กด q เพื่อออก
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }

                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ ก่อนอ่านค่าใหม่
                    break;

                case SHOW_RAINFALL:
                    showRainfall();
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        {
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ
                    break;

                case SHOW_WIND:
                    showWind();
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        {
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ
                    break;
                }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    static void taskWrapper(void* pvParameters){
        static_cast<CommandLineManager*>(pvParameters)->cliTask();
    }

public:
    CommandLineManager(TBmanager* tb, Preferences* p):tbManager(tb), prefs(p){}
    

    void loadSettings() {
    prefs->begin("conn-info", false); // เปิดแบบเขียนได้

    bool updated = false; // ใช้ตรวจว่ามีการอัปเดตค่าใหม่หรือไม่

    // === SSID ===
    if (prefs->isKey("ssid")) {
        tempSsid = prefs->getString("ssid");
    } else {
        tempSsid = DEFAULT_WIFI_SSID;
        prefs->putString("ssid", DEFAULT_WIFI_SSID);
        updated = true;
    }

    // === PASSWORD ===
    if (prefs->isKey("password")) {
        tempPass = DEFAULT_WIFI_PASSWORD;
    } else {
        tempPass = DEFAULT_WIFI_PASSWORD;
        prefs->putString("password", DEFAULT_WIFI_PASSWORD);
        updated = true;
    }

    // === SERVER ===
    if (prefs->isKey("server")) {
        tempServer = prefs->getString("server");
    } else {
        tempServer = DEFAULT_TB_SERVER;
        prefs->putString("server", DEFAULT_TB_SERVER);
        updated = true;
    }

    // === TOKEN ===
    if (prefs->isKey("token")) {
        tempToken = prefs->getString("token");
    } else {
        tempToken = DEFAULT_TB_TOKEN;
        prefs->putString("token", DEFAULT_TB_TOKEN);
        updated = true;
    }

    prefs->end();

    if (updated) {
        Serial.println("[INFO] Default settings were missing. Saved defaults to Preferences.");
    }
}
    void begin(){
        xTaskCreate(taskWrapper,"CLI_Task", 1024*4,this,1,NULL);
    }

    bool shouldEnterMenuOnBoot(unsigned long waitTime){
        Serial.println("\n[ACTION] Press any key within 10 seconds to enter setup menu.");
        unsigned long startTime=millis();
        while(millis()-startTime<waitTime){
            if(Serial.available()>0){
                while(Serial.available()>0) Serial.read();
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        return false;
    }

    const char* getSsid() { return tempSsid.c_str(); }
    const char* getPassword() { return tempPass.c_str(); }
    const char* getServer() { return tempServer.c_str(); }
    const char* getToken() { return tempToken.c_str(); }
};

#endif // COMMANDLINE_H