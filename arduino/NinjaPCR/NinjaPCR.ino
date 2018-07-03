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
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "serialcontrol_chrome.h"
#include "wifi_communicator.h"

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

void setup() {
    Serial.begin(BAUD_RATE);
    pinMode(PIN_WIFI_MODE, INPUT);
    delay(100);
    isApMode = (digitalRead(PIN_WIFI_MODE)==VALUE_WIFI_MODE_AP);
    delay(250);
    PCR_DEBUG("NinjaPCR ver. "); 
    PCR_DEBUG_LINE(OPENPCR_FIRMWARE_VERSION_STRING);
    delay(3000);
    initHardware();
    EEPROM.begin(1024);

    PCR_DEBUG("PIN_WIFI_MODE=");
    PCR_DEBUG_LINE(isApMode);
    if (isApMode) {
        PCR_DEBUG_LINE("AP mode");
        startWiFiAPMode();
    }
    else {
        PCR_DEBUG_LINE("Server mode");
        startWiFiHTTPServer();
        setup_normal();
        wifi = new WifiCommunicator(wifi_receive, wifi_send);
        gpThermocycler->SetCommunicator(wifi);
    }
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
    unsigned long startMillis;
    unsigned long elapsed;
    if (isApMode) {
        
        delay(100);
        loopWiFiAP();
        return;
    }
    startMillis = millis();
    gpThermocycler->Loop();
    
    if (isWiFiConnected()) {
        loopWiFiHTTPServer();
    }
    
    elapsed = millis() - startMillis;
    if (elapsed<0 || elapsed > INTERVAL_MSEC) {
        elapsed = 0;
    }
    if (gpThermocycler->GetProgramState() == Thermocycler::ProgramState::EComplete) {
        PCR_DEBUG_LINE("COMPLETE");
        if (!finishSent) {
            // TODO call IFTTT API if needed
            finishSent = true;
        }
    }
    delay(INTERVAL_MSEC-elapsed);
}

bool startLamp = false;
void checkSerialConnection() {
    PCR_DEBUG("pcr1.0.7"); //TODO
    Serial.print("\n");
#ifdef USE_STATUS_PINS
    digitalWrite(PIN_STATUS_A, (startLamp)?HIGH:LOW);
#endif /* USE_STATUS_PINS */
    startLamp = !startLamp;
    unsigned long timeStart = millis();
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
