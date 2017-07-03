/*
 *  serialcontrol.cpp - OpenPCR control software.
 *  Copyright (C) 2010-2012 Josh Perfetto and Xia Hong. All Rights Reserved.
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

#include "pcr_includes.h"
#include "display.h"
#include "serialcontrol_chrome.h"

unsigned char remainingBody = 0;
boolean waitingForMessage = true;
boolean startFound = false;
unsigned char bodyLength = 0;
int nextByteIndex = 0;
unsigned long serialStart = 0;
int SERIAL_TIMEOUT_MSEC = 2000;

SerialControl::SerialControl(Display* pDisplay) : Communicator () {
  ipDisplay = pDisplay;
}

boolean SerialControl::ParseWholeMessage() {
  serialStart = millis();
  SERIAL_STATUS serialStatus = SERIAL_CONTINUE;
  do {
    serialStatus = ReadPacket();
  } while (serialStatus==SERIAL_CONTINUE);

  if (serialStatus==SERIAL_RECEIVED) {
    return true;
  }
  return false;
}

SERIAL_STATUS SerialControl::ReadPacket() {
  if (Serial.available()) {
    unsigned char readByte = Serial.read();
    unsigned char startCode = 0xFF;
    unsigned char endCode = 0xFE;

    if (waitingForMessage && readByte == startCode) {
      // Start code found.
      startFound = true;
      waitingForMessage = false;
      nextByteIndex++;
    }
    else if (nextByteIndex == 1) {
      // Read command code
      int code = readByte + 0;
      currentCommand = readByte; //SEND_CMD or  STATUS_REQ
      nextByteIndex++;
    }
    else if (nextByteIndex == 2) {
      // Read body length
      bodyLength = readByte;
      int len = 0 + readByte;
      remainingBody = bodyLength;
      nextByteIndex++;
    }
    else if (remainingBody > 0) {
      //Read Body
      commandBody[nextByteIndex - 3] = readByte;
      remainingBody--;
      nextByteIndex++;
    }
    else if (endCode == readByte && remainingBody == 0) {
      //Finish successfully (read)
      return SERIAL_RECEIVED; // Finish (Success)
    }
    else if (millis() > serialStart + SERIAL_TIMEOUT_MSEC) {
      return SERIAL_TIMEOUT; // Finish (Timeout)
    }
    else {
      FinishReading(); // Refresh and wait for valid message
    }
    return SERIAL_CONTINUE;
  }
  else if (millis() > serialStart + SERIAL_TIMEOUT_MSEC) {
    // Timeout (Serial not available)
    return SERIAL_TIMEOUT; // Finish (Timeout)
  } else {
    // Waiting
    return SERIAL_CONTINUE;
  }
}

void SerialControl::SendResponse(char *response, int size) {
  //send packet
  Serial.write(START_CODE);
  Serial.write(STATUS_RESP);

  Serial.write((byte) size);
  Serial.write((byte*) response, size);
  Serial.write(ESCAPE_CODE);
}
void SerialControl::OnFinishReaing () {
  startFound = false;
  waitingForMessage = true;
  bodyLength = 0;
  nextByteIndex = 0;
}
