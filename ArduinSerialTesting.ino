/*
  Arduino Serial Testing
  Use to test inputs and outputs of an Arduino UNO or NANO
  All digital pins are assigned OUTPUT, not including pins 0,1 that are used for the serial communication.
  All analog pins are assigned INPUT_PULLUP.
  Available pins for UNO and NANO are: digital 2-13, analog 14-19, also possible to write A0-A5.
  Available pins for MEGA are: digital 2-53, analog 54-69, also possible to write A0-A15.
  How to use:
  1. Open the serial monitor on 9600.
  1.1 Set the serial monitor to Newline.
  2. Insert the number of the pin you want to check or action number from the following options:
    2.1 - 99 - Print the pins state and modes
    2.2 - 98 - Test all the pins
    2.3 - 97 - Activte pins range
  3. Choose an action from the following options:
    3.0. 0 - Change pin output to state - LOW
    3.1. 1 - Change pin output to state - HIGH
    3.2. 2 - Change pin mode to INPUT
    3.3. 3 - Change pin mode to INPUT_PULLUP
    3.4. 4 - Change pin mode to OUTPUT
    3.5. 5 - Display digital read of the pin (0/1)
    3.6. 6 - Display analog read of the pin (0-1023)
    3.7. 7 - Write PWM value to pin (0-255)
    3.8 - 8 - Blink That Pin in 1 second interval for 30 seconds



    You can type -1 at any level to get back to the start.

    Things to add:
    implement the TestAll function
    Activte Pins Range function

    Things to check:
    blinking the led
    EEPROM - memory for the entire pin selection and state

*/
#include <EEPROM.h>

// Arduino types and variables
const int UNO = 1;
const int NANO = 2;
const int MEGA = 3;

const int MaxNumberOfPins = 70;
const int MaxNumberOfPWMPins = 16;
const int EndArraySign = 99;
int NumberOfPins;
int NumberOfPWMPins;
int FirstDigitalPin;
int LastDigitalPin;
int FirstAnalogPin;
int LastAnalogPin;
int DigitalPWMPins[MaxNumberOfPWMPins] = {};
bool ArduinoDefined = false;
bool RestoredArduino = false;

// Pin modes constants
byte PinsStateArray[MaxNumberOfPins] = {};
byte PinsModeArray[MaxNumberOfPins] = {};
byte BlinkingPinsArray[MaxNumberOfPins] = {};
const byte RX = 0;
const byte TX = 1;
const byte Input = 2;
const byte InputPullUp = 3;
const byte Output = 4;

// Inputs variables:
int InputPinNumer = 0;
int InputActionNumber = 0;
int InputPWMNumber = 0;

// Stages variables:
int SerialInput;
int ActionStage = 0;
int FirstStage = 1;
int SecondStage = 2;
int ThirdStage = 3;
bool BlinkingPins = false;
bool BlinkState = LOW;
int BlinkingIndex = 0;

// Timers
float CurrentMillis = 0;
float PrevMillis = 0;
int BlinkTime = 1000;
float BlinkPrevMillis = 0;
float BlinkingPinsResetTimer = 0;
const int BlinkingPinsResetTime = 30000;
float PrevEEPROMMillis = 0;
const int EEPREOMUpdateTime = 3000; // Update once every 3 seconds

// Action Options
const int BackToStart = -1;
const int PinToLow = 0;
const int PinToHigh = 1;
const int PinToInput = 2;
const int PinToInputPullUp = 3;
const int PinToOutput = 4;
const int DigitalReadAction = 5;
const int AnalogReadAction = 6;
const int PWMToPin = 7;
const int BlinkThePin = 8;
const int ActivtePinsRangeCode = 97;
const int TestAllCode = 98;
const int PrintPins = 99;

// Variables used for reading the serial input
bool PrintInfo = true;
bool newData = false;
const byte numChars = 8;
char receivedChars[numChars]; // an array to store the received data
bool AnalogPin = false;

