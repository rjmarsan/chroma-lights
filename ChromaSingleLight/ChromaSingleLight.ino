#include <EEPROMBackupVar.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>

#include <SerialCommand.h>

#define clockpin 13 // CI
#define enablepin 10 // EI
#define latchpin 9 // LI
#define datapin 11 // DI
 
#define NumLEDs 3
 
int LEDChannels[NumLEDs][3] = {0};
int SB_CommandMode;
int SB_RedCommand;
int SB_GreenCommand;
int SB_BlueCommand;

SerialCommand sCmd;
 
void setup() {
 
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);
   SPCR = (1<<SPE)|(1<<MSTR)|(0<<SPR1)|(0<<SPR0);
   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
 
   Serial.begin(115200);
   for (int i = 1; i < NumLEDs; i++)
   {
     setLED(i, 1023, 1023, 1023);
   }
   WriteLEDArray();
  
   setupSerialCommands();
   setupStorage();
   setupAnimations();
}

/** LIGHTS API **/
void SB_SendPacket() {
 
    if (SB_CommandMode == B01) {
     SB_RedCommand = 120;
     SB_GreenCommand = 100;
     SB_BlueCommand = 100;
    }
 
    SPDR = SB_CommandMode << 6 | SB_BlueCommand>>4;
    while(!(SPSR & (1<<SPIF)));
    SPDR = SB_BlueCommand<<4 | SB_RedCommand>>6;
    while(!(SPSR & (1<<SPIF)));
    SPDR = SB_RedCommand << 2 | SB_GreenCommand>>8;
    while(!(SPSR & (1<<SPIF)));
    SPDR = SB_GreenCommand;
    while(!(SPSR & (1<<SPIF)));
 
}
 
void WriteLEDArray() {
 
    SB_CommandMode = B00; // Write to PWM control registers
    for (int h = 0;h<NumLEDs;h++) {
	  SB_RedCommand = LEDChannels[h][0];
	  SB_GreenCommand = LEDChannels[h][1];
	  SB_BlueCommand = LEDChannels[h][2];
	  SB_SendPacket();
    }
 
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
    SB_CommandMode = B01; // Write to current control registers
    for (int z = 0; z < NumLEDs; z++) SB_SendPacket();
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
}

void setLED(byte LED, int red, int green, int blue)
{
  LEDChannels[LED][0] = red;
  LEDChannels[LED][1] = green;
  LEDChannels[LED][2] = blue;

}
/** LIGHTS API DONE **/










/** MAIN CODE **/
boolean SAVE_FROM_PREVIOUS = true;

EEPROMVar<boolean> lightison(true);
EEPROMVar<int> current_animation(0);
void(*animations[2][3])() = {
  {anim1_setup,anim1_update,anim1_getinfo},
  {anim2_setup,anim2_update,anim2_getinfo}
};
int num_animations = sizeof(animations)/sizeof(animations[0]);

void setupSerialCommands() {
  Serial.begin(57600);
  sCmd.addCommand("pickanim",pickAnimation);
  sCmd.addCommand("turnon",turnOn);
  sCmd.addCommand("turnoff",turnOff);
  sCmd.addCommand("getstate",getState);
  sCmd.addCommand("getinfo",getinfo);
  sCmd.addCommand("getallinfo",getAllInfo);
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  Serial.println("Ready");
}

void setupStorage() {
  EEPROM.setMemPool(120, EEPROMSizeUno); 
  EEPROM.setMaxAllowedWrites(20);
  if (SAVE_FROM_PREVIOUS) {
    lightison.restore();
    current_animation.restore();
  } else {
    lightison.save();
    current_animation.save();
  }
  //one last error check
  if (current_animation < 0 || current_animation > num_animations)  {
    current_animation = 0;
    current_animation.save();
  }
}

void setupAnimations() {
  for (int i=0; i<num_animations; i++) {
    animations[i][0]();
  }
}





/** Main app commands **/
void unrecognized(const char *command) {
  Serial.println("What?");
}

void pickAnimation() {
  int aNumber;
  char *arg;

  //Serial.println("We're in pickAnimation");
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    //Serial.print("First argument was: ");
    //Serial.println(aNumber);
    if (aNumber >= 0 && aNumber < num_animations) {
      current_animation = aNumber;
      current_animation.save();
    } else {
    }
  } else {
  }
  Serial.println(current_animation);

}

void turnOn() {
  lightison = true;
  lightison.save();
  Serial.println("on");
}

void turnOff() {
  lightison = false;
  lightison.save();
  Serial.println("off");
}

void getState() {
  Serial.print("{ 'on':");
  Serial.print(lightison);
  Serial.print(", 'animation':");
  Serial.print(current_animation);
  Serial.print(", 'num_animations':");
  Serial.print(num_animations);
  Serial.print("}\n");
}

