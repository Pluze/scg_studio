#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocentralAds1292r.h"

#define WIFI_TIMEOUT 1000 * 1
#define MYSSID "Xiaomi_FF8E"
#define MYPSWD "NOMOREMDPI"
/* vars */
// WiFi network name and password:
char networkName[50];
char networkPswd[200];
// IP address to send UDP data to:
// either use the ip address of the server or
// a network broadcast address
const int udpPort = 3333;
// Are we currently connected?
boolean connected = false;
// The udp library class
WiFiUDP udp;
IPAddress myip;
IPAddress broadcast;
/*
current state

0 for first time power on

1 for sensors been init

2 for waiting for instruction/idle

3 for sending signals

4 for known error state
*/
int sys_state = 0;
// Sensors pin settings
const int pin_INT1 = 04;   // IMU data ready
const int pin_SS1 = SS;    // IMU SS/CSB SS=IO5/29
//const int pin_DRDYB = 02;  // ECG data ready
//const int pin_SS2 = 17;    // ECG SS/CSB
const int ADS1292_DRDY_PIN = 16;
const int ADS1292_CS_PIN = 17;
const int ADS1292_START_PIN = 15;
const int ADS1292_PWDN_PIN = 2;

ads1292r ADS1292R;

// Data buffer and reading state
float buffer[10];
bool buffer_success1 = 0;
bool buffer_success2 = 0;
String strbuffer;
// Tmp var for IMU
int16_t temp;
int16_t tmp1[6] = { 0, 0, 0, 0, 0, 0 };
int16_t tmp2[6] = { 0, 0, 0, 0, 0, 0 };
int16_t tmp3[6] = { 0, 0, 0, 0, 0, 0 };
byte ix1 = 0;
byte ix2 = 0;
// Tmp var for ECG
int32_t ecgVal;
int32_t ecgTmp1 = 0;
int32_t ecgTmp2 = 0;
int32_t ecgTmp3 = 0;
byte ex1 = 0;
byte ex2 = 0;
byte ex3 = 0;
// DB
int8_t battDB[] = { 10, 11, 13, 15,
                    17, 19, 22, 24, 27, 30, 32, 35, 38, 41,
                    44, 47, 50, 52, 55, 57, 59, 62, 64, 65,
                    67, 69, 70, 72, 74, 75, 76, 78, 79, 80,
                    82, 83, 84, 85, 87, 88, 89, 90, 91, 92,
                    93, 94, 94, 95, 96, 97, 98, 99, 99, 100 };

/* system functions */
void setup() {
  pinMode(pin_INT1, INPUT);
  pinMode(pin_SS1, OUTPUT);
  //pinMode(pin_DRDYB, INPUT);
  //pinMode(pin_SS2, OUTPUT);

  pinMode(ADS1292_DRDY_PIN, INPUT);
  pinMode(ADS1292_CS_PIN, OUTPUT);
  pinMode(ADS1292_START_PIN, OUTPUT);
  pinMode(ADS1292_PWDN_PIN, OUTPUT);

  Serial.begin(115200);
  SerialLog(6, "Serial.begin(115200)");
  setWiFiconfig();
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SerialLog(6, "SPI.begin");
  delay(1000);
  setup_LSM6DSL(pin_SS1);
  SerialLog(6, "setup_LSM6DSL");
  delay(1000);
  //setup_ECG(pin_SS2);
  SPI.setDataMode(SPI_MODE1);
  ADS1292R.ads1292Init(ADS1292_CS_PIN,ADS1292_PWDN_PIN,ADS1292_START_PIN);
  SerialLog(6, "setup_ECG");
  delay(1000);
  SerialLog(6, "Configuring wifi...");
  connectToWiFi(networkName, networkPswd);
  sys_state = 1;
}

