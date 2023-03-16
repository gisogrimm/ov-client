#include <OSCBoards.h>
#include <OSCData.h>
#include <OSCMatch.h>
#include <OSCMessage.h>
#include <OSCTiming.h>
#include <SLIPEncodedSerial.h>
#include <SLIPEncodedUSBSerial.h>

#include <WiFi.h>
#include <WiFiUdp.h>

#include <Wire.h>

#define DEBUG
#include "MPU6050_6Axis_MotionApps20.h"

MPU6050 mpu;

uint16_t remote_port = 0;
IPAddress next_remote_ip;
IPAddress remote_ip;
char remote_path[1024];

OSCMessage msg_data("/headtrack");

// MPU control/status vars
bool dmpReady = false;   // set true if DMP init was successful
uint8_t mpuIntStatus;    // holds actual interrupt status byte from MPU
uint8_t devStatus;       // return status after each device operation (0 = success, !0
                         // = error)
uint16_t packetSize;     // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;      // count of all bytes currently in FIFO
uint8_t fifoBuffer[64];  // FIFO storage buffer

// orientation/motion vars
int16_t ax, ay, az;
int16_t gx, gy, gz;
double gx0 = 0;
double gy0 = 0;
double gz0 = 0;
float rotx = 0;
float roty = 0;
float rotz = 0;
float rotscale = 0.0609756;

Quaternion q;
VectorInt16 aa;  // [x, y, z]            accel sensor measurements
VectorInt16
  aaReal;  // [x, y, z]            gravity-free accel sensor measurements
VectorInt16
  aaWorld;            // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;  // [x, y, z]            gravity vector

double rt = 0;
double rtp = 0;
double dt = 0;

bool b_calibrating = false;
bool b_calibinit = true;

uint16_t calib_cnt = 0;

WiFiUDP Udp;

byte mac[6];

char ssid[32];                //  your network SSID (name)
char pass[] = "headtracker";  // your network password
IPAddress local_IP(192, 168, 100, 1);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  msg_data.add(1.0).add(1.0f).add(1.0f).add(1.0f).add(1.0f).add(1.0f).add(1.0f).add(1.0f);
  WiFi.macAddress(mac);
  sprintf(ssid, "head-%02x%02x%02x%02x", mac[2], mac[3], mac[4], mac[5]);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  Udp.begin(9999);

  pinMode(15, INPUT);
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);  // 400kHz I2C clock (200kHz if CPU is 8MHz). Comment
                          // this line if having compilation difficulties
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(187);
  mpu.setYGyroOffset(25);
  mpu.setZGyroOffset(-3);
  mpu.setZAccelOffset(1688);  // 1688 factory default for my test chip

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);
    // set our DMP Ready flag so the main loop() function knows it's okay to use
    // it
    dmpReady = true;
    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
}

void osc_connect(OSCMessage &msg) {
  if ((msg.size() == 2) && msg.isInt(0) && msg.isString(1)) {
    remote_port = msg.getInt(0);
    remote_ip = next_remote_ip;
    msg.getString(1, remote_path);
    msg_data.setAddress(remote_path);
    //msg_data.setupMessage();
  }
}

void osc_disconnect(OSCMessage &msg) {
  remote_port = 0;
}

void osc_calib(OSCMessage &msg) {
  b_calibinit = true;
}

void proc_osc() {
  int size = Udp.parsePacket();
  if (size > 0) {
    OSCMessage msg;
    while (size--) {
      msg.fill(Udp.read());
      next_remote_ip = Udp.remoteIP();
    }
    if (!msg.hasError()) {
      msg.dispatch("/connect", osc_connect);
      msg.dispatch("/disconnect", osc_disconnect);
      msg.dispatch("/calib", osc_calib);
    }
  }
}

void send_msg() {
  if (remote_port > 0) {
    Udp.beginPacket(remote_ip, remote_port);
    msg_data.send(Udp);
    Udp.endPacket();
  }
}