// EEPROM variables and constants
const int RestorationCode = 99;
const int ArduinoTypeEEPROMAddr = 0;
const int PinsStateArrayEEPROMAddr = 1; // This is an array and require more than one memory address
const int PinsModeArrayEEPROMAddr = 1 + PinsStateArrayEEPROMAddr + MaxNumberOfPins; // This is an array and require more than one memory address
const int BlinkingPinsArrayEEPROMAddr = 1 + PinsModeArrayEEPROMAddr + MaxNumberOfPins; // This is an array and require more than one memory address
bool SomethingChanged = false; // To check if something was changed

// Functions:
// This function runs once at the setup phase to define which arduino you are using.
// You can change this with restart.
void DefineArduino() {
  if (!ArduinoDefined) {
    Serial.println("Select an Arduino from the list.");
    Serial.print("UNO: ");
    Serial.println(UNO);
    Serial.print("NANO: ");
    Serial.println(NANO);
    Serial.print("MEGA: ");
    Serial.println(MEGA);
    Serial.print("To restore the last session, type: ");
    Serial.println(RestorationCode);
    // Wait for input from the user.
    while (!Serial.available());
    int ArduinoType = SerialRead();
    if (ArduinoType == RestorationCode) {
      ArduinoType = EEPROM.read(ArduinoTypeEEPROMAddr);
      RestoredArduino = true;
      Serial.println("Restoring last session.");
    }
    if (ArduinoType == UNO || ArduinoType == NANO) {
      // Uno and nano pins:
      NumberOfPins = 20;
      NumberOfPWMPins = 6;
      FirstDigitalPin = 2;
      LastDigitalPin = 13;
      FirstAnalogPin = 14;
      LastAnalogPin = 19;
      int UNODigitalPWMPins[] = {3, 5, 6, 9, 10, 11};
      // Set the PWM array for the correct pins
      for (int i = 0; i < NumberOfPWMPins; i++) {
        DigitalPWMPins[i] = UNODigitalPWMPins[i];
      }
      // Fill out the rest of the array with EndMarks
      for (int i = NumberOfPWMPins; i < MaxNumberOfPWMPins; i++) {
        DigitalPWMPins[i] = EndArraySign;
      }
      ArduinoDefined = true;
      Serial.println("Arduino defined as UNO/NANO.");
      EEPROM.update(ArduinoTypeEEPROMAddr, ArduinoType);
    }
    else if (ArduinoType == MEGA) {
      // Mega pins: PWM: 2 to 13 and 44 to 46. Digital 2-53. Analog 54-69.
      NumberOfPins = 70;
      NumberOfPWMPins = 15;
      FirstDigitalPin = 2;
      LastDigitalPin = 53;
      FirstAnalogPin = 54;
      LastAnalogPin = 69;
      int MEGADigitalPWMPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 44, 45, 46};
      // Set the PWM array for the correct pins
      for (int i = 0; i < NumberOfPWMPins; i++) {
        DigitalPWMPins[i] = MEGADigitalPWMPins[i];
      }
      // Fill out the rest of the array with EndMarks
      for (int i = NumberOfPWMPins; i < MaxNumberOfPWMPins; i++) {
        DigitalPWMPins[i] = EndArraySign;
      }
      ArduinoDefined = true;
      Serial.println("Arduino defined as MEGA.");
      EEPROM.update(ArduinoTypeEEPROMAddr, ArduinoType);
    }
    else {
      Serial.println("Please try again.");
      // Rerun the function.
      newData = false;
      DefineArduino();
    }
  }
  newData = false;
}

// This function writes the pins states and modes arrays from the EEPROM to the main arrays
void RecallFromEEPROM() {
  for (int i = 0; i < NumberOfPins; i++) {
    PinsModeArray[i] = EEPROM.read(PinsModeArrayEEPROMAddr + i);
    PinToMode(i, PinsModeArray[i]);
    PinsStateArray[i] = EEPROM.read(PinsStateArrayEEPROMAddr + i);
    PinToState(i, PinsStateArray[i]);
    BlinkingPinsArray[i] = EEPROM.read(BlinkingPinsArrayEEPROMAddr + i);

  }
}