void loop() {
  char inChar = 0;
  if (Serial.available() > 0) {

    while (Serial.available() > 1) {
      Serial.read();
    }
    inChar = (char)Serial.read();
    switch (inChar) {
      case 's':
        sys_state = 3;
        SerialLog(5, "start do_sendingsignal");
        break;
      case 'e':
        sys_state = 2;
        SerialLog(5, "stop do_sendingsignal, idle");
        break;
      case 'r':
        SerialLog(5, "manually restart");
        ESP.restart();
        break;
      case 'b':
        do_batteryinfo();
        break;
      case '?':
        do_sysstateinfo();
        break;
      default:
        SerialLog(5, "Unrecognized instructions");
        break;
    }
  }
  switch (sys_state) {
    case 3:
      do_sendingsignal();
      break;
    case 0:
      SerialLog(3, "uninit SYSstate 0, ESP.restart");
      ESP.restart();
      break;
    case 1:
      sys_state = 2;
      break;
    case 2:
      // do nothing but waiting
      sleep(1);
      break;
    case 4:
      SerialLog(3, "unknown SYSstate 4, ESP.restart");
      ESP.restart();
      break;
    default:
      SerialLog(3, "catastrophic SYSstate ?, ESP.restart");
      ESP.restart();
      break;
  }
}

/* state functions */
void do_sendingsignal() {
  

  // getECGdata(pin_SS2);
  // if (!buffer_success2)
  // {
  //   // SerialLog(4,"ecg bug");
  //   return;
  // }
  SPI.setDataMode(SPI_MODE1);
ads1292OutputValues ecgRespirationValues;
SPI.setDataMode(SPI_MODE1);
  boolean ret = ADS1292R.getAds1292EcgAndRespirationSamples(ADS1292_DRDY_PIN,ADS1292_CS_PIN,&ecgRespirationValues);
  if (ret == true)
  {
    SPI.setDataMode(SPI_MODE0);
  getIMUdata(pin_SS1, 1, 0);
  getIMUdata(pin_SS1, 2, 1);
  getIMUdata(pin_SS1, 3, 2);
  getIMUdata(pin_SS1, 4, 3);
  getIMUdata(pin_SS1, 5, 4);
  getIMUdata(pin_SS1, 6, 5);
    buffer[7] = (ecgRespirationValues.sDaqVals[1]) ;  // ignore the lower 8 bits out of 24bits
  }

  // only send data when connected
  if (connected) {
    udp.beginPacket(broadcast, udpPort);
    buffer[0] = micros();
    udp.print(buffer[0]);
    udp.print(',');
    udp.print(buffer[1]);
    udp.print(',');
    udp.print(buffer[2]);
    udp.print(',');
    udp.print(buffer[3]);
    udp.print(',');
    udp.print(buffer[4]);
    udp.print(',');
    udp.print(buffer[5]);
    udp.print(',');
    udp.print(buffer[6]);
    udp.print(',');
    udp.print(buffer[7]);
    udp.print('\n');
    udp.endPacket();
  } else {
    SerialLog(4, "udp bug");
  }
}

void do_batteryinfo() {
  int batt = (analogReadMilliVolts(A5) / 5);
  if (batt < 367) {
    SerialLog(4, "battery lower than 3.67v (10%). Connent to DC charger!");
    return;
  }
  if (batt > 420) {
    SerialLog(4, "battery higher than 4.20v (100%). Are you charging?");
    return;
  }
  strbuffer = "battery: " + String(float(batt) / 100.) + "v, " + String(battDB[batt - 367]) + "%";
  SerialLog(5, strbuffer);
}

void do_sysstateinfo() {
  switch (sys_state) {
    case 0:
      SerialLog(5, "first time power on (0)");
      break;
    case 1:
      SerialLog(5, "sensors been init (1)");
      break;
    case 2:
      SerialLog(5, "waiting for instruction/idle (2)");
      break;
    case 3:
      SerialLog(5, "sending signals (3)");
      break;
    case 4:
      SerialLog(5, "known error state (4)");
      break;
    default:
      SerialLog(5, "unknown error (5)");
      break;
  }
}

/* basic functions */
void getIMUdata(int pin_SS, int bufferInd, int axisInd) {
  int addL = 0x2C;
  int addH = 0x2D;
  switch (axisInd) {
    case 0:  // GX
      addL = 0x22;
      addH = 0x23;
      break;
    case 1:  // GY
      addL = 0x24;
      addH = 0x25;
      break;
    case 2:  // GZ
      addL = 0x26;
      addH = 0x27;
      break;
    case 3:  // LX
      addL = 0x28;
      addH = 0x29;
      break;
    case 4:  // LY
      addL = 0x2A;
      addH = 0x2B;
      break;
    default:  // LZ
      axisInd = 5;
      break;
  }
  getIMUdata_core(pin_SS, bufferInd, addL, addH, axisInd);
}

