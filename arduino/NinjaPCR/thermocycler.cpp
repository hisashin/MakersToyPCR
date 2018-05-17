/*
 *  thermocycler.cpp - OpenPCR control software.
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

#include "pcr_includes.h"
#include "thermocycler.h"

#include "program.h"
#include "serialcontrol_chrome.h"
#include "wifi_communicator.h"
#include "../Wire/Wire.h"
#include "display.h"

#ifndef USE_WIFI
#include <avr/pgmspace.h>
#endif

//constants

// I2C address for MCP3422 - base address for MCP3424
#define MCP3422_ADDRESS 0X68
#define MCP342X_RES_FIELD  0X0C // resolution/rate field
#define MCP342X_18_BIT     0X0C // 18-bit 3.75 SPS
#define MCP342X_BUSY       0X80 // read: output not ready

#define CYCLE_START_TOLERANCE 1.0
#define LID_START_TOLERANCE 1.0
//#define CYCLE_START_TOLERANCE 8.0
//#define LID_START_TOLERANCE 12.0

// #define PID_CONF_ORIGINAL_OPENPCR
// #define PID_CONF_NINJAPCR_WIFI
#define PID_CONF_SIMULATED_TUBE_TEMP

#ifdef PID_CONF_ORIGINAL_OPENPCR
#define PLATE_PID_INC_NORM_P 1000
#define PLATE_PID_INC_NORM_I 250
#define PLATE_PID_INC_NORM_D 250

#define PLATE_PID_INC_LOW_THRESHOLD 40
#define PLATE_PID_INC_LOW_P 600
#define PLATE_PID_INC_LOW_I 200
#define PLATE_PID_INC_LOW_D 400

#define PLATE_PID_DEC_HIGH_THRESHOLD 70
#define PLATE_PID_DEC_HIGH_P 800
#define PLATE_PID_DEC_HIGH_I 700
#define PLATE_PID_DEC_HIGH_D 300

#define PLATE_PID_DEC_NORM_P 500
#define PLATE_PID_DEC_NORM_I 400
#define PLATE_PID_DEC_NORM_D 200

#define PLATE_PID_DEC_LOW_THRESHOLD 35
#define PLATE_PID_DEC_LOW_P 2000
#define PLATE_PID_DEC_LOW_I 100
#define PLATE_PID_DEC_LOW_D 200

#define PLATE_BANGBANG_THRESHOLD 2.0
#endif

#ifdef PID_CONF_NINJAPCR_WIFI

#define PLATE_PID_INC_NORM_P 2000
#define PLATE_PID_INC_NORM_I 250
#define PLATE_PID_INC_NORM_D 250

#define PLATE_PID_INC_LOW_THRESHOLD 40
#define PLATE_PID_INC_LOW_P 1200
#define PLATE_PID_INC_LOW_I 200
#define PLATE_PID_INC_LOW_D 400

#define PLATE_PID_DEC_HIGH_THRESHOLD 70
#define PLATE_PID_DEC_LOW_THRESHOLD 35

#define PLATE_PID_DEC_HIGH_P 1600
#define PLATE_PID_DEC_HIGH_I 700
#define PLATE_PID_DEC_HIGH_D 300

#define PLATE_PID_DEC_NORM_P 2000
#define PLATE_PID_DEC_NORM_I 500
#define PLATE_PID_DEC_NORM_D 200

#define PLATE_PID_DEC_LOW_P 4000
#define PLATE_PID_DEC_LOW_I 100
#define PLATE_PID_DEC_LOW_D 200

#define PLATE_BANGBANG_THRESHOLD 1.5

#endif /* PID_CONF_NINJAPCR_WIFI */


#ifdef PID_CONF_SIMULATED_TUBE_TEMP

#define PID_RATIO 0.3
#define PLATE_PID_INC_NORM_P (1000*PID_RATIO)
#define PLATE_PID_INC_NORM_I (250*PID_RATIO)
#define PLATE_PID_INC_NORM_D (250*PID_RATIO)