// This function updates the arrays: state, mode and blinking pins from the main arrays to the EEPROM
void UpdateTheEEPROM() {
  for (int i = 0; i < NumberOfPins; i++) {
    EEPROM.update(PinsStateArrayEEPROMAddr + i, PinsStateArray[i]);
    EEPROM.update(PinsModeArrayEEPROMAddr + i, PinsModeArray[i]);
    EEPROM.update(BlinkingPinsArrayEEPROMAddr + i, BlinkingPinsArray[i]);
  }
  //Serial.println("EEPROM updated.");
}
// Function to read from the serial by demand
int SerialRead() {
  static byte ndx = 0;
  char rc;
  int Input = 0;
  char endMarker = '\n';
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    //    Serial.print("rc = ");
    //    Serial.println(rc);
    if (rc != endMarker) {
      if (rc == 'A' || rc == 'a') {
        //Serial.println("AnalogPin = HIGH");
        AnalogPin = true;
      }
      else {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
    }
    else {
      receivedChars[ndx] = '\n'; // terminate the string
      ndx = 0;
      newData = true;
      Input = atoi(receivedChars);
      if (AnalogPin) {
        //Serial.println("Analog Pin value added.");
        Input += FirstAnalogPin;
        AnalogPin = false;
      }
    }
  }
  //  Serial.print("Input = ");
  //  Serial.println(Input);
  return Input; // Return the pin number to the main function
}

// Change pin output to state HIGH/LOW and update the state array
void PinToState(int pin, bool State) {
  digitalWrite(pin, State);
  PinsStateArray[InputPinNumer] = State; // Update the pin state array to the correct state
  Serial.print("Pin number: ");
  Serial.print(pin);
  Serial.print("  state is: ");
  Serial.println(State);
  SomethingChanged = true; // Mark that something was changed and need to be updated in the EEPROM
}

// Change pin assigned role to a different mode.
// If the number is not correct the pin is redefined as OUTPUT.
void PinToMode(int pin, int Mode) {
  PinsStateArray[pin] = LOW;
  Serial.print("Pin: ");
  Serial.print(pin);
  Serial.print("  changed to: ");
  if (Mode == Input) {
    pinMode(pin, INPUT);
    PinsModeArray[pin] = Input;
    Serial.println("INPUT.");
    DigitalReadPin(pin);
  }
  else if (Mode == InputPullUp) {
    pinMode(pin, INPUT_PULLUP);
    PinsModeArray[pin] = InputPullUp;
    Serial.println("INPUT_PULLUP.");
    DigitalReadPin(pin);
  }
  else {
    pinMode(pin, OUTPUT);
    PinsModeArray[pin] = Output;
    Serial.println("OUTPUT.");
    PinToState(pin, PinsStateArray[pin]);
  }
  SomethingChanged = true; // Mark that something was changed and need to be updated in the EEPROM
}

// Make a pin blink with 1 sec intervals
void BlinkPins() {
  int i = 0;
  BlinkState = !BlinkState;
  while (BlinkingPinsArray[i] < EndArraySign) {
    digitalWrite(BlinkingPinsArray[i], BlinkState);
    i++;
  }
}

// Function to update the blinking pins array
void UpdateBlinkingArray(int Pin) {
  int i = 0;
  // Function to check if the pin is already in the array or not
  bool PinInArray = false;
  for (i = 0; i < sizeof(BlinkingPinsArray); i++) {
    if (BlinkingPinsArray[i] == Pin) {
      PinInArray = true;
      i = sizeof(BlinkingPinsArray);
    }
  }
  // If the pin isn't in the array, add the pin to the array
  if (!PinInArray) {
    BlinkingPinsArray[BlinkingIndex] = Pin; // Update the array to include the pin number
    BlinkingIndex++; // Advance the index by 1 for the next time.
    BlinkingPins = true; // Make sure the blinking function is ON
  }
  else { // If the pin is in the array already, remove it and turn it off
    digitalWrite(Pin, LOW);
    BlinkingIndex--;
    if (BlinkingIndex < 0) { // This should not happen but included for safety
      BlinkingIndex = 0;
    }
    i = 0;
    // Find the pin location in the blinking pins array
    while (BlinkingPinsArray[i] != Pin) {
      i++;
    }
    int TempPinData = BlinkingPinsArray[BlinkingIndex]; // Store the data from the last cell
    BlinkingPinsArray[i] = TempPinData; // Replace with the contant of the last cell
    BlinkingPinsArray[BlinkingIndex] = EndArraySign; // Remark the end over that area
  }
  if (BlinkingIndex == 0) { // If it was the last cell turn off the blinking function
    BlinkingPins = false;
  }
  SomethingChanged = true; // Mark that something was changed and need to be updated in the EEPROM
}

