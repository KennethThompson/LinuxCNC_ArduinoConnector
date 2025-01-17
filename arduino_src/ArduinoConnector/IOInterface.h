#pragma once
#ifndef IOINTERFACE_H_
#define IOINTEFFACE_H_
#include "Connection.h"
void readCommands();
void comalive();
void flushSerial();
void readTmpInputs(ConnectionBase* client);

//Variables for Saving States
#ifdef INPUTS
  int InState[Inputs];
  int oldInState[Inputs];
  unsigned long lastInputDebounce[Inputs];
#endif
#ifdef SINPUTS
  int sInState[sInputs];
  int soldInState[sInputs];
  int togglesinputs[sInputs];
  unsigned long lastsInputDebounce[sInputs];
#endif
#ifdef OUTPUTS
  int OutState[Outputs];
  int oldOutState[Outputs];
#endif
#ifdef DALLAS_TEMP_SENSOR
  double inTmpSensorState[TmpSensors];
  //int oldInTempSensorState[TmpSensors];
#endif
#ifdef PWMOUTPUTS
  int OutPWMState[PwmOutputs];
  int oldOutPWMState[PwmOutputs];
#endif
#ifdef AINPUTS
  int oldAinput[AInputs];
#endif
#ifdef LPOTIS
  int Lpoti[LPotis];
  int oldLpoti[LPotis];
#endif
#ifdef BINSEL
  int oldAbsEncState;
#endif
#ifdef KEYPAD
  byte KeyState = 0;
#endif
#ifdef MULTIPLEXLEDS
  byte KeyLedStates[numVccPins*numGndPins];
#endif
#if QUADENCS == 1 
  const int QuadEncs = 1;  
#endif
#if QUADENCS == 2 
  const int QuadEncs = 2;  
#endif
#if QUADENCS == 3 
  const int QuadEncs = 3;  
#endif
#if QUADENCS == 4 
  const int QuadEncs = 4;  
#endif
#if QUADENCS == 5 
  const int QuadEncs = 5;  
#endif
#ifdef QUADENC
  long EncCount[QuadEncs];
  long OldEncCount[QuadEncs];
#endif


#ifdef JOYSTICK
long counter[JoySticks*2] = {0};      // Initialize an array for the counters
long prevCounter[JoySticks*2] = {0};  // Initialize an array for the previous counters
float incrementFactor[JoySticks*2] = {0.0}; // Initialize an array for the incrementFactors
unsigned long lastUpdateTime[JoySticks*2] = {0}; // Store the time of the last update for each potentiometer
  
#endif

//### global Variables setup###
//Please don't touch them
unsigned long oldmillis = 0;
unsigned long newcom = 0;
unsigned long lastcom = 0;
int connectionState = 0;

#define STATE_CMD 0
#define STATE_IO 1
#define STATE_VALUE 2


byte state = STATE_CMD;
char inputbuffer[5];
byte bufferIndex = 0;
char cmd = 0;
uint16_t io = 0;
uint16_t value = 0;

String statusMessage;
uint8_t statusMessageReady = 0;


#ifdef JOYSTICK

void readJoySticks(ConnectionBase * client) {
  for (int i = 0; i < JoySticks*2; i++) {
    unsigned long currentTime = millis(); // Get the current time

    // Check if it's time to update the counter for this potentiometer
    if (currentTime - lastUpdateTime[i] >= 100) { // Adjust 100 milliseconds based on your needs
      lastUpdateTime[i] = currentTime; // Update the last update time for this potentiometer

      int potValue = analogRead(JoyStickPins[i]); // Read the potentiometer value

      // Calculate the distance of the potentiometer value from the middle
      int distanceFromMiddle = potValue - middleValue;

      // Apply deadband to ignore small variations around middleValue
      if (abs(distanceFromMiddle) <= deadband) {
        incrementFactor[i] = 0.0; // Set incrementFactor to 0 within the deadband range
      } else {
        // Apply non-linear scaling to distanceFromMiddle to get the incrementFactor
        incrementFactor[i] = pow((distanceFromMiddle * scalingFactor), 3);
      }

      // Update the counter if the incrementFactor has reached a full number
      if (incrementFactor[i] >= 1.0 || incrementFactor[i] <= -1.0) {
        counter[i] += static_cast<long>(incrementFactor[i]); // Increment or decrement the counter by the integer part of incrementFactor
        incrementFactor[i] -= static_cast<long>(incrementFactor[i]); // Subtract the integer part from incrementFactor
      }

      // Check if the counter value has changed
      if (counter[i] != prevCounter[i]) {
        client->SendPinStatusMessage('R',JoyStickPins[i],counter[i]);
        // Update the previous counter value with the current counter value
        prevCounter[i] = counter[i];
      }
    }
  }
}
#endif