void loop() {
  proc_osc();
  if (!dmpReady)
    return;
  mpuIntStatus = mpu.getIntStatus();
  // get current FIFO count
  fifoCount = mpu.getFIFOCount();
  // check for overflow (this should never happen unless our code is too
  // inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    // otherwise, check for DMP data ready interrupt (this should happen
    // frequently)
  } else if (mpuIntStatus & 0x02) {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize)
      fifoCount = mpu.getFIFOCount();
    // read a packet from FIFO, then clear the buffer
    mpu.getFIFOBytes(fifoBuffer, packetSize);
    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;
    rt = 0.001 * millis();
    dt = rt - rtp;
    rtp = rt;
    if (b_calibinit || (digitalRead(15) && (!b_calibrating))) {
      b_calibinit = false;
      b_calibrating = true;
      Serial.println("C1");
      calib_cnt = 400;
      mpu.setXGyroOffset(0);
      mpu.setYGyroOffset(0);
      mpu.setZGyroOffset(0);
      gx0 = 0;
      gy0 = 0;
      gz0 = 0;
    }
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetAccel(&aa, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
    if (dt > 0) {
      msg_data.set(0, rt).set(1, q.w).set(2, q.x).set(3, q.y).set(4, q.z).set(5, rotx).set(6, roty).set(7, rotz);
      send_msg();
      // Serial.print('Q');
      // Serial.print(q.w);
      // Serial.print(',');
      // Serial.print(q.x);
      // Serial.print(',');
      // Serial.print(q.y);
      // Serial.print(',');
      // Serial.print(q.z);
      // Serial.print(": ");
      // Serial.print(remote_ip.toString());
      // Serial.print(' ');
      // Serial.println(remote_path);

      // Serial.print("W");
      // Serial.print(aaWorld.x);
      // Serial.print(',');
      // Serial.print(aaWorld.y);
      // Serial.print(',');
      // Serial.println(aaWorld.z);

      // Serial.print('R');
      // Serial.print(dt);
      // Serial.print(',');
      // Serial.print(ax);
      // Serial.print(',');
      // Serial.print(ay);
      // Serial.print(',');
      // Serial.print(az);
      // Serial.print(',');
      // Serial.print(gx);
      // Serial.print(',');
      // Serial.print(gy);
      // Serial.print(',');
      // Serial.println(gz);
    }
    if (calib_cnt) {
      gx0 += gx;
      gy0 += gy;
      gz0 += gz;
      calib_cnt--;
      if (calib_cnt == 0) {
        b_calibrating = false;
        gx0 /= 400.0;
        gy0 /= 400.0;
        gz0 /= 400.0;
        rotx = 0;
        roty = 0;
        rotz = 0;
        // by some reason unknown to me, the offset has to be scaled by -2 to achieve correct values:
        mpu.setXGyroOffset(-2 * gx0);
        mpu.setYGyroOffset(-2 * gy0);
        mpu.setZGyroOffset(-2 * gz0);
        Serial.println("C0");
        Serial.print('O');
        Serial.print(mpu.getXGyroOffset());
        Serial.print(',');
        Serial.print(mpu.getYGyroOffset());
        Serial.print(',');
        Serial.print(mpu.getZGyroOffset());
        Serial.print(',');
        Serial.print(gx0);
        Serial.print(',');
        Serial.print(gy0);
        Serial.print(',');
        Serial.println(gz0);
      }
    } else {
      if (dt > 0) {
        //rotx += (gx - gx0) * dt * rotscale;
        //roty += (gy - gy0) * dt * rotscale;
        //rotz += (gz - gz0) * dt * rotscale;
        rotx += gx * dt * rotscale;
        roty += gy * dt * rotscale;
        rotz += gz * dt * rotscale;
        // Serial.print('G');
        // Serial.print(rotx);
        // Serial.print(',');
        // Serial.print(roty);
        // Serial.print(',');
        // Serial.println(rotz);
        //Serial.print('A');
        //Serial.print(ax);
        //Serial.print(',');
        //Serial.print(ay);
        //Serial.print(',');
        //Serial.println(az);
      }
    }
  }
}

/*
   Local Variables:
   mode: c++
   c-basic-offset: 2
   End:
*/