#define PLATE_PID_INC_LOW_THRESHOLD 80
#define PLATE_PID_INC_LOW_P (600*PID_RATIO)
#define PLATE_PID_INC_LOW_I (200*PID_RATIO)
#define PLATE_PID_INC_LOW_D (400*PID_RATIO)

#define PLATE_PID_DEC_HIGH_THRESHOLD 70
#define PLATE_PID_DEC_HIGH_P (800*0.8)
#define PLATE_PID_DEC_HIGH_I (700*0.8)
#define PLATE_PID_DEC_HIGH_D (300*0.8)

#define PLATE_PID_DEC_NORM_P (500*0.8*10.0)
#define PLATE_PID_DEC_NORM_I (400*0.8*10.0)
#define PLATE_PID_DEC_NORM_D (200*0.8*3.0)

#define PLATE_PID_DEC_LOW_THRESHOLD 35
#define PLATE_PID_DEC_LOW_P (2000*0.8*1.5)
#define PLATE_PID_DEC_LOW_I (100*0.8*1.5)
#define PLATE_PID_DEC_LOW_D (200*0.8*1.5)

#define PLATE_BANGBANG_THRESHOLD 5.0

#endif /* PID_CONF_SIMULATED_TUBE_TEMP */
#define MIN_PELTIER_PWM -1023
#define MAX_PELTIER_PWM 1023

#define MAX_LID_PWM 1023
#define MIN_LID_PWM 0

#define STARTUP_DELAY 4000

 /* Thermal resistance*/
#define THETA_WELL 3.0
#define THETA_LID 15.0
/* Capacity */
#define CAPACITY_TUBE 3.0

//pid parameters
/*
const SPIDTuning LID_PID_GAIN_SCHEDULE[] = {
  //maxTemp, kP, kI, kD
  { 
    70, 40, 0.15, 60   }
  ,
  { 
    200, 80, 1.1, 10   }
};
*/
const SPIDTuning LID_PID_GAIN_SCHEDULE[] = {
  //maxTemp, kP, kI, kD
  { 
    70, 160, 0.15, 60   }
  ,
  { 
    200, 320, 1.1, 10   }
};
//public
Thermocycler::Thermocycler(boolean restarted):
iRestarted(restarted),
ipDisplay(NULL),
ipProgram(NULL),
ipDisplayCycle(NULL),
ipSerialControl(NULL),
iProgramState(EStartup),
ipPreviousStep(NULL),
ipCurrentStep(NULL),
iThermalDirection(OFF),
iPeltierPwm(0),
iCycleStartTime(0),
iRamping(true),
//iPlatePid(&iPlateThermistor.GetTemp(), // Use measured well temp
iPlatePid(&iEstimatedSampleTemp, // Use estimated sample temp (test)
&iPeltierPwm, &iTargetPlateTemp, PLATE_PID_INC_NORM_P, PLATE_PID_INC_NORM_I, PLATE_PID_INC_NORM_D, DIRECT),
iLidPid(LID_PID_GAIN_SCHEDULE, MIN_LID_PWM, MAX_LID_PWM),
iEstimatedSampleTemp(25),
iTargetLidTemp(0) {
#ifndef USE_WIFI
#ifdef USE_LCD
  ipDisplay = new Display();
#else
  ipDisplay = NULL;
#endif /* USE_LCD */
  ipSerialControl = new SerialControl(ipDisplay);
#endif /* USE_WIFI */
  //init pins
  pinMode(PIN_LID_PWM, OUTPUT);
  // Peltier pins
  pinMode(PIN_WELL_INA, OUTPUT);
  pinMode(PIN_WELL_INB, OUTPUT);
  // Fan
#ifdef USE_FAN
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, PIN_FAN_VALUE_OFF);
#endif
  digitalWrite(PIN_WELL_INA, PIN_WELL_VALUE_OFF);
  digitalWrite(PIN_WELL_INB, PIN_WELL_VALUE_OFF);
  pinMode(PIN_WELL_PWM, OUTPUT);

#ifdef PIN_LCD_CONTRAST
  pinMode(5, OUTPUT);
