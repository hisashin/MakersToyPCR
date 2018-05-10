/*
 *  openpcr.pde - OpenPCR control software.
 *  Copyright (C) 2010-2012 Josh Perfetto. All Rights Reserved.
 *
 *  OpenPCR control software is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenPCR control software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the OpenPCR control software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <LiquidCrystal.h>
#include <EEPROM.h>

#include "pcr_includes.h"
#include "adc.h"
#include "thermocycler.h"
#include "thermistors.h"

// #define OFFLINE_DEMO // No WiFi
#ifdef USE_WIFI

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "serialcontrol_chrome.h"
#include "wifi_communicator.h"


#endif /* USE_WIFI */

Thermocycler* gpThermocycler = NULL;
WifiCommunicator *wifi = NULL;

boolean isInitialStart() {
    for (int i = 0; i < 50; i++) {
        if (EEPROM.read(i) != 0xFF)
            return false;
    }
    return true;
}

const SPIDTuning LID_PID_GAIN_SCHEDULE2[] =
    {
    //maxTemp, kP, kI, kD
                { 70, 40, 0.15, 60 },
                { 200, 80, 1.1, 10 } };


bool isApMode = false;

// #define FORCE_AP_MODE // For debug
#define FORCE_NORMAL_MODE // FOR DEBUG

void setup() {
    Serial.begin(BAUD_RATE);
    EEPROM.begin(4096);
    
#ifdef OFFLINE_DEMO
    // Skip network
    Serial.println("OFFLINE DEMO.");
    gpThermocycler = new Thermocycler(false);
    gpThermocycler->ipSerialControl = new SerialControl(NULL);
    // Profile
    Serial.println("Input demo profile");
    /*
    95℃ 2min
    (95℃ 30sec / 55℃ 30sec / 72℃ 30sec ) x 35
     */
     Serial.println("s=ACGTC&c=start&d=30261&l=20&n=Simple test&p=(1[30|95|Initial|0])(35[60|95|High|0][60|55|Low|0][60|72|Med|0])(1[0|20|Final Hold|0])");
     gpThermocycler->ipSerialControl->ProcessDummyMessage(SEND_CMD, "s=ACGTC&c=start&d=30261&l=20&n=Simple test&p=(1[60|95|Initial|0])(35[60|95|High|0][60|55|Low|0][60|72|Med|0])(1[0|20|Final Hold|0]) ");


     // Dummy profile (for keeping lid temp)
     //Serial.println("s=ACGTC&c=start&d=30261&l=110&n=Simple test&p=(1[120|95|Initial|0])(35[30|95|High|0][30|55|Low|0][30|72|Med|0])(1[0|20|Final Hold|0])");
     //gpThermocycler->ipSerialControl->ProcessDummyMessage(SEND_CMD, "s=ACGTC&c=start&d=30261&l=110&n=Simple test&p=(1[120|95|Initial|0])(35[30|95|High|0][30|55|Low|0][30|72|Med|0])(1[0|20|Final Hold|0]) ");

    return;
#endif
#ifdef USE_WIFI
    Serial.println("NinjaPCR WiFi");
    pinMode(PIN_WIFI_MODE, INPUT);
    isApMode = (digitalRead(PIN_WIFI_MODE)==VALUE_WIFI_MODE_AP);
#ifdef FORCE_AP_MODE
  isApMode = true;
  Serial.println("FORCE_AP");
#endif /* FORCE_AP_MODE */

#ifdef FORCE_NORMAL_MODE
  Serial.println("FORCE_NORMA");
  isApMode = false;
#endif /* FORCE_NORMAL_MODE */
    if (isApMode) {
        Serial.println("AP mode");
        startWiFiAPMode();
    }
    else {
        Serial.println("Server mode");
        setup_normal();
        wifi = new WifiCommunicator(wifi_receive, wifi_send);
        gpThermocycler->SetCommunicator(wifi);
        startWiFiHTTPServer();
    }
#else
    setup_normal();
#endif /* USE_WIFI */
}