#ifdef QUADENC
void readEncoders(ClientBase* client){
    if(QuadEncs>=1){
      #if QUADENCS >= 1
        EncCount[0] = Encoder0.read()/QuadEncMp[0];
      #endif
    }
    if(QuadEncs>=2){
      #if QUADENCS >= 2
        EncCount[1] = Encoder1.read()/QuadEncMp[1];
      #endif
    }
    if(QuadEncs>=3){
      #if QUADENCS >= 3
        EncCount[2] = Encoder2.read()/QuadEncMp[2];
      #endif
    }
    if(QuadEncs>=4){
      #if QUADENCS >= 4
        EncCount[3] = Encoder3.read()/QuadEncMp[3];
      #endif
    }
    if(QuadEncs>=5){
      #if QUADENCS >= 5
        EncCount[4] = Encoder4.read()/QuadEncMp[4];
      #endif
    }

    for(int i=0; i<QuadEncs;i++){
      if(QuadEncSig[i]==2){
        if(OldEncCount[i] != EncCount[i]){
          client->SendPinStatusMessage('R',i,EncCount[i]);//send Counter
          OldEncCount[i] = EncCount[i];
        }
      }  
      if(QuadEncSig[i]==1){
        if(OldEncCount[i] < EncCount[i]){
        client->SendPinStatusMessage('R',i,1); //send Increase by 1 Signal
        OldEncCount[i] = EncCount[i];
        }
        if(OldEncCount[i] > EncCount[i]){
        client->SendPinStatusMessage('R',i,0); //send Increase by 1 Signal
        OldEncCount[i] = EncCount[i];
        }
      }
    }
}

#endif
/*
void comalive(){
  if(lastcom == 0){ //no connection yet. send E0:0 periodicly and wait for response
    while (lastcom == 0){
      readCommands();
      flushSerial();
      Serial.println("E0:0");
      delay(200);
      #ifdef STATUSLED
        StatLedErr(1000,1000);
      #endif
    }  
    connectionState = 1;
    flushSerial();
    #ifdef DEBUG
      Serial.println("first connect");
    #endif
  }
  if(millis() - lastcom > timeout){  
  #ifdef STATUSLED
     StatLedErr(500,200);
  #endif
      if(connectionState == 1){
        #ifdef DEBUG
          Serial.println("disconnected");
        #endif
        connectionState = 2;
      }
            
   }
   else{  
      connectionState=1;
      #ifdef STATUSLED
        if(DLEDSTATUSLED == 1){
          #ifdef DLED
            controlDLED(StatLedPin, 1);
          #endif
        }
        else{
          digitalWrite(StatLedPin, HIGH);
        }
      #endif 
    }
}
*/



void flushSerial(){
  while (Serial.available() > 0) {
  Serial.read();
  }
}

#ifdef STATUSLED
void StatLedErr(int offtime, int ontime){
  unsigned long newMillis = millis();
  
  if (newMillis - oldmillis >= offtime){
      #ifdef DLED
        if(DLEDSTATUSLED == 1){
          controlDLED(StatLedPin, 1);}
      #endif
      if(DLEDSTATUSLED == 0){digitalWrite(StatLedPin, HIGH);}
    } 
  if (newMillis - oldmillis >= offtime+ontime){{
      #ifdef DLED
        if(DLEDSTATUSLED == 1){
          controlDLED(StatLedPin, 0);}
      #endif
      if(DLEDSTATUSLED == 0){digitalWrite(StatLedPin, LOW);}
            
      oldmillis = newMillis;
      
    }
  }

}
#endif

#ifdef OUTPUTS
void writeOutputs(int Pin, int Stat){
  digitalWrite(Pin, Stat);
}
#endif