#endif /* PIN_LCD_CONTRAST */

  // SPCR = 01010000 // TODO
  //interrupt disabled,spi enabled,msb 1st,master,clk low when idle,
  //sample on leading edge of clk,system clock/4 rate (fastest)
  int clr;
#ifndef USE_WIFI
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<4);
  clr=SPSR;
  clr=SPDR;
  delay(10); 

  iPlatePid.SetOutputLimits(MIN_PELTIER_PWM, MAX_PELTIER_PWM);

TCCR1A |= (1<<WGM11) | (1<<WGM10);
  // Peltier PWM
#ifdef TCCR2A //ATMEGA328
  TCCR1B = _BV(CS21);
  // Lid PWM
  TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS22);
#else
  /*
    TCCR1B = _BV(CS11);
    // Lid PWM
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11) | _BV(WGM10);
    TCCR1B = _BV(CS12);
   */
#endif /* TCCR2A */
#endif /* ifndef USE_WIFI */
  iszProgName[0] = '\0';
  iPlateThermistor.start();
}

Thermocycler::~Thermocycler() {
  delete ipSerialControl;
  delete ipDisplay;
}

// accessors
int Thermocycler::GetNumCycles() {
  return ipDisplayCycle->GetNumCycles();
}

int Thermocycler::GetCurrentCycleNum() {
  int numCycles = GetNumCycles();
  return ipDisplayCycle->GetCurrentCycle() > numCycles ? numCycles : ipDisplayCycle->GetCurrentCycle();
}

Thermocycler::ThermalState Thermocycler::GetThermalState() {
  if (iProgramState == EStartup || iProgramState == EStopped)
    return EIdle;

  if (iRamping) {
    if (ipPreviousStep != NULL) {
      return ipCurrentStep->GetTemp() > ipPreviousStep->GetTemp() ? EHeating : ECooling;
    } 
    else {
      return iThermalDirection == HEAT ? EHeating : ECooling;
    }
  } 
  else {
    return EHolding;
  }
}

// control
void Thermocycler::SetProgram(Cycle* pProgram, Cycle* pDisplayCycle, const char* szProgName, int lidTemp) {
  Stop();

  ipProgram = pProgram;
  ipDisplayCycle = pDisplayCycle;

  strcpy(iszProgName, szProgName);
  iTargetLidTemp = lidTemp;
}

void Thermocycler::Stop() {
  iProgramState = EStopped;

  ipProgram = NULL;
  ipPreviousStep = NULL;
  ipCurrentStep = NULL;

  iStepPool.ResetPool();
  iCyclePool.ResetPool();

  if (ipDisplay != NULL) {
    ipDisplay->Clear();
  }
}

PcrStatus Thermocycler::Start() {
	//Serial.print("st");
  if (ipProgram == NULL) {
    return ENoProgram;
  }

  //advance to lid wait state
  iProgramState = ELidWait;

  return ESuccess;
}

static boolean lamp = false;

