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
#include "thermocycler.h"


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

boolean InitialStart() {
  for (int i = 0; i < 50; i++) {
    if (EEPROM.read(i) != 0xFF)
      return false;
  }
  
  return true;
}

const SPIDTuning LID_PID_GAIN_SCHEDULE2[] = {
  //maxTemp, kP, kI, kD
  { 
    70, 40, 0.15, 60   }
  ,
  { 
    200, 80, 1.1, 10   }
};
void setup() {
#ifdef USE_STATUS_PINS
  pinMode(PIN_STATUS_A,OUTPUT);
  pinMode(PIN_STATUS_B,OUTPUT);
  digitalWrite(PIN_STATUS_A,LOW);
  digitalWrite(PIN_STATUS_B,LOW);
#endif /* USE_STATUS_PINS */
  
#ifdef USE_WIFI
  Serial.begin(115200);
  Serial.println("Starting NinjaPCR WiFi");
#else
  Serial.begin(4800);
  Serial.println("Starting NinjaPCR Serial");
  }
#endif /* USE_WIFI */

  //init factory settings
  if (InitialStart()) {
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
  
  Serial.println("init 3");

#ifdef USE_STATUS_PINS
  digitalWrite(PIN_STATUS_A, HIGH);
#endif /* USE_STATUS_PINS */
  
  Serial.println("Init Thermocycler 0");
  gpThermocycler = new Thermocycler(restarted);
  Serial.println("Init Thermocycler 1");
  
#ifdef USE_WIFI
  wifi = new WifiCommunicator(wifi_receive, wifi_send);
  gpThermocycler->SetCommunicator(wifi);
  Serial.println("Starting WiFi...");
  network_start();
#endif /* USE_WIFI */
}
 
bool connected = false;
bool initDone = false;
short INTERVAL_MSEC = 500;

void loop() {
#ifdef USE_WIFI
  if (network_is_connected()) {
#else
  if (connected) {
#endif /* USE_WIFI */
    gpThermocycler->Loop();
    delay(1000);
  } else {
#ifndef USE_WIFI
  checkPlugged();
#endif /* USE_WIFI */
  }
#ifdef USE_WIFI
  network_loop();
#endif /* USE_WIFI */
}

bool startLamp = false;
void checkPlugged () {
    Serial.print("pcr1.0.5");
    Serial.print("\n");
 #ifdef USE_STATUS_PINS
    digitalWrite(PIN_STATUS_A, (startLamp)?HIGH:LOW);
#endif /* USE_STATUS_PINS */
    startLamp = !startLamp;
    int timeStart = millis();
    while (millis()<timeStart+INTERVAL_MSEC) {
      while (Serial.available()){
        char ch = Serial.read();
        if (ch=='a'&&!connected) {
 #ifdef USE_STATUS_PINS
          digitalWrite(PIN_STATUS_A, LOW);
#endif /* USE_STATUS_PINS */
          
          connected = true;
        }
      }
    }
}
