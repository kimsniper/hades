/*
 * Copyright (c) 2025, Mezael Docoy
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

 
// Set digital output pin numbers:

const int DO_Generator_Crank_P9_K1 = 9;
const int DO_Generator_Start_P8_K2 = 8;
const int DO_OilLamp_Indicator_P7_K3 = 7;
const int DO_ArdSup_HoldContact_P6_K4 = 6;
const int DO_GeneratorLine1_Contact_P5_K5 = 5;
const int DO_GeneratorLine2_Contact_P4_K6 = 4;
//const int DO_MainsLine1_Contact_P3_K7 = 3;
//const int DO_MainsLine2_Contact_P2_K8 = 2;

// Set digital input pin numbers:

int DI_Oil_LowLevelContact = A3;

// Set analog input pin numbers:

int AI_Generator_VoltageLevelSensor_PA0 = A0;
int AI_Mains_VoltageLevelSensor_PA1 = A1;

// Set variables

int Var_Mains_ONStatus = 0;
int Var_Generator_ONStatus = 0;
int Var_Generator_StopTimer = 0;
int Var_Generator_Store10BITValue = 0;
float Var_Generator_VoltageValueMultiplier = .251572;
float Var_Generator_VoltageValue = 0;
int Var_Mains_Store10BITValue = 0;
float Var_Mains_VoltageValueMultiplier = .280112;
float Var_Mains_VoltageValue = 0;
int Var_Oil_LowLevelStatus = 0;
int Var_Generator_CrankTestTime = 1; // up to 2 times only for crank
int Var_Generator_CrankTimer = 0;
int Var_Generator_CrankTimerSP = 5; // 5 seconds per crank
int Var_Generator_CrankCounter = 0;
int Var_Generator_NoLoadToStopTime = 60;
int Var_Generator_NoStartAlarm = 0;
int Var_Generator_CrankStatus = 0;
int Var_Generator_ContactRelayTrigger_timer = 0;
int Var_Generator_ContactRelayTrigger_timerSP = 180; // secnds
int Var_Generator_ContactRelayTrigger_Status = 0; // 0 - untriggered, 1 - triggered for the first time
int Var_Generator_MaxVoltage = 240;
int Var_Generator_MinVoltage = 215;
int Var_Generator_MaxWorking_Status = 0;


void setup() 
{ 
  delay(10000);
  
  // Initialize digital outputs
  pinMode(DO_Generator_Crank_P9_K1, OUTPUT);
  pinMode(DO_Generator_Start_P8_K2, OUTPUT);
  pinMode(DO_GeneratorLine1_Contact_P5_K5, OUTPUT);
  pinMode(DO_GeneratorLine2_Contact_P4_K6, OUTPUT);
  pinMode(DO_ArdSup_HoldContact_P6_K4, OUTPUT);
  pinMode(DO_OilLamp_Indicator_P7_K3, OUTPUT);
  
  // Initialize digital inputs
  
  pinMode(DI_Oil_LowLevelContact, INPUT);

  // Initialize analog inputs
  
  pinMode(AI_Generator_VoltageLevelSensor_PA0, INPUT);
  pinMode(AI_Mains_VoltageLevelSensor_PA1, INPUT);

  digitalWrite(DO_Generator_Crank_P9_K1, HIGH); //Set Crank to Low
  digitalWrite(DO_OilLamp_Indicator_P7_K3, HIGH); //Set low oil lamp indicator to Low
  // Set gen set main lines connection to LOW
  digitalWrite(DO_GeneratorLine1_Contact_P5_K5, HIGH);
  digitalWrite(DO_GeneratorLine2_Contact_P4_K6, HIGH); 
  //Set arduino power hold contact relay to active   
  digitalWrite(DO_ArdSup_HoldContact_P6_K4, LOW);
  //Start genset 
  digitalWrite(DO_Generator_Start_P8_K2, LOW);
  //Set serial comm baud rate
  //Serial.begin(9600);
  
  
  delay(30000); // delay just incase Mains power will return
}

void loop() 
{
      // Check system
      Check_SystemParameters();

      // GenSet Crank
      if (Var_Mains_VoltageValue <= 197 && Var_Generator_VoltageValue <= 197 && Var_Generator_NoStartAlarm == 0 && Var_Oil_LowLevelStatus == LOW && Var_Generator_CrankStatus == 0) 
      {    
        Generator_Crank(); // crank gen set
      } 
      else
      {
        digitalWrite(DO_Generator_Crank_P9_K1, HIGH);
      }

      // Shutdown GenSet if no voltage is detected after cranking
      if (Var_Generator_VoltageValue <= 197 && Var_Generator_CrankStatus == 1) 
      {    
        Generator_Stop(); 
      } 

      // Trigger relays for GenSet Input voltage to household
      if (Var_Generator_VoltageValue >= Var_Generator_MinVoltage && Var_Generator_VoltageValue <= Var_Generator_MaxVoltage && Var_Oil_LowLevelStatus == LOW) 
      {   
        if (Var_Generator_ContactRelayTrigger_Status == 0)
        {
          Generator_Line_Relay_FirstStart();           
        }
        else
        {
          Generator_Line_Relay_Start();
        }              
      }      
      else
      {
        if (Var_Generator_ContactRelayTrigger_Status != 0)
        {        
          Var_Generator_ContactRelayTrigger_Status = 2;
        }
        // De-energized genset line relays
        digitalWrite(DO_GeneratorLine1_Contact_P5_K5, HIGH);
        digitalWrite(DO_GeneratorLine2_Contact_P4_K6, HIGH);
        Var_Generator_ContactRelayTrigger_timer = 0;
      }
      
      // Shutdodwn generator when mains power returns
      if (Var_Mains_VoltageValue >= 197) 
      {    
        // De-energized genset relays
        Generator_Stop();           
      }
      else
      {
        Var_Generator_StopTimer = 0; // generator no load to stop timer will reset if mains supply will fail before genset shutdown
      }

      // Shutdodwn generator when oil alarm is detected
      if(Var_Oil_LowLevelStatus == HIGH)
      {    
        digitalWrite(DO_OilLamp_Indicator_P7_K3, LOW); 
        digitalWrite(DO_GeneratorLine1_Contact_P5_K5, HIGH);
        digitalWrite(DO_GeneratorLine2_Contact_P4_K6, HIGH);
        delay(10000);
        digitalWrite(DO_Generator_Start_P8_K2, HIGH);
      }
}

void Check_SystemParameters()
{
  Var_Oil_LowLevelStatus = digitalRead(DI_Oil_LowLevelContact); // Checking oil level status
           
  // Read generator voltage level and print values to serial
      
  Var_Generator_Store10BITValue = analogRead(AI_Generator_VoltageLevelSensor_PA0);
  Var_Generator_VoltageValue = Var_Generator_Store10BITValue * Var_Generator_VoltageValueMultiplier;
  //Serial.print("Var_Generator_Store10BITValue:"); 
  //Serial.print(Var_Generator_Store10BITValue); 
  //Serial.print("Var_Generator_VoltageValue:"); 
  //Serial.print(Var_Generator_VoltageValue); 
    
  // Read mains voltage level and print values to serial
      
  Var_Mains_Store10BITValue = analogRead(AI_Mains_VoltageLevelSensor_PA1);
  Var_Mains_VoltageValue = Var_Mains_Store10BITValue * Var_Mains_VoltageValueMultiplier;
  //Serial.print("Var_Mains_Store10BITValue:"); 
  //Serial.print(Var_Mains_Store10BITValue); 
  //Serial.print("Var_Mains_VoltageValue:"); 
  //Serial.print(Var_Mains_VoltageValue); 
}

void Generator_Crank()
{ 
  digitalWrite(DO_Generator_Crank_P9_K1, LOW); // trigger this relay for generator crank contact
  
  Var_Generator_CrankTimer += 1;
  
  delay(1000);
  if (Var_Generator_CrankTimer == Var_Generator_CrankTimerSP)
  {
    digitalWrite(DO_Generator_Crank_P9_K1, HIGH); // De-energized relay to stop crank

    Var_Generator_CrankCounter += 1;
    
    delay(20000);
    
    Var_Generator_CrankTimer = 0;
    
    if (Var_Generator_CrankCounter == Var_Generator_CrankTestTime)
    {
      Generator_NoStartAlarm();
    } 
  }
}

void Generator_NoStartAlarm()
{
  Var_Generator_NoStartAlarm = 1;
}

void Generator_Stop()
{  
  Var_Generator_StopTimer += 1;
  
  delay(1000);
  
  if (Var_Generator_StopTimer == Var_Generator_NoLoadToStopTime)
  {
    digitalWrite(DO_Generator_Start_P8_K2, HIGH); // De - energized relay for generator stop contact
    ArdSup_Off();
  } 
}

void Generator_Line_Relay_FirstStart()
{  
  // Trigger genset relays after set time to stabilize output  
  Var_Generator_ContactRelayTrigger_timer += 1;
  delay(1000);
  if(Var_Generator_ContactRelayTrigger_timer == Var_Generator_ContactRelayTrigger_timerSP)
  {
    digitalWrite(DO_GeneratorLine1_Contact_P5_K5, LOW);
    digitalWrite(DO_GeneratorLine2_Contact_P4_K6, LOW);
    Var_Generator_CrankStatus = 1;   
    Var_Generator_ContactRelayTrigger_timer = 0;
    Var_Generator_ONStatus = 1;
    Var_Generator_ContactRelayTrigger_Status = 1;
    Var_Generator_ContactRelayTrigger_timerSP = 30; //change contact timer setpoint for runnning status
  } 
}

void Generator_Line_Relay_Start()
{
  if(Var_Generator_ContactRelayTrigger_Status == 2)
  {
    // Trigger genset relays after set time to stabilize output  
    Var_Generator_ContactRelayTrigger_timer += 1;
    delay(1000);
    if(Var_Generator_ContactRelayTrigger_timer == Var_Generator_ContactRelayTrigger_timerSP)
    {
      digitalWrite(DO_GeneratorLine1_Contact_P5_K5, LOW);
      digitalWrite(DO_GeneratorLine2_Contact_P4_K6, LOW);
      Var_Generator_ContactRelayTrigger_Status = 1;
      Var_Generator_ContactRelayTrigger_timer = 0;
    }
  }
  
}

//Shutdown arduino self supply

void ArdSup_Off()
{
  delay(2000);
  digitalWrite(DO_ArdSup_HoldContact_P6_K4, HIGH);
}
