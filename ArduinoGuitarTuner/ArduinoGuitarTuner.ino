#include <Servo.h>
//#include <AudioFrequencyMeter.h>
 
// 
Servo servo;

//pines
short pinServo=40;
short PBON=51, PBOFF=50, PBSTRING=52, PBSTART=53; // Declaracion de los readers de los botones PBON, PBOFF, PBSTRING, PBSTART, FREQ
short LEDDONE=23, LEDON=22; // Declaracion de los outputs de los leds que vamos a encender LEDDONE, LEDON
short LEDS[6]={35, 34, 33, 32, 31, 30};//E, A, D, G, B,  E
unsigned long timerPB, upperTime;

// constantes, diferenciales, etc;
float noteFrequencies[6]={82.4, 110, 146.8, 196, 246.9, 329.6}; //unidad en Hz
short servoMovement=10; // que tanto se movera el servo
float df=3; //margen de las frecuencias

/*
 * Empiezan variables para identificar la frecuencia
 * CAT
 */
 
//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//storage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

//variables for deciding whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

//End CAT


//Variables de estado
short currentNote; //E=1, A=2,..E=6
boolean isTunning; 
boolean isBtnUp;
boolean isSystemOn;

void setup() {
  Serial.begin(9600);
  //servo.attach(pinServo); //Declaracion del Servo
  
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

  isTunning=false;
  isSystemOn=false;
  isBtnUp=true;//el medina sabe como programar;
  currentNote=0;
  timerPB=0;
  upperTime=100;
  frequency=0;

  cli();//disable interrupts
  
  //set up continuous sampling of analog pin 0 at 38.5kHz
 
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready
                  
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
  
  time++;//increment timer at rate of 38.5kHz
  
  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp){
    maxAmp = abs(127-ADCH);
  }
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }
}

void reset(){//clean out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}

void resetAll(){
  isTunning=false;
  digitalWrite(LEDS[currentNote-1], LOW);
  currentNote=0;
  isSystemOn=false;
  digitalWrite(LEDON, LOW);
  Serial.println("Se debio de apagar");
} 

void loop(){
  //
  if(!isTunning){
    if(digitalRead(PBSTRING)==LOW && isSystemOn){
      if(isBtnUp && timerPB==0){
        turnOnLed();
        timerPB=1; 
      }
      isBtnUp=false;
    }
     else if(digitalRead(PBOFF)==LOW && isSystemOn){
      if(isBtnUp && timerPB==0){
        resetAll();
        timerPB=1;
      }
      isBtnUp=false;
     }
     else if(digitalRead(PBSTART)==LOW && isSystemOn && currentNote!=0){
      if(isBtnUp && timerPB==0){
        isTunning=true;
        timerPB=1;
        Serial.print("Nota Actual: ");
        Serial.println(currentNote);
      }
      isBtnUp=false;
     }
    else if(digitalRead(PBON)==LOW){
      if(isBtnUp && timerPB==0){
        digitalWrite(LEDON, HIGH);
        isSystemOn=true;
        timerPB=1;
        Serial.print("Hola Elsa: ");
        Serial.println(timerPB);
      }
      isBtnUp=false;
     }
     else 
      isBtnUp=true;
  }
  else if(isSystemOn){ 

    if (checkMaxAmp>ampThreshold){
      if(period==0)
        frequency=0.0;
      else
        frequency = 38462/float(period);//calculate frequency timer rate/period
      Serial.print("Frecuencia: ");
      Serial.println(frequency);
      if(frequency!=0)
        moveServo();
      else
        stopServo();
    }
    else 
      Serial.println("Ni leeyo");
    if(digitalRead(PBOFF)==LOW){
      if(timerPB==0){
        resetAll();
        timerPB=1;
      }
    }
    
  }
  
  if(timerPB!=0)
     timerPB++;
  if(timerPB==upperTime)
    timerPB=0;
  Serial.println(timerPB);
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
  servo.attach(pinServo);
    Serial.println("Servo se va a mover");
    
    if(noteFrequencies[currentNote-1]-df > frequency) //Mover clockWise  
      servo.write(90+servoMovement);// sets the servo position according to the scaled value
    
    else if(noteFrequencies[currentNote-1]+df < frequency) 
      servo.write(90-servoMovement);// sets the servo position according to the scaled value
    
    else
      stringDone();
      
}
void stopServo(void){
  servo.detach();
}