void setup_normal() {

#ifdef USE_STATUS_PINS
    pinMode(PIN_STATUS_A,OUTPUT);
    pinMode(PIN_STATUS_B,OUTPUT);
    digitalWrite(PIN_STATUS_A,LOW);
    digitalWrite(PIN_STATUS_B,LOW);
#endif /* USE_STATUS_PINS */

    //init factory settings
    if (isInitialStart()) {
        EEPROM.write(0, 100); // set contrast to 100
    }
    //restart detection

#ifdef USE_WIFI
    //TODO EEPROM
    boolean restarted = false;
#else
    boolean restarted = !(MCUSR & 1);
    MCUSR &= 0xFE;
#endif /* USE_WIFI */

#ifdef USE_STATUS_PINS
    digitalWrite(PIN_STATUS_A, HIGH);
#endif /* USE_STATUS_PINS */
    gpThermocycler = new Thermocycler(restarted);
}

bool isSerialConnected = false;
bool initDone = false;
short INTERVAL_MSEC = 1000;

int sec = 0;
bool finishSent = false;
void loop() {
    long startMillis;
    long elapsed;
#ifdef OFFLINE_DEMO
    startMillis = millis();
    gpThermocycler->Loop();
    gpThermocycler->ipSerialControl->ProcessDummyMessage(STATUS_REQ, "");
    elapsed =  millis() - startMillis;
    sec++;
    //Serial.print();Serial.print(sec++);
    //Serial.print("Elapsed=");Serial.println(elapsed);
    if (elapsed<0 || elapsed > INTERVAL_MSEC) {
      elapsed = 0;
    }
    delay(INTERVAL_MSEC-elapsed);
    if (gpThermocycler->GetProgramState() == Thermocycler::ProgramState::EComplete) {
      Serial.println("COMPLETE");
      if (!finishSent) {
         //slack_send("NinjaPCR_profile_finished_" + String(sec) + "sec");
         finishSent = true;
      }
    }
    return;
#endif
#ifdef USE_WIFI
    if (isApMode) {
        loopWiFiAP();
        return;
    }
    startMillis = millis();
    gpThermocycler->Loop();
    // TODO accurate timing
    if (isWiFiConnected()) {
        loopWiFiHTTPServer();
    } else {
        Serial.println("Disconnected");
    }
    elapsed =  millis() - startMillis;
    if (elapsed<0 || elapsed > INTERVAL_MSEC) {
      elapsed = 0;
    }
    if (gpThermocycler->GetProgramState() == Thermocycler::ProgramState::EComplete) {
      Serial.println("COMPLETE");
      if (!finishSent) {
         //slack_send("NinjaPCR_finished_" + String(sec) + "sec");
         finishSent = true;
      }
    }
    Serial.print(elapsed); Serial.println("sec");
    delay(INTERVAL_MSEC-elapsed);
#else
    if (isSerialConnected) {
        gpThermocycler->Loop();
        delay(1000);
    }
    else {
        checkSerialConnection();
    }
#endif
}

bool startLamp = false;
void checkSerialConnection() {
    Serial.print("pcr1.0.5");
    Serial.print("\n");
#ifdef USE_STATUS_PINS
    digitalWrite(PIN_STATUS_A, (startLamp)?HIGH:LOW);
#endif /* USE_STATUS_PINS */
    startLamp = !startLamp;
    int timeStart = millis();
    while (millis() < timeStart + INTERVAL_MSEC) {
        while (Serial.available()) {
            char ch = Serial.read();
            if (ch == 'a' && !isSerialConnected) {
#ifdef USE_STATUS_PINS
                digitalWrite(PIN_STATUS_A, LOW);
#endif /* USE_STATUS_PINS */
                isSerialConnected = true;
            }
        }
    }
}