// This function prints the options after the pin selected.
void PrintPinRequest() {
  Serial.println("Insert a pin number.");
  Serial.print("Available digital pins are: ");
  Serial.print(FirstDigitalPin);
  Serial.print(" - ");
  Serial.println(LastDigitalPin);
  Serial.print("For analog pins use A0 - A");
  Serial.println(LastAnalogPin - FirstAnalogPin);
  //  Serial.print("Or pin codes by numbers starting from A0, are: ");
  //  Serial.print(FirstAnalogPin);
  //  Serial.print(" - ");
  //  Serial.println(LastAnalogPin);
  Serial.println("Select 99 to print the pins state and modes");
  //  Serial.print("Select ");
  //  Serial.print(TestAllCode);
  //  Serial.println(" to test the circuit - outputs to high then low and read.");
  PrintInfo = false;
}

// This function checks if the selected pin mode is input
bool CheckIfInput(int pin) {
  if (PinsModeArray[pin] == Input || PinsModeArray[pin] == InputPullUp) return true;
  else return false;
}

// This function checks if the selected pin is an analog pin
bool CheckAnalog(int pin) {
  if (pin >= FirstAnalogPin && pin <= LastAnalogPin) {
    return true;
  }
  else return false;
}

// This function checks if the selected pin is a PWM pin
bool CheckPWM(int pin) {
  for (int i = 0; i < NumberOfPWMPins; i++) {
    if (pin == DigitalPWMPins[i]) {
      return true;
    }
  }
  return false;
}

// This function displays the digital read of the pin
void DigitalReadPin(int pin) {
  Serial.print("Digital pin: ");
  Serial.print(pin);
  Serial.print("  reads: ");
  Serial.println(digitalRead(pin));
}

// This function displays the analog read of the pin
void AnalogReadPin(int pin) {
  Serial.print("Analog pin: A");
  Serial.print(pin - FirstAnalogPin);
  Serial.print("  reads: ");
  Serial.println(analogRead(pin));
}

// This function displays the options according to the selected pin
void PrintOptions(int pin) {
  Serial.println("Choose an action from the following:");
  if (!CheckIfInput(pin)) {
    Serial.println("0 - Pin output to - LOW");
    Serial.println("1 - Pin output to - HIGH");
  }
  Serial.println("2 - Pin mode to INPUT");
  Serial.println("3 - Pin mode to INPUT_PULLUP");
  Serial.println("4 - Pin mode to OUTPUT");
  if (CheckIfInput(pin)) {
    Serial.println("5 - Digital read.");
  }
  if (CheckAnalog(pin) && CheckIfInput(pin)) {
    Serial.println("6 - Analog read.");
  }
  if (CheckPWM(pin) && !CheckIfInput(pin)) {
    Serial.println("7 - Write PWM value (0-255).");
  }
  if (!CheckIfInput(pin)) {
    Serial.println("8 - Blink That Pin for 30 seconds.");
  }
  PrintInfo = false;
}

// This function activate the pins to the correct state
void ActivtePinsRange() {
  //  Serial.println("Choose first pin.");
  //  Serial.println("Choose last pin.");
}

// This function prints out the pins and the modes assigned to them, Example: Pin: 2 Mode: output
void PrintPinState() {
  Serial.println("Pins numbers and assigned mode:");
  for (int i = 0; i <= NumberOfPins; i++) {
    Serial.print("Pin: ");
    if (CheckAnalog(i)) {
      Serial.print("A");
      Serial.print(i - FirstAnalogPin);
    }
    else Serial.print(i);
    Serial.print("  Mode: ");
    switch (PinsModeArray[i]) {
      case RX:
        Serial.print("RX");
        break;
      case TX:
        Serial.print("TX");
        break;
      case Input:
        Serial.print("Input");
        break;
      case InputPullUp:
        Serial.print("InputPullUp");
        break;
      case Output:
        Serial.print("Output");
        break;
      default:
        Serial.print("Unknown mode.");
        break;
    }
    if (!CheckIfInput(i)) {
      Serial.print("  State: ");
      Serial.print(PinsStateArray[i]);
    }
    Serial.println();
  }
}