void getinfo() {
  char *arg = sCmd.next();
  if (arg != NULL) {
    int aNumber = atoi(arg);    // Converts a char string to an integer
    if (aNumber >= 0 && aNumber < num_animations) {
      animations[aNumber][2]();
    }
  } 
}

void getAllInfo() {
  for (int i=0; i<num_animations; i++) {
    animations[i][2]();
  }
}
/***********/





void loop() {
  sCmd.readSerial();
  if (lightison) {
    animations[current_animation][1]();
  } else {
    off();
  }
  delay(16);
}





/*******************/




/** Library stuff **/

void copy_ccolor(int* target, int* dest) {
  dest[0] = target[0];
  dest[1] = target[1];
  dest[2] = target[2];
}

/*******************/










/** Animation 0 - off **/
void off() {  
  for (int i = 1; i < NumLEDs; i++) {
    setLED(i, 0, 0, 0);
  }
  WriteLEDArray();  
}


/*******************/


/** Animation 1 **/

float anim1_progress = 0;
float anim1_max_progress = 3.1415;

#define ANIM1_MAXLIGHTS 6
int anim1_colors[][3] = {
  { 1023, 0, 0},
  { 0, 1023, 0},
  { 0, 0, 1023},
  { 1023, 0, 1023},
  { 0, 1023, 1023},
  { 1023, 1023, 0},
};

int anim1_previous_color[3];
int anim1_target_color[3];
int anim1_prev_index = 0;

EEPROMVar<float> anim1_speed = 0.002;

void anim1_setup() {
  copy_ccolor(anim1_colors[0], anim1_previous_color);
  copy_ccolor(anim1_colors[1], anim1_target_color);
  sCmd.addCommand("anim1_setspeed",anim1_setspeed);
  if (SAVE_FROM_PREVIOUS) {
    anim1_speed.restore();
  } else {
    anim1_speed.save();
  }
}

void anim1_getinfo() {
  Serial.print("{ 'name':'Random Fading Lights', 'commands':['anim1_setspeed'] }\n");
}


/** anim1_setspeed INT from 0 to 10000  **/
void anim1_setspeed() {
  char *arg;
  int newspeed;

  Serial.println("We're in Anim1 setSpeed");
  arg = sCmd.next();
  if (arg != NULL) {
    newspeed = atoi(arg);
    Serial.println(newspeed);
    anim1_speed = newspeed/10000.0;
    anim1_speed.save();
  } 

}



void anim1_update() {
  float progress = 1-(1+cos(anim1_progress))*0.5;
  int r = (int)(anim1_previous_color[0]*(1-progress) + anim1_target_color[0]*(progress));
  int g = (int)(anim1_previous_color[1]*(1-progress) + anim1_target_color[1]*(progress));
  int b = (int)(anim1_previous_color[2]*(1-progress) + anim1_target_color[2]*(progress));
  
  for (int i = 1; i < NumLEDs; i++) {
    setLED(i, r, g, b);
  }
  WriteLEDArray();  
  anim1_progress += anim1_speed;
  if (anim1_progress > anim1_max_progress) {
    anim1_progress = 0;
    int newindex = random(ANIM1_MAXLIGHTS);
    while (newindex == anim1_prev_index) {
      newindex = random(ANIM1_MAXLIGHTS);
    }
    anim1_prev_index = newindex;
    copy_ccolor(anim1_target_color, anim1_previous_color);
    copy_ccolor(anim1_colors[newindex], anim1_target_color);
  }
}

/*******************/

/** Animation 2 - Solid **/


EEPROMVar<int> anim2_r(1023);
EEPROMVar<int> anim2_g(1023);
EEPROMVar<int> anim2_b(1023);

void anim2_setup() {
  sCmd.addCommand("anim2_pickcolor",anim2_pickcolor);
  if (SAVE_FROM_PREVIOUS) {
    anim2_r.restore();
    anim2_g.restore();
    anim2_b.restore();
  } else {
    anim2_r.save();
    anim2_g.save();
    anim2_b.save();
  }
}

void anim2_getinfo() {
  Serial.print("{ 'name':'Solid Light', 'commands':['anim2_pickcolor'] }\n");
}

/** anim2_pickcolor INT INT INT from 0 to 1023  **/
void anim2_pickcolor() {
  char *arg;

  Serial.println("We're in Anim2 pickColor");
  arg = sCmd.next();
  if (arg != NULL) {
    anim2_r = atoi(arg);
    anim2_r.save();
  } 
  arg = sCmd.next();
  if (arg != NULL) {
    anim2_g = atoi(arg);
    anim2_g.save();
  } 
  arg = sCmd.next();
  if (arg != NULL) {
    anim2_b = atoi(arg);
    anim2_b.save();
  } 

}


void anim2_update() {  
  for (int i = 1; i < NumLEDs; i++) {
    setLED(i, anim2_r, anim2_g, anim2_b);
  }
  WriteLEDArray();  
}

/*******************/