// internal
void Thermocycler::Loop() {
#ifdef USE_STATUS_PINS
	digitalWrite(PIN_STATUS_A, (!lamp)?HIGH:LOW);
  digitalWrite(PIN_STATUS_B, (lamp)?HIGH:LOW);
#endif /* USE_STATUS_PINS */
	lamp = !lamp;
  ipSerialControl->Process();
  switch (iProgramState) {
  case EStartup:
    if (millis() > STARTUP_DELAY) {
      iProgramState = EStopped;
      	iRestarted = false;
      if (!iRestarted && !ipSerialControl->CommandReceived()) {
        //check for stored program
        SCommand command;
        if (ProgramStore::RetrieveProgram(command, (char*)ipSerialControl->GetBuffer()))
          ProcessCommand(command);
      }
    }
    break;

  case ELidWait:
    if (GetLidTemp() >= iTargetLidTemp - LID_START_TOLERANCE) {
      //lid has warmed, begin program
      iThermalDirection = OFF;
      iPeltierPwm = 0;
      PreprocessProgram();
      iProgramState = ERunning;

      ipProgram->BeginIteration();
      AdvanceToNextStep();

      iProgramStartTimeMs = millis();
    }
    break;

  case ERunning:
    //update program
    if (iProgramState == ERunning) {
      if (iRamping && abs(ipCurrentStep->GetTemp() - GetTemp()) <= CYCLE_START_TOLERANCE && GetRampElapsedTimeMs() > ipCurrentStep->GetRampDurationS() * 1000) {
        //begin step hold
        //eta updates
        if (ipCurrentStep->GetRampDurationS() == 0) {
          //fast ramp
          iElapsedFastRampDegrees += absf(GetTemp() - iRampStartTemp);
          iTotalElapsedFastRampDurationMs += millis() - iRampStartTime;
        }

        if (iRampStartTemp > GetTemp()) {
          iHasCooled = true;
        }
        iRamping = false;
        iCycleStartTime = millis();

      } 
      else if (!iRamping && !ipCurrentStep->IsFinal() && millis() - iCycleStartTime > (unsigned long)ipCurrentStep->GetStepDurationS() * 1000) {
        //begin next step
        AdvanceToNextStep();

        //check for program completion
        if (ipCurrentStep == NULL || ipCurrentStep->IsFinal()) {
          iProgramState = EComplete;        
        }
      }
    }
    break;

  case EComplete:
    if (iRamping && ipCurrentStep != NULL && abs(ipCurrentStep->GetTemp() - GetTemp()) <= CYCLE_START_TOLERANCE) {
      iRamping = false;
    }
    break;
  }
  //Read lid and well temp
  iPlateThermistor.ReadTemp();
  iLidThermistor.ReadTemp();
  ControlLid();
  
  double estimatedAirTemp = GetPlateTemp() * 0.4 + GetLidTemp() * 0.6; // TODO use actual air temperature
  iEstimatedSampleTemp += ((GetPlateTemp()-iEstimatedSampleTemp)/THETA_WELL + (estimatedAirTemp-iEstimatedSampleTemp)/THETA_LID ) / CAPACITY_TUBE;
  
  CalcPlateTarget();
  ControlPeltier();

  //program
  UpdateEta();
 #ifdef USE_LCD
  ipDisplay->Update();
  #endif
  ipSerialControl->Process();
}

void Thermocycler::SetCommunicator(Communicator *comm) {
  ipSerialControl = comm;
}
//private
void Thermocycler::AdvanceToNextStep() {
  ipPreviousStep = ipCurrentStep;
  ipCurrentStep = ipProgram->GetNextStep();
  if (ipCurrentStep == NULL)
    return;

  //update eta calc params
  if (ipPreviousStep == NULL || ipPreviousStep->GetTemp() != ipCurrentStep->GetTemp()) {
    iRamping = true;
    iRampStartTime = millis();
    iRampStartTemp = GetPlateTemp();
  } 
  else {
    iCycleStartTime = millis(); //next step starts immediately
  }

  CalcPlateTarget();
  SetPlateControlStrategy();
}

void Thermocycler::SetPlateControlStrategy() {
  if (InControlledRamp())
    return;

  if (absf(iTargetPlateTemp - GetPlateTemp()) >= PLATE_BANGBANG_THRESHOLD && !InControlledRamp()) {
    iPlateControlMode = EBangBang;
    iPlatePid.SetMode(MANUAL);
  } 
  else {
    iPlateControlMode = EPIDPlate;
    iPlatePid.SetMode(AUTOMATIC);
  }

  if (iRamping) {
    if (iTargetPlateTemp >= GetPlateTemp()) {
      iDecreasing = false;
      if (iTargetPlateTemp < PLATE_PID_INC_LOW_THRESHOLD)
        iPlatePid.SetTunings(PLATE_PID_INC_LOW_P, PLATE_PID_INC_LOW_I, PLATE_PID_INC_LOW_D);
      else
        iPlatePid.SetTunings(PLATE_PID_INC_NORM_P, PLATE_PID_INC_NORM_I, PLATE_PID_INC_NORM_D);

    } 
    else {
      iDecreasing = true;
      if (iTargetPlateTemp > PLATE_PID_DEC_HIGH_THRESHOLD)
        iPlatePid.SetTunings(PLATE_PID_DEC_HIGH_P, PLATE_PID_DEC_HIGH_I, PLATE_PID_DEC_HIGH_D);
      else if (iTargetPlateTemp < PLATE_PID_DEC_LOW_THRESHOLD)
        iPlatePid.SetTunings(PLATE_PID_DEC_LOW_P, PLATE_PID_DEC_LOW_I, PLATE_PID_DEC_LOW_D);
      else
        iPlatePid.SetTunings(PLATE_PID_DEC_NORM_P, PLATE_PID_DEC_NORM_I, PLATE_PID_DEC_NORM_D);
    }
  }
}

