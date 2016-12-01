#include <Servo.h>
 
// 
Servo servo;
//pines
short pinServo=25;
short FREQREADER=4;
short PBON=26, PBOFF=27, PBSTRING=53, PBSTART=29; // Declaracion de los readers de los botones PBON, PBOFF, PBSTRING, PBSTART, FREQ
short LEDDONE=30, LEDON=31; // Declaracion de los outputs de los leds que vamos a encender LEDDONE, LEDON
short LEDS[6]={32, 33, 34, 35, 36, 37};//E, A, D, G, B,  E
//short anterior,actual;

// constantes, diferenciales, etc;
double FREQ;
unsigned long duration;
float noteFrequencies[6]={82.4, 110, 146.8, 196, 246.9, 329.6}; //unidad en Hz
short servoMovement=8; // que tanto se movera el servo
float df=10; //margen de las frecuencias

//Variables de estado
short currentNote; //E=1, A=2,..E=6
boolean isTunning; 
boolean isBtnUp;

void setup() {
  Serial.begin(9600);
  
  servo.attach(pinServo); //Declaracion del Servo
  pinMode(FREQREADER, INPUT);
  
  //Declaracion de los Botones
  pinMode(PBON, INPUT);
  pinMode(PBOFF, INPUT);
  pinMode(PBSTRING, INPUT_PULLUP);
  pinMode(PBSTART, INPUT);
  
    //Declaracion de los LEds
  pinMode(LEDS[0], OUTPUT);
  pinMode(LEDS[1], OUTPUT);
  pinMode(LEDS[2], OUTPUT);
  pinMode(LEDS[3], OUTPUT);
  pinMode(LEDS[4], OUTPUT);
  pinMode(LEDS[5], OUTPUT);
  
  pinMode(LEDON, OUTPUT);
  isTunning=false;
  isBtnUp=true;//el medina no sabe como programar;
  currentNote=0;
  digitalWrite(LEDON, HIGH);
}
void resetAll(void){
  isTunning=false;
  digitalWrite(LEDS[currentNote-1], LOW);
  currentNote=0;
  digitalWrite(LEDON, LOW);
}
void loop() {
  //
  if(!isTunning){
    if(digitalRead(PBSTRING)==LOW){
      if(isBtnUp)
        turnOnLed();
      isBtnUp=false;
    }
    else
      isBtnUp=true;
     /*else if(digitalRead(PBOFF)==LOW)
      resetAll();
     else if(digitalRead(PBSTART)==LOW)
     isTunning=true;*/
  }
  else {
    duration = pulseIn(FREQREADER, HIGH);
    if(duration!=0)
      FREQ= 500000/duration;
    else FREQ=0;
    checkFreq();
    //moveServo();
    Serial.print("Frecuencia.  ");
    Serial.println(FREQ); 
  }
}
void checkFreq(void){
  for(int c=0; c<6; c++)
    if(noteFrequencies[c]-df <= FREQ && noteFrequencies[c]+df >= FREQ){
    Serial.print(c);
       digitalWrite(LEDS[c], HIGH);
  }
}
void turnOnLed(void){
  Serial.print("Nota actual: ");
  Serial.println(currentNote);
  if(currentNote!=0)
    digitalWrite(LEDS[currentNote-1], LOW);
   currentNote++;
   if(currentNote==7)
    currentNote=1;
   digitalWrite(LEDS[currentNote-1], HIGH);
}
void stringDone(void){
  digitalWrite(LEDDONE, HIGH);
  delay(1000);
  digitalWrite(LEDDONE, LOW);
  isTunning=false;
}
void moveServo(void){
    Serial.println("Servo se va a mover");
    if(noteFrequencies[currentNote-1]-df < FREQ-df){ //Mover clockWise
      servo.write(90+servoMovement);// sets the servo position according to the scaled value
      delay(15); // waits for the servo to get there
    }
    else if(noteFrequencies[currentNote-1] > FREQ+df) {
      servo.write(90-servoMovement);// sets the servo position according to the scaled value
      delay(15); // waits for the servo to get there
    }
    else
      stringDone();
  }
