#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
// WiFi ap network name and password:
const char *ssid = "yourAP";
const char *password = "yourPassword";

//IP address to send UDP data to:
const int udpPort = 3333;
IPAddress myIP;
//The udp library class
WiFiUDP udp;

const int pin_INT1 = 04;  // data ready
const int pin_SS1 = SS;   // CSB

const int pin_DRDYB = 02;  // data ready
const int pin_SS2 = 17;    // CSB

void setup() {
  pinMode(pin_INT1, INPUT);
  pinMode(pin_SS1, OUTPUT);
  pinMode(pin_DRDYB, INPUT);
  pinMode(pin_SS2, OUTPUT);

  Serial.begin(115200);  // (less than 115200 will decimate the signal -> de facto LPF)

  SPI.begin();
  Serial.println("SPI.begin");
  delay(1000);
  setup_LSM6DSL(pin_SS1);
  Serial.println("setup_LSM6DSL");
  delay(1000);
  setup_ECG(pin_SS2);
  Serial.println("setup_ECG");
  delay(1000);
  Serial.println();
  Serial.println("Configuring access point...");
  // You can remove the password parameter if you want the AP to be open.
  // a valid password must have more than 7 characters
  //if (!WiFi.softAP(ssid, password)) {
  if (!WiFi.softAP(ssid)) {
    log_e("Soft AP creation failed.");
    while (1)
      ;
  }
  myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  delay(1000);
  udp.begin(myIP, udpPort);
}
char tail[4] = { 0x00, 0x00, 0x80, 0x7f };
float buffer[3];
bool buffer_success1 = 0;
bool buffer_success2 = 0;
void loop() {
  delay(1);
  getIMUdata(pin_SS1);
  if (!buffer_success1) {
    Serial.print("[");
    Serial.print(millis() / 1000.0);
    Serial.print("]\t");
    Serial.println("imu bug");
    return;
  }

  getECGdata(pin_SS2);
  if (!buffer_success2) {
    Serial.print("[");
    Serial.print(millis() / 1000.0);
    Serial.print("]\t");
    Serial.println("ecg bug");
    return;
  }
  //only send data when connected

  //Send a packet
  udp.beginPacket("192.168.4.255", udpPort);
  buffer[2] = micros();
  udp.print(buffer[0]);
  udp.print(',');
  udp.print(buffer[1]);
  udp.print(',');
  udp.print(buffer[2]);
  udp.print('\n');
  udp.endPacket();
}

int32_t temp;
int32_t tmp1 = 0;
int32_t tmp2 = 0;
int32_t tmp3 = 0;
void getIMUdata(int pin_SS) {
  if (digitalRead(pin_INT1) == true) {

    // sampled data is located at 3 8-bit registers
    byte x1 = readRegister(pin_SS, 0x2C);
    byte x2 = readRegister(pin_SS, 0x2D);

    // 3 8-bit registers combination on a 24 bit number
    temp = x2;
    temp = (temp << 8) | x1;

    tmp1 = tmp2;
    tmp2 = tmp3;
    tmp3 = temp;

    if (abs((tmp1 - tmp2) + (tmp3 - tmp2)) > 450) {
      buffer_success1 = 0;
      Serial.print("[");
      Serial.print(millis() / 1000.0);
      Serial.print("]\t");
      Serial.println("imu glitch");

      return;
    }
    //Serial.println(temp);

    //Wait for 1 second
    //delay(1000);
    buffer[0] = tmp2;
    buffer_success1 = 1;
  } else {
    buffer_success1 = 0;
    Serial.print("[");
    Serial.print(millis() / 1000.0);
    Serial.print("]\t");
    Serial.println("imu 0");
  }
}

int32_t ecgVal;
int32_t ecgTmp1 = 0;
int32_t ecgTmp2 = 0;
int32_t ecgTmp3 = 0;

void getECGdata(int pin_SS) {
  if (digitalRead(pin_DRDYB) == false) {

    // sampled data is located at 3 8-bit registers
    byte x1 = readRegister(pin_SS, 0x37);
    byte x2 = readRegister(pin_SS, 0x38);
    byte x3 = readRegister(pin_SS, 0x39);

    // 3 8-bit registers combination on a 24 bit number
    ecgVal = x1;
    ecgVal = (ecgVal << 8) | x2;
    ecgVal = (ecgVal << 8) | x3;

    ecgTmp1 = ecgTmp2;
    ecgTmp2 = ecgTmp3;
    ecgTmp3 = ecgVal;

    if (abs((ecgTmp1 - ecgTmp2) + (ecgTmp3 - ecgTmp2)) > 29000) {
      buffer_success1 = 0;
      Serial.print("[");
      Serial.print(millis() / 1000.0);
      Serial.print("]\t");
      Serial.println("ecg glitch");
      return;
    }

    buffer[1] = ecgTmp2;
    buffer_success2 = 1;
  } else {
    buffer_success2 = 0;
    Serial.print("[");
    Serial.print(millis() / 1000.0);
    Serial.print("]\t");
    Serial.println("ecg 0");
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
  //Follow the next steps to configure the device for this example, starting from default registers values.
  //1. Set address 0x01 = 0x11: Connect channel 1’s INP to IN2 and INN to IN1.
  writeRegister(pin_SS, 0x01, 0x11);
  //2. Set address 0x02 = 0x19: Connect channel 2’s INP to IN3 and INN to IN1.
  writeRegister(pin_SS, 0x02, 0x19);
  //3. Set address 0x0A = 0x07: Enable the common-mode detector on input pins IN1, IN2 and IN3.
  writeRegister(pin_SS, 0x0A, 0x07);
  //4. Set address 0x0C = 0x04: Connect the output of the RLD amplifier internally to pin IN4.
  writeRegister(pin_SS, 0x0C, 0x04);
  //5. Set address 0x12 = 0x04: Use external crystal and feed the internal oscillator's output to the digital.
  writeRegister(pin_SS, 0x12, 0x04);
  //6. Set address 0x14 = 0x24: Shuts down unused channel 3’s signal path.
  writeRegister(pin_SS, 0x14, 0x24);
  //7. Set address 0x21 = 0x02: Configures the R2 decimation rate as 5 for all channels.
  writeRegister(pin_SS, 0x21, 0x02);
  //8. Set address 0x22 = 0x02: Configures the R3 decimation rate as 6 for channel 1.
  writeRegister(pin_SS, 0x22, 0x02);
  //9. Set address 0x23 = 0x02: Configures the R3 decimation rate as 6 for channel 2.
  writeRegister(pin_SS, 0x23, 0x02);
  //10. Set address 0x27 = 0x08: Configures the DRDYB source to channel 1 ECG (or fastest channel).
  writeRegister(pin_SS, 0x27, 0x08);
  //11. Set address 0x2F = 0x30: Enables channel 1 ECG and channel 2 ECG for loop read-back mode.
  writeRegister(pin_SS, 0x2F, 0x30);
  //12. Set address 0x00 = 0x01: Starts data conversion.
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