void getIMUdata_core(int pin_SS, int bufferInd, int addL, int addH, int axisInd) {
  if (1) {
    ix1 = readRegister(pin_SS, addL);
    ix2 = readRegister(pin_SS, addH);
    temp = ix2;
    temp = (temp << 8) | ix1;

    tmp1[axisInd] = tmp2[axisInd];
    tmp2[axisInd] = tmp3[axisInd];
    tmp3[axisInd] = temp;

    if (abs((tmp1[axisInd] - tmp2[axisInd]) + (tmp3[axisInd] - tmp2[axisInd])) > 450) {
      buffer_success1 = 0;
      SerialLog(4, "imu glitch");
      return;
    }
    buffer[bufferInd] = tmp2[axisInd];
    buffer_success1 = 1;
  } else {
    buffer_success1 = 0;
    SerialLog(4, "imu 0");
  }
}

void getECGdata(int pin_SS) {
  if (1) {
    ex1 = readRegister(pin_SS, 0x37);
    ex2 = readRegister(pin_SS, 0x38);
    ex3 = readRegister(pin_SS, 0x39);
    ecgVal = ex1;
    ecgVal = (ecgVal << 8) | ex2;
    ecgVal = (ecgVal << 8) | ex3;

    ecgTmp1 = ecgTmp2;
    ecgTmp2 = ecgTmp3;
    ecgTmp3 = ecgVal;

    if (abs((ecgTmp1 - ecgTmp2) + (ecgTmp3 - ecgTmp2)) > 29000) {
      buffer_success2 = 0;
      SerialLog(4, "ecg glitch");
      return;
    }
    buffer[1] = ecgTmp2;
    buffer_success2 = 1;
  } else {
    buffer_success2 = 0;
    SerialLog(4, "ecg 0");
  }
}

void setup_LSM6DSL(int pin_SS) {
  writeRegister(pin_SS, 0x12, 0b00000101);
  delay(1000);
  writeRegister(pin_SS, 0x10, 0b10100001);
  writeRegister(pin_SS, 0x17, 0b10100000);
  writeRegister(pin_SS, 0x11, 0b10100000);
  writeRegister(pin_SS, 0x13, 0b00000010);
  writeRegister(pin_SS, 0x15, 0b00000010);
  writeRegister(pin_SS, 0x0D, 0b00000001);
}

void setup_ECG(int pin_SS) {
  delay(1000);
  writeRegister(pin_SS, 0x00, 0x00);
  // datasheet ads1293
  // Follow the next steps to configure the device for this example, starting from default registers values.
  // 1. Set address 0x01 = 0x11: Connect channel 1’s INP to IN2 and INN to IN1.
  writeRegister(pin_SS, 0x01, 0x11);
  // 2. Set address 0x02 = 0x19: Connect channel 2’s INP to IN3 and INN to IN1.
  writeRegister(pin_SS, 0x02, 0x19);
  // 3. Set address 0x0A = 0x07: Enable the common-mode detector on input pins IN1, IN2 and IN3.
  writeRegister(pin_SS, 0x0A, 0x07);
  // 4. Set address 0x0C = 0x04: Connect the output of the RLD amplifier internally to pin IN4.
  writeRegister(pin_SS, 0x0C, 0x04);
  // 5. Set address 0x12 = 0x04: Use external crystal and feed the internal oscillator's output to the digital.
  writeRegister(pin_SS, 0x12, 0x04);
  // 6. Set address 0x14 = 0x24: Shuts down unused channel 3’s signal path.
  writeRegister(pin_SS, 0x14, 0x24);
  // 7. Set address 0x21 = 0x02: Configures the R2 decimation rate as 5 for all channels.
  writeRegister(pin_SS, 0x21, 0x02);
  // 8. Set address 0x22 = 0x02: Configures the R3 decimation rate as 6 for channel 1.
  writeRegister(pin_SS, 0x22, 0x02);
  // 9. Set address 0x23 = 0x02: Configures the R3 decimation rate as 6 for channel 2.
  writeRegister(pin_SS, 0x23, 0x02);
  // 10. Set address 0x27 = 0x08: Configures the DRDYB source to channel 1 ECG (or fastest channel).
  writeRegister(pin_SS, 0x27, 0x08);
  // 11. Set address 0x2F = 0x30: Enables channel 1 ECG and channel 2 ECG for loop read-back mode.
  writeRegister(pin_SS, 0x2F, 0x30);
  // 12. Set address 0x00 = 0x01: Starts data conversion.
  writeRegister(pin_SS, 0x00, 0x01);
}