#ifdef PWMOUTPUTS
void writePwmOutputs(int Pin, int Stat){
  analogWrite(Pin, Stat);
}

#endif

#ifdef DLED
void initDLED(){
  strip.begin();
  strip.setBrightness(DLEDBrightness);
  
    for (int i = 0; i < DLEDcount; i++) {
    strip.setPixelColor(i, strip.Color(DledOffColors[i][0],DledOffColors[i][1],DledOffColors[i][2]));
    }
  strip.show();
  #ifdef DEBUG
    Serial.print("DLED initialised");
  #endif
}

void controlDLED(int Pin, int Stat){
  if(Stat == 1){

    strip.setPixelColor(Pin, strip.Color(DledOnColors[Pin][0],DledOnColors[Pin][1],DledOnColors[Pin][2]));
    #ifdef DEBUG
      Serial.print("DLED No.");
      Serial.print(Pin);
      Serial.print(" set to:");
      Serial.println(Stat);

    #endif
    } 
    else{

      strip.setPixelColor(Pin, strip.Color(DledOffColors[Pin][0],DledOffColors[Pin][1],DledOffColors[Pin][2]));
      #ifdef DEBUG
        Serial.print("DLED No.");
        Serial.print(Pin);
        Serial.print(" set to:");
        Serial.println(Stat);

      #endif   
    }
  strip.show();
  }
#endif

#ifdef LPOTIS
int readLPoti(ClientBase* client){
    for(int i= 0;i<LPotis; i++){
      int var = analogRead(LPotiPins[i][0])+margin;
      int pos = 1024/(LPotiPins[i][1]-1);
      var = var/pos;
      if(oldLpoti[i]!= var){
        oldLpoti[i] = var;
        client->SendPinStatusMessage('L', LPotiPins[i][0],oldLpoti[i]);
      }
    }
}
#endif


#ifdef AINPUTS
int readAInputs(ConnectionBase* client){
   
   for(int i= 0;i<AInputs; i++){
      unsigned long var = 0;
      for(int d= 0;d<smooth; d++){// take couple samples to denoise signal
        var = var+ analogRead(AInPinmap[i]);
      }
      var = var / smooth;
      if(oldAinput[i]!= var){
        oldAinput[i] = var;
        client->SendPinStatusMessage('A',AInPinmap[i],oldAinput[i]);
      }
    }
}
#endif
#ifdef INPUTS
void readInputs(ConnectionBase* client){
    for(int i= 0;i<Inputs; i++){
      auto State = digitalRead(InPinmap[i]);
      
      if(InState[i]!= State && millis()- lastInputDebounce[i] > debounceDelay){
        InState[i] = State;
        client->SendPinStatusMessage('I',InPinmap[i],InState[i]);
      
      lastInputDebounce[i] = millis();
      }
    }
}
#endif
#ifdef DALLAS_TEMP_SENSOR
void readTmpInputs(ConnectionBase* client){
    for(int i= 0;i<TmpSensors; i++){
      DallasTemperature * sensor = TmpSensorControlMap[i];
      if(sensor->isConversionComplete() == false)
      {
        continue;
      }
      double v = sensor->getTempCByIndex(0); // Future Todo: Add in support for multiple sensors per pin.
      // The sensor is returning a double. Future Todo: Enable an option that ouputs the double value if desired
      double v_f = (v * 9/5) + 32; // Perform conversion to Farenheit if output in F is toggled on below
      
      if(inTmpSensorState[i]!= v){
        inTmpSensorState[i] = v;
        
        #if TEMP_OUTPUT_C == 1
          client->SendPinStatusMessage('T',TmpSensorMap[i],inTmpSensorState[i], 2);
        #else
          client->SendPinStatusMessage('T',TmpSensorMap[i],v_f, 2);
        #endif
      }
      sensor->requestTemperatures(); 
    }
}
#endif
#ifdef SINPUTS
void readsInputs(ClientBase* client){
  for(int i= 0;i<sInputs; i++){
    sInState[i] = digitalRead(sInPinmap[i]);
    if (sInState[i] != soldInState[i] && millis()- lastsInputDebounce[i] > debounceDelay){
      // Button state has changed and debounce delay has passed
      
      if (sInState[i] == LOW || soldInState[i]== -1) { // Stuff after || is only there to send States at Startup
        // Button has been pressed
        togglesinputs[i] = !togglesinputs[i];  // Toggle the LED state
      
        if (togglesinputs[i]) {
          client->SendPinStatusMessage('I',sInPinmap[i],togglesinputs[i]);  // Turn the LED on
        } 
        else {
          client->SendPinStatusMessage('I',sInPinmap[i],togglesinputs[i]);   // Turn the LED off
        }
      }
      soldInState[i] = sInState[i];
      lastsInputDebounce[i] = millis();
    }
  }
}
#endif