void Thermocycler::CalcPlateTarget() {
  if (ipCurrentStep == NULL)
    return;

  if (InControlledRamp()) {
    //controlled ramp
    double tempDelta = ipCurrentStep->GetTemp() - ipPreviousStep->GetTemp();
    double rampPoint = (double)GetRampElapsedTimeMs() / (ipCurrentStep->GetRampDurationS() * 1000);
    iTargetPlateTemp = ipPreviousStep->GetTemp() + tempDelta * rampPoint;

  } 
  else {
    //fast ramp
    iTargetPlateTemp = ipCurrentStep->GetTemp();
  }
}

void Thermocycler::ControlPeltier() {
  Thermocycler::ThermalDirection newDirection = Thermocycler::ThermalDirection::OFF;
  if (iProgramState == ERunning || (iProgramState == EComplete && ipCurrentStep != NULL)) {
    // Check whether we are nearing target and should switch to PID control
    // float plateTemp = GetPlateTemp();
    /* Test: Use estimated sample temp instead or measured well temp */
    float plateTemp = GetTemp();
    // Serial.print(iTargetPlateTemp);Serial.print("<=>");Serial.println(plateTemp);
    if (iPlateControlMode == EBangBang && absf(iTargetPlateTemp - plateTemp) < PLATE_BANGBANG_THRESHOLD) {
      iPlateControlMode = EPIDPlate;
      iPlatePid.SetMode(AUTOMATIC);
      iPlatePid.ResetI();
    }

    // Apply control mode
    if (iPlateControlMode == EBangBang) {
      // Full drive
      iPeltierPwm = iTargetPlateTemp > plateTemp ? MAX_PELTIER_PWM : MIN_PELTIER_PWM;
    }
    iPlatePid.Compute();

    if (iDecreasing && iTargetPlateTemp > PLATE_PID_DEC_LOW_THRESHOLD) {
      if (iTargetPlateTemp < plateTemp) {
        iPlatePid.ResetI();
      }
      else {
        iDecreasing = false;
      }
    } 

    if (iPeltierPwm > 0)
      newDirection = HEAT;
    else if (iPeltierPwm < 0)
      newDirection = COOL; 
    else
      newDirection = OFF;
  } 
  else {
    iPeltierPwm = 0;
  }
  iThermalDirection = newDirection;
  SetPeltier(newDirection, abs(iPeltierPwm));
}
void Thermocycler::ControlLid() {
  int drive = 0;  
  if (iProgramState == ERunning || iProgramState == ELidWait) {
    float temp = GetLidTemp();
    drive = iLidPid.Compute(iTargetLidTemp, temp);
  }
#ifdef USE_ESP8266
// Use on-off control instead of PWM because ESP8266 does not have enough pins
#ifdef PIN_LID_PWM_ACTIVE_LOW
  //analogWrite(PIN_LID_PWM, 1023-drive);
  int v = (int)(!(drive>(MAX_PELTIER_PWM/2)));
  digitalWrite(PIN_LID_PWM, v);
#else
  int v = (int)(drive>(MAX_PELTIER_PWM/2));
  digitalWrite(PIN_LID_PWM, v);
  //analogWrite(PIN_LID_PWM, drive);
#endif /* PIN_LID_PWM_ACTIVE_LOW */

#else

#ifdef PIN_LID_PWM_ACTIVE_LOW
  analogWrite(PIN_LID_PWM, 1023-drive);
#else
  // Active high
  analogWrite(PIN_LID_PWM, drive);
#endif /* PIN_LID_PWM_ACTIVE_LOW */

#endif

  analogValueLid = drive;
}

