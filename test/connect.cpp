#include <WiFi.h>
#include <Preferences.h>

Preferences prefs;
String ssid;
String password;

// ฟังก์ชันเชื่อมต่อ Wi-Fi
void connectWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to %s", ssid);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Connection failed!");
  }
}

// ฟังก์ชันล้างค่า Wi-Fi ใน Preferences
void clearWiFiPrefs() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  Serial.println("🗑️ Cleared Wi-Fi credentials in Preferences");
}

// ฟังก์ชันอ่านค่า Wi-Fi จาก Serial และบันทึก
void inputWiFiFromSerial() {
  Serial.println("\nEnter new SSID: ");
  while (Serial.available() == 0);
  ssid = Serial.readStringUntil('\n');
  ssid.trim();

  Serial.println("Enter new Password: ");
  while (Serial.available() == 0);
  password = Serial.readStringUntil('\n');
  password.trim();

  // บันทึกลง Preferences
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();

  Serial.println("💾 Wi-Fi credentials saved!");
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  // ล้างค่าเก่าเลย
  clearWiFiPrefs();

  // ป้อนค่าใหม่ผ่าน Serial
  inputWiFiFromSerial();

  // เชื่อมต่อ Wi-Fi
  connectWiFi(ssid.c_str(), password.c_str());
}

void loop() {
  // ถ้าหลุดจาก Wi-Fi ให้ลองเชื่อมต่อใหม่อัตโนมัติ
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔄 Reconnecting...");
    connectWiFi(ssid.c_str(), password.c_str());
  }
  delay(10000);
}