byte readRegister(int pin_SS, byte reg) {
  byte data;
  reg |= 1 << 7;
  digitalWrite(pin_SS, LOW);
  SPI.transfer(reg);
  data = SPI.transfer(0);
  digitalWrite(pin_SS, HIGH);
  return data;
}

void writeRegister(int pin_SS, byte reg, byte data) {
  reg &= ~(1 << 7);
  digitalWrite(pin_SS, LOW);
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(pin_SS, HIGH);
}
// read ssid,password
void setWiFiconfig() {
  SerialLog(6, "configuring WiFi");
  unsigned long startTime = millis();
  while (millis() - startTime < WIFI_TIMEOUT) {
    char tmp = 0;
    if (Serial.available() > 0) {
      // Read WiFi SSID
      SerialLog(6, "strSSID");
      String strSSID;
      while (Serial.available() > 0) {
        tmp = Serial.read();
        if (tmp == ',') {
          break;
        }
        strSSID += (char)tmp;
        SerialLog(6, strSSID);
      }

      // Read WiFi password
      SerialLog(6, "strPWD");
      String strPWD;
      while (Serial.available() > 0) {
        tmp = Serial.read();
        if (tmp == '\n') {
          break;
        }
        strPWD += (char)tmp;
        SerialLog(6, strPWD);
      }

      // Save to EEPROM and break
      strSSID.toCharArray(networkName, 50);
      strPWD.toCharArray(networkPswd, 200);
      SerialLog(6, "configure succeed");
      break;
    }
  }

  // If timeout, use hardcoded WiFi configuration
  if (millis() - startTime >= WIFI_TIMEOUT) {
    strcpy(networkName, MYSSID);
    strcpy(networkPswd, MYPSWD);
    SerialLog(6, "configure failed");
  }
}

void connectToWiFi(char *ssid, char *pwd) {
  strbuffer = "Connecting to WiFi network: " + String(ssid);
  SerialLog(6, strbuffer);
  // Serial.println("Connecting to WiFi network: " + String(ssid));
  //  delete old config
  WiFi.disconnect(true);
  // register event handler
  WiFi.onEvent(WiFiEvent);

  // Initiate connection
  WiFi.begin(ssid, pwd);
  SerialLog(6, "Waiting for WIFI connection...");
}

// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      // When connected set
      strbuffer = "WiFi connected! IP address: " + WiFi.localIP().toString();
      SerialLog(5, strbuffer);
      myip = WiFi.localIP();
      broadcast = myip;
      broadcast[3] = 255;
      // initializes the UDP state
      // This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      SerialLog(4, "WiFi lost connection");
      connected = false;
      break;
    default:
      break;
  }
}

/*
0 Emergency: system is unusable

1 Alert: action must be taken immediately

2 Critical: critical conditions

3 Error: error conditions

4 Warning: warning conditions

5 Notice: normal but significant condition

6 Informational: informational messages

7 Debug: debug-level messages

*/
void SerialLog(int level, String a) {
  Serial.print("[");
  Serial.print(millis() / 1000.0);
  Serial.print("]    ");
  switch (level) {
    case 0:
      Serial.print("[Emergency] ");
      break;
    case 1:
      Serial.print("[Alert] ");
      break;
    case 2:
      Serial.print("[Critical] ");
      break;
    case 3:
      Serial.print("[Error] ");
      break;
    case 4:
      Serial.print("[Warning] ");
      break;
    case 5:
      Serial.print("[Notice] ");
      break;
    case 6:
      Serial.print("[Info] ");
      break;
    case 7:
      Serial.print("[Debug] ");
      break;
    default:
      Serial.print("[?] ");
      break;
  }
  Serial.println(a);
}