//PreprocessProgram initializes ETA parameters and validates/modifies ramp conditions
void Thermocycler::PreprocessProgram() {
  Step* pCurrentStep;
  Step* pPreviousStep = NULL;

  iProgramHoldDurationS = 0;
  iEstimatedTimeRemainingS = 0;
  iHasCooled = false;

  iProgramControlledRampDurationS = 0;
  iProgramFastRampDegrees = 0;
  iElapsedFastRampDegrees = 0;
  iTotalElapsedFastRampDurationMs = 0;  

  ipProgram->BeginIteration();
  while ((pCurrentStep = ipProgram->GetNextStep()) && !pCurrentStep->IsFinal()) {
    //validate ramp
    if (pPreviousStep != NULL && pCurrentStep->GetRampDurationS() * 1000 < absf(pCurrentStep->GetTemp() - pPreviousStep->GetTemp()) * PLATE_FAST_RAMP_THRESHOLD_MS) {
      //cannot ramp that fast, ignored set ramp
      pCurrentStep->SetRampDurationS(0);
    }

    //update eta hold
    iProgramHoldDurationS += pCurrentStep->GetStepDurationS();

    //update eta ramp
    if (pCurrentStep->GetRampDurationS() > 0) {
      //controlled ramp
      iProgramControlledRampDurationS += pCurrentStep->GetRampDurationS();
    } 
    else {
      //fast ramp
      double previousTemp = pPreviousStep ? pPreviousStep->GetTemp() : GetPlateTemp();
      iProgramFastRampDegrees += absf(previousTemp - pCurrentStep->GetTemp()) - CYCLE_START_TOLERANCE;
    }

    pPreviousStep = pCurrentStep;
  }
}

void Thermocycler::UpdateEta() {
  if (iProgramState == ERunning) {
    double fastSecondPerDegree;
    if (iElapsedFastRampDegrees == 0 || !iHasCooled)
      fastSecondPerDegree = 1.0;
    else
      fastSecondPerDegree = iTotalElapsedFastRampDurationMs / 1000 / iElapsedFastRampDegrees;

    unsigned long estimatedDurationS = iProgramHoldDurationS + iProgramControlledRampDurationS + iProgramFastRampDegrees * fastSecondPerDegree;
    unsigned long elapsedTimeS = GetElapsedTimeS();
    iEstimatedTimeRemainingS = estimatedDurationS > elapsedTimeS ? estimatedDurationS - elapsedTimeS : 0;
  }
}
#ifdef SUPPRESS_PELTIER_SWITCHING
/*
 * Suppress frequent peltier's direction switching to
 * reduce relay noise and save Peltier device
 */
// OFF, HEAT, COOL
static Thermocycler::ThermalDirection prevDirection = Thermocycler::ThermalDirection::OFF; // Logical value by PID
static int prevPWMDuty = 0; // Logical value by PID
static Thermocycler::ThermalDirection prevActualDirection = Thermocycler::ThermalDirection::OFF; // Actual status of hardware
static int prevActualPWMDuty = 0; // Actual status of hardware

