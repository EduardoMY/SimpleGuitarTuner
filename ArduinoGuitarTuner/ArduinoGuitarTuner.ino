#include <Servo.h>
 
// 
Servo servo;

//pines
short pinServo=37;
short FREQREADER=4;
short PBON=52, PBOFF=50, PBSTRING=53, PBSTART=51; // Declaracion de los readers de los botones PBON, PBOFF, PBSTRING, PBSTART, FREQ
short LEDDONE=42, LEDON=43; // Declaracion de los outputs de los leds que vamos a encender LEDDONE, LEDON
short LEDS[6]={48, 44, 46, 49, 47, 45};//E, A, D, G, B,  E
unsigned long timer, upperTime;

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
  pinMode(PBON, INPUT_PULLUP);
  pinMode(PBOFF, INPUT_PULLUP);
  pinMode(PBSTRING, INPUT_PULLUP);
  pinMode(PBSTART, INPUT_PULLUP);
  
    //Declaracion de los LEds
  pinMode(LEDS[0], OUTPUT);
  pinMode(LEDS[1], OUTPUT);
  pinMode(LEDS[2], OUTPUT);
  pinMode(LEDS[3], OUTPUT);
  pinMode(LEDS[4], OUTPUT);
  pinMode(LEDS[5], OUTPUT);
  
  pinMode(LEDON, OUTPUT);
  digitalWrite(LEDON, HIGH);
  
  isTunning=false;
  isBtnUp=true;//el medina no sabe como programar;
  currentNote=0;
  timer=0;
  upperTime=10;
}

void resetAll(){
  isTunning=false;
  digitalWrite(LEDS[currentNote-1], LOW);
  currentNote=0;
  digitalWrite(LEDON, LOW);
} 

void loop(void) {
  //
  if(!isTunning){
    if(digitalRead(PBSTRING)==LOW){
      if(isBtnUp && timer==0){
        turnOnLed();
        timer=1; 
        Serial.print("Numero");
        Serial.println(timer);
      }
      isBtnUp=false;
    }
     else if(digitalRead(PBOFF)==LOW){
      if(isBtnUp && timer==0){
        resetAll();
        timer=1;
      }
      isBtnUp=false;
     }
     else if(digitalRead(PBSTART)==LOW){
      if(isBtnUp && timer==0){
        isTunning=true;
        timer=1;
      }
      isBtnUp=false;
     }
     else
      isBtnUp=true;
  }
  else {
    duration = pulseIn(FREQREADER, HIGH);
    if(duration!=0)
      FREQ= 500000/duration;
    else FREQ=0;
    //checkFreq();
    //moveServo();
    Serial.print("Frecuencia.  ");
    Serial.println(FREQ); 
  }
  if(timer!=0)
     timer++;
  if(timer==upperTime)
    timer=0;
  Serial.println(timer);
}
/*
void checkFreq(void){
  for(int c=0; c<6; c++)
    if(noteFrequencies[c]-df <= FREQ && noteFrequencies[c]+df >= FREQ)
      Serial.print(c);
} */

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
