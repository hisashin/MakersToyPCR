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

#define OFFLINE_DEMO // No WiFi
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
//#define FORCE_NORMAL_MODE // FOR DEBUG

/*
void setup () {
    Serial.begin(9600);
    Serial.println("Hello NinjaPCR!");
    CPlateThermistor iPlateThermistor;
    CLidThermistor iLidThermistor;
    initADC();
}
*/

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
    //93℃に達したら落ちた
    gpThermocycler->ipSerialControl->ProcessDummyMessage(SEND_CMD, "s=ACGTC&c=start&d=30261&l=100&n=Simple test&p=(1[30|95|Initial Step|0])(3[30|95|Step|0][30|55|Step|0][60|72|Step|0])(1[0|20|Final Hold|0]) ");
    Serial.println("Start!");
    return;
#endif
#ifdef USE_WIFI
    Serial.println("Starting NinjaPCR WiFi");
    pinMode(PIN_WIFI_MODE, INPUT);
    isApMode = (digitalRead(PIN_WIFI_MODE)==VALUE_WIFI_MODE_AP);
#ifdef FORCE_AP_MODE
  isApMode = true;
  Serial.println("FORCE_AP_MODE");
#endif

#ifdef FORCE_NORMAL_MODE
  Serial.println("FORCE_NORMAL_MODE");
  isApMode = false;
#endif
    if (isApMode) {
        Serial.println("AP mode");
        startWiFiAPMode();
    }
    else {
        Serial.println("Server mode");
        setup_normal();
        wifi = new WifiCommunicator(wifi_receive, wifi_send);
        gpThermocycler->SetCommunicator(wifi);
        Serial.println("Starting WiFi...");
        startWiFiHTTPServer();
    }
#else
    setup_normal();
#endif
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

    Serial.println("Init Thermocycler 0");
    gpThermocycler = new Thermocycler(restarted);
    Serial.println("Init Thermocycler 1");

}

bool isSerialConnected = false;
bool initDone = false;
short INTERVAL_MSEC = 500;

int sec = 0;
void loop() {
#ifdef OFFLINE_DEMO
    long startMillis = millis();
    gpThermocycler->Loop();
    gpThermocycler->ipSerialControl->ProcessDummyMessage(STATUS_REQ, "");
    long elapsed =  millis() - startMillis;
    Serial.print(sec++);
    Serial.print("Elapsed=");Serial.println(elapsed);
    if (elapsed<0 || elapsed > 1000) {
      elapsed = 0;
    }
    delay(1000-elapsed);
    return;
#endif
#ifdef USE_WIFI
    if (isApMode) {
        loopWiFiAP();
        return;
    }
    gpThermocycler->Loop();
    delay(1000);
    // TODO accurate timing
    if (isWiFiConnected()) {
        loopWiFiHTTPServer();
    } else {
        Serial.println("WiFi Disconnected");
    }
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