// Run a test on all pins, turning all of them to LOW for 2 seconds then to HIGH.
// Waiting for 2 seconds and then turning all of them to INPUT_PULLUP.
// Then displaying the input and restoring all to OUTPUT.
void TestAll() {
  //Serial.println("Starting test.");
  for (int i = FirstDigitalPin; i <= LastAnalogPin; i++) {
    pinMode(i, OUTPUT);
    PinsModeArray[i] = Output;
    PinsStateArray[i] = LOW;
    digitalWrite(i, PinsStateArray[i]);
    delay(10);
  }

  delay(1000);
  for (int i = FirstDigitalPin; i <= LastAnalogPin; i++) {
    PinsStateArray[i] = HIGH;
    digitalWrite(i, PinsStateArray[i]);
    delay(10);
  }
  //Serial.println("All digital pins are HIGH.");
  delay(3000);
  for (int i = FirstDigitalPin; i <= LastDigitalPin; i++) {
    PinsStateArray[i] = LOW;
    digitalWrite(i, PinsStateArray[i]);
    pinMode(i, INPUT_PULLUP);
    PinsModeArray[i] = InputPullUp;
    DigitalReadPin(i);
  }
  for (int i = FirstAnalogPin; i <= LastAnalogPin; i++) {
    PinsStateArray[i] = LOW;
    digitalWrite(i, PinsStateArray[i]);
    pinMode(i, INPUT_PULLUP);
    AnalogReadPin(i);
  }
}

void setup() {
  // initialize serial:
  Serial.begin(9600);
  while (!Serial);
  // Define an Arduino at the start of the run
  DefineArduino();
  if (!RestoredArduino) { // If the Arduino is restored these definitions have already happend
    // Declare digital pins 0 and 1 as RX/TX
    PinsModeArray[0] = RX; // RX pin
    PinsModeArray[1] = TX; // TX pin
    int i;
    // Declare all digital pins as OUTPUT.
    for (i = FirstDigitalPin; i <= LastDigitalPin; i++) {
      PinToMode(i, Output);
    }
    // Declare all analog pins as INPUT_PULLUP and show analog reading.
    for (i = FirstAnalogPin; i <= LastAnalogPin; i++) {
      PinToMode(i, InputPullUp);
    }
    for (i = 0; i < MaxNumberOfPins; i++) {
      BlinkingPinsArray[i] = EndArraySign;
    }
  }
  else { // In case the arduino is restored from EEPROM
    RecallFromEEPROM();
  }
  ActionStage = FirstStage;
  Serial.println("System Ready.");
}