#ifdef BINSEL
int readAbsKnob(){
  int var = 0;
  if(digitalRead(BinSelKnobPins[0])==1){
    var += 1;
  }
  if(digitalRead(BinSelKnobPins[1])==1){
    var += 2;
  }
  if(digitalRead(BinSelKnobPins[2])==1){
    var += 4;
  }  
  if(digitalRead(BinSelKnobPins[3])==1){
    var += 8;
  }  
  if(digitalRead(BinSelKnobPins[4])==1){
    var += 16;
  }
  if(var != oldAbsEncState){
    Serial.print("K0:");
    Serial.println(var);
    }
  oldAbsEncState = var;
  return (var);
}
#endif

#ifdef KEYPAD
void readKeypad(ConnectionBase* client){
  //detect if Button is Pressed
  for (int col = 0; col < numCols; col++) {
    pinMode(colPins[col], OUTPUT);
    digitalWrite(colPins[col], LOW);
    // Read the state of the row pins
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], INPUT_PULLUP);
      if (digitalRead(rowPins[row]) == LOW && lastKey != keys[row][col]) {
        // A button has been pressed
        client->SendPinStatusMessage('M',keys[row][col],1);
        lastKey = keys[row][col];
        row = numRows;

      }
      if (digitalRead(rowPins[row]) == HIGH && lastKey == keys[row][col]) {
        // The Last Button has been unpressed
        client->SendPinStatusMessage('M',keys[row][col],0);
        lastKey = -1; //reset Key pressed
        row = numRows;
      }
    }
    
    // Set the column pin back to input mode
    pinMode(colPins[col], INPUT);
  }

}
#endif

#ifdef MULTIPLEXLEDS
void multiplexLeds() {
  unsigned long currentMillis = millis();
  //init Multiplex
  #ifdef KEYPAD //if Keyboard is presend disable Pullup Resistors to not mess with LEDs while a Button is pressed.
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], OUTPUT);
      digitalWrite(rowPins[row], LOW);
    }
  #endif

  for (int i = 0; i < numVccPins; i++) {
    pinMode(LedVccPins[i], OUTPUT);
    digitalWrite(LedVccPins[i], LOW); // Set to LOW to disable all Vcc Pins
  }
  for (int i = 0; i < numGndPins; i++) {
    pinMode(LedGndPins[i], OUTPUT);
    digitalWrite(LedGndPins[i], HIGH); // Set to HIGH to disable all GND Pins
  }
  
  for(currentLED = 0; currentLED < numVccPins*numGndPins ;currentLED ++){
    if(ledStates[currentLED] == 1){                         //only handle turned on LEDs 
      digitalWrite(LedVccPins[currentLED/numVccPins],HIGH); //turn current LED on
      digitalWrite(LedGndPins[currentLED%numVccPins],LOW);
      
      Serial.print("VCC: ");
      Serial.print(LedVccPins[currentLED/numVccPins]);
      Serial.print(" GND: ");
      Serial.println(LedGndPins[currentLED%numVccPins]);
      
      delayMicroseconds(interval);                          //wait couple ms
      digitalWrite(LedVccPins[currentLED/numVccPins],LOW);  //turn off and go to next one
      digitalWrite(LedGndPins[currentLED%numVccPins],HIGH);
    }
  }
/*
  }
  if(ledStates[currentLED]==0){//If currentLED is Off, manage next one. 
    currentLED++;
  }
  if(currentLED >= numVccPins*numGndPins){
      currentLED= 0;
  } 
  */
}
#endif

