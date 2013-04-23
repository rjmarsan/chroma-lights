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
   
   
   anim1_setup();
}
 
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






void loop() {
  
  anim1_update();
  delay(16);
}



/** Library stuff **/

void copy_ccolor(int* target, int* dest) {
  dest[0] = target[0];
  dest[1] = target[1];
  dest[2] = target[2];
}








float anim1_progress = 0;
float anim1_max_progress = 1;

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

float anim1_speed = 0.002;

void anim1_setup() {
  copy_ccolor(anim1_colors[0], anim1_previous_color);
  copy_ccolor(anim1_colors[1], anim1_target_color);
}



void anim1_update() {
  int r = (int)(anim1_previous_color[0]*(1-anim1_progress) + anim1_target_color[0]*(anim1_progress));
  int g = (int)(anim1_previous_color[1]*(1-anim1_progress) + anim1_target_color[1]*(anim1_progress));
  int b = (int)(anim1_previous_color[2]*(1-anim1_progress) + anim1_target_color[2]*(anim1_progress));
  
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