#define PWM_SWITCHING_THRESHOLD 10
void Thermocycler::SetPeltier(ThermalDirection dir, int pwm /* Absolute value of peltier */) {
    Thermocycler::ThermalDirection dirActual;
    int pwmActual;
  if (dir != OFF && prevActualDirection != OFF && dir != prevActualDirection && prevActualPWMDuty!=0) {
      // Direction will be changed.
      if (prevPWMDuty==0 && pwm > PWM_SWITCHING_THRESHOLD) {
          pwmActual = pwm;
          dirActual = dir;
      } else {
          // Once set zero without switching relay
          pwmActual = 0;
          dirActual = prevActualDirection;
      }

  } else {
      // No need of switching direction.
      dirActual = dir;
      pwmActual = pwm;
  }
  if (dirActual == COOL) {
    digitalWrite(PIN_WELL_INA, PIN_WELL_VALUE_OFF);
    digitalWrite(PIN_WELL_INB, PIN_WELL_VALUE_ON);
#ifdef USE_FAN
  digitalWrite(PIN_FAN, PIN_FAN_VALUE_ON);
#endif
  }
  else if (dirActual == HEAT) {
    digitalWrite(PIN_WELL_INA, PIN_WELL_VALUE_ON);
    digitalWrite(PIN_WELL_INB, PIN_WELL_VALUE_OFF);
#ifdef USE_FAN
  digitalWrite(PIN_FAN, PIN_FAN_VALUE_ON);
#endif
  }
  else {
      // Off
    digitalWrite(PIN_WELL_INA, PIN_WELL_VALUE_OFF);
    digitalWrite(PIN_WELL_INB, PIN_WELL_VALUE_OFF);
#ifdef USE_FAN
  digitalWrite(PIN_FAN, PIN_FAN_VALUE_ON);
#endif
  }
     
#ifdef PIN_WELL_PWM_ACTIVE_LOW
  analogWrite(PIN_WELL_PWM, MAX_PELTIER_PWM-pwmActual);
#else
  analogWrite(PIN_WELL_PWM, pwmActual);
#endif /* PIN_WELL_PWM_ACTIVE_LOW */
  analogValuePeltier = (dir==COOL)?-pwmActual:pwmActual;

  prevDirection = dir;
  prevPWMDuty = pwm;
  prevActualDirection = dirActual;
  prevActualPWMDuty = pwmActual;
}
#else
void Thermocycler::SetPeltier(Thermocycler::ThermalDirection dir, int pwm) {
  if (dir == COOL) {
    digitalWrite(PIN_WELL_INA, HIGH);
    digitalWrite(PIN_WELL_INB, LOW);
  } 
  else if (dir == HEAT) {
    digitalWrite(PIN_WELL_INA, LOW);
    digitalWrite(PIN_WELL_INB, HIGH);
  } 
  else {
    digitalWrite(PIN_WELL_INA, LOW);
    digitalWrite(PIN_WELL_INB, LOW);
  }

#ifdef PIN_WELL_PWM_ACTIVE_LOW
  analogWrite(PIN_WELL_PWM, MAX_PELTIER_PWM-5pwm);
#else
  analogWrite(PIN_WELL_PWM, pwm);
#endif /* PIN_WELL_PWM_ACTIVE_LOW */
  analogValuePeltier = (dir==COOL)?-pwm:pwm;
}
#endif /* SUPPRESS_PELTIER_SWITCHING */

void Thermocycler::ProcessCommand(SCommand& command) {
  if (command.command == SCommand::EStart) {
    //find display cycle
    Cycle* pProgram = command.pProgram;
    Cycle* pDisplayCycle = pProgram;
    int largestCycleCount = 0;

    for (int i = 0; i < pProgram->GetNumComponents(); i++) {
      ProgramComponent* pComp = pProgram->GetComponent(i);
      if (pComp->GetType() == ProgramComponent::ECycle) {
        Cycle* pCycle = (Cycle*)pComp;
        if (pCycle->GetNumCycles() > largestCycleCount) {
          largestCycleCount = pCycle->GetNumCycles();
          pDisplayCycle = pCycle;
        }
      }
    }
    GetThermocycler().SetProgram(pProgram, pDisplayCycle, command.name, command.lidTemp);
    GetThermocycler().Start();

  } 
  else if (command.command == SCommand::EStop) {
    GetThermocycler().Stop(); //redundant as we already stopped during parsing

  } 
  else if (command.command == SCommand::EConfig) {
    //update displayed
    ipDisplay->SetContrast(command.contrast);

    //update stored contrast
    ProgramStore::StoreContrast(command.contrast);
  }
}