void commandReceived(char cmd, uint16_t io, uint16_t value){
  #ifdef OUTPUTS
  if(cmd == 'O'){
    writeOutputs(io,value);
    lastcom=millis();

  }
  #endif
  #ifdef PWMOUTPUTS
  if(cmd == 'P'){
    writePwmOutputs(io,value);
    lastcom=millis();

  }
  #endif
  #ifdef DLED
  if(cmd == 'D'){
    controlDLED(io,value);
    lastcom=millis();
    #ifdef debug
      Serial.print("DLED:");
      Serial.print(io);
      Serial.print(" State:");
      Serial.println(DLEDstate[io]);
    #endif

  }
  #endif
  #ifdef MULTIPLEXLEDS
    if(cmd == 'M'){
      ledStates[io] = value; // Set the LED state
      lastcom=millis();
      #ifdef DEBUG
        Serial.print("multiplexed Led No:");
        Serial.print(io);
        Serial.print("Set to:");
        Serial.println(ledStates[io]);
      #endif

  }
  #endif

  /*
  if(cmd == 'E'){
    lastcom=millis();
    if(connectionState == 2){
     reconnect(); 
    }
  }
  */


  #ifdef DEBUG
    Serial.print("ARDUINO DEBUG: IO Processor Received= ");
    Serial.print(cmd);
    Serial.print(io);
    Serial.print(":");
    Serial.println(value);
  #endif
}

// Keeping original command processing logic for now.  Sending
// commands to/from the arduino in the original string format is beneficial 
// as this avoids the problem of sending binary MsgPack structures which choke on null/0x00 values over the Serial Port.
// TODO: Consider refactoring command processor
void pushCommand(int b){
  byte current = b;
    switch(state)
    {
      case STATE_CMD:
      {
        cmd = current;
        state = STATE_IO;
        bufferIndex = 0;
        break;
      }
      case STATE_IO:
      {
        if(isDigit(current)){
            inputbuffer[bufferIndex++] = current;
        }else if(current == ':'){
            inputbuffer[bufferIndex] = 0;
            io = atoi(inputbuffer);
            state = STATE_VALUE;
            bufferIndex = 0;
        }
        else
        {
          #ifdef DEBUG
          Serial.print("ARDUINO DEBUG: Invalid character: ");
          Serial.println(current);
          #endif
        }
        break;
      }
      case STATE_VALUE:
      {
        if(isDigit(current)){
            inputbuffer[bufferIndex++] = current;
        }
        else if(current == '\n'){
            inputbuffer[bufferIndex] = 0;
            value = atoi(inputbuffer);
            commandReceived(cmd, io, value);
            state = STATE_CMD;
        }
        else{
          #ifdef DEBUG
          Serial.print("ARDUINO DEBUG: Invalid character: ");
          Serial.println(current);
          #endif
        }
        break;
      }
  }
}

/*
void readCommands(){
    #ifdef SERIAL_TO_LINUXCNC
      byte current;
      while(Serial.available() > 0){
          current = Serial.read();
          switch(state){
              case STATE_CMD:
                    cmd = current;
                    state = STATE_IO;
                    bufferIndex = 0;
                  break;
              case STATE_IO:
                  if(isDigit(current)){
                      inputbuffer[bufferIndex++] = current;
                  }else if(current == ':'){
                      inputbuffer[bufferIndex] = 0;
                      io = atoi(inputbuffer);
                      state = STATE_VALUE;
                      bufferIndex = 0;
                  }
                  else{
                      #ifdef DEBUG
                      Serial.print("Invalid character: ");
                      Serial.println(current);
                      #endif
                  }
                  break;
              case STATE_VALUE:
                  if(isDigit(current)){
                      inputbuffer[bufferIndex++] = current;
                  }
                  else if(current == '\n'){
                      inputbuffer[bufferIndex] = 0;
                      value = atoi(inputbuffer);
                      commandReceived(cmd, io, value);
                      state = STATE_CMD;
                  }
                  else{
                    #ifdef DEBUG
                    Serial.print("Invalid character: ");
                    Serial.println(current);
                    #endif
                  
                  }
                  break;
          }

      }
      #endif
}
*/

#endif // #define IOINTERFACE_H_