void loop() {
  CurrentMillis = millis();
  // Update the EEPROM if needed
  if (SomethingChanged && CurrentMillis - PrevEEPROMMillis > EEPREOMUpdateTime) {
    UpdateTheEEPROM();
    SomethingChanged = false;  // Unmark that something was changed so the EEPROM will not be updated
    PrevEEPROMMillis = CurrentMillis;
  }
  // Blink the required pins
  if (BlinkingPins && CurrentMillis - BlinkPrevMillis > BlinkTime) {
    if (CurrentMillis - BlinkingPinsResetTimer > BlinkingPinsResetTime) {
      BlinkingPinsResetTimer = CurrentMillis;
      for (int i = 0; i < MaxNumberOfPins; i++) {
        BlinkingPinsArray[i] = EndArraySign;
        BlinkingIndex = 0;
        digitalWrite(i, LOW);
      }
    }
    BlinkPins();
  }
  // Print information according to the stage
  if (PrintInfo) {
    if (ActionStage == FirstStage) {
      PrintPinRequest();
    }
    else if (ActionStage == SecondStage) {
      PrintOptions(InputPinNumer);
      //      Serial.print("Input Pin Numer: ");
      //      Serial.println(InputPinNumer);
    }
    else if (ActionStage == ThirdStage) {
      Serial.println("Insert PWM value (0-255)");
      PrintInfo = false;
    }
    Serial.println("Or -1 to go back.");
  }
  if (Serial.available()) {
    SerialInput = SerialRead();
    if (SerialInput == BackToStart) {
      ActionStage = FirstStage;
      newData = false;
      PrintInfo = true;
    }
  }
  if (newData) {
    // First stage - select pin or print all pins inputs.
    if (ActionStage == FirstStage) {
      InputPinNumer = SerialInput;
      if (InputPinNumer >= FirstDigitalPin && InputPinNumer <= LastAnalogPin) {
        ActionStage = SecondStage;
        Serial.print("Selected pin number is: ");
        Serial.println(InputPinNumer);
      }
      else if (InputPinNumer == PrintPins) {
        PrintPinState();
        ActionStage = FirstStage;
      }
      else if (InputPinNumer == TestAllCode) {
        TestAll();
        ActionStage = FirstStage;
      }
      else if (ActivtePinsRangeCode == TestAllCode) {
        ActivtePinsRange();
        ActionStage = FirstStage;
      }
      else {
        Serial.println("Error, pin number is out of range.");
        ActionStage = FirstStage;
      }
    }
    // Second stage - after pin selected.
    // Select action for the pin or go back.
    else if (ActionStage == SecondStage) {
      InputActionNumber = SerialInput;
      //      Serial.print("Action selected: ");
      //      Serial.println(InputActionNumber);
      ActionStage = FirstStage;
      switch (InputActionNumber) {
        case PinToLow:
          if (!CheckIfInput(InputPinNumer)) {
            PinToState(InputPinNumer, LOW);
          }
          else Serial.println("Error, pin mode is INPUT.");
          break;
        case PinToHigh:
          if (!CheckIfInput(InputPinNumer)) {
            PinToState(InputPinNumer, HIGH);
          }
          else Serial.println("Error, pin mode is INPUT.");
          break;
        case PinToInput:
          PinToMode(InputPinNumer, Input);
          break;
        case PinToInputPullUp:
          PinToMode(InputPinNumer, InputPullUp);
          break;
        case PinToOutput:
          PinToMode(InputPinNumer, Output);
          break;
        case DigitalReadAction:
          if (CheckIfInput(InputPinNumer)) {
            DigitalReadPin(InputPinNumer);
          }
          else Serial.println("Error, pin mode is OUTPUT.");
          break;
        case AnalogReadAction:
          if (CheckAnalog(InputPinNumer)) {
            if (CheckIfInput(InputPinNumer)) {
              AnalogReadPin(InputPinNumer);
            }
            else Serial.println("Error, pin mode is OUTPUT.");
          }
          else Serial.println("Error, not an analog pin.");
          break;
        case PWMToPin:
          if (!CheckIfInput(InputPinNumer)) ActionStage = ThirdStage;
          else Serial.println("Error, pin mode is INPUT.");
          break;
        case BlinkThePin:
          BlinkingPinsResetTimer = CurrentMillis;
          UpdateBlinkingArray(InputPinNumer);
          break;
        default:
          Serial.println("Unknown command.");
          ActionStage = FirstStage;
          PrintInfo = true;
          break;
      }
    }
    else if (ActionStage == ThirdStage) {
      ActionStage = FirstStage;
      InputPWMNumber = SerialInput;
      if (InputPWMNumber >= 0 && InputPWMNumber <= 255) {
        analogWrite(InputPinNumer, InputPWMNumber);
        Serial.print("Pin number: ");
        Serial.print(InputPinNumer);
        Serial.print("  PWM value set to: ");
        Serial.println(InputPWMNumber);
        PinsStateArray[InputPinNumer] = InputPWMNumber;
      }
      else {
        Serial.println("Wrong value, try again.");
        ActionStage = ThirdStage;
      }
    }
    else {
      ActionStage = FirstStage;
    }
    newData = false;
    PrintInfo = true;
  }
}
