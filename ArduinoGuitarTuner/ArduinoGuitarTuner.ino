#include <Servo.h>
//#include <AudioFrequencyMeter.h>
 
// 
Servo servo;

//pines
short pinServo=37;
short PBON=52, PBOFF=50, PBSTRING=53, PBSTART=51; // Declaracion de los readers de los botones PBON, PBOFF, PBSTRING, PBSTART, FREQ
short LEDDONE=42, LEDON=43; // Declaracion de los outputs de los leds que vamos a encender LEDDONE, LEDON
short LEDS[6]={48, 44, 46, 49, 47, 45};//E, A, D, G, B,  E
unsigned long timerPB, upperTime;

// constantes, diferenciales, etc;
float noteFrequencies[6]={82.4, 110, 146.8, 196, 246.9, 329.6}; //unidad en Hz
short servoMovement=10; // que tanto se movera el servo
float df=10; //margen de las frecuencias

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

void setup() {
  Serial.begin(9600);
  servo.attach(pinServo); //Declaracion del Servo
  
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

  isTunning=true;
  isBtnUp=true;//el medina no sabe como programar;
  currentNote=0;
  timerPB=0;
  upperTime=10;
  frequency=0;
  //servo.write(90);
  //servo.writeMicroseconds(50);
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
  digitalWrite(LEDON, LOW);
} 

void loop(){
  //
  if(!isTunning){
    if(digitalRead(PBSTRING)==LOW){
      if(isBtnUp && timerPB==0){
        turnOnLed();
        timerPB=1; 
        //Serial.print("Numero");
        //Serial.println(timerPB);
      }
      isBtnUp=false;
    }
     else if(digitalRead(PBOFF)==LOW){
      if(isBtnUp && timerPB==0){
        resetAll();
        timerPB=1;
      }
      isBtnUp=false;
     }
     else if(digitalRead(PBSTART)==LOW){
      if(isBtnUp && timerPB==0){
        isTunning=true;
        timerPB=1;
      }
      isBtnUp=false;
     }
     else
      isBtnUp=true;
  }
  else { 
    if (checkMaxAmp>ampThreshold){
      frequency = 38462/float(period);//calculate frequen      servo.detach();cy timer rate/period
      Serial.print("Frecuencia: ");
      Serial.println(frequency);
      moveServo();
    }
    if(digitalRead(PBSTRING)==LOW)
          servo.detach();
  }
  if(timerPB!=0)
     timerPB++;
  if(timerPB==upperTime)
    timerPB=0;
  //Serial.println(timer);
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
    servo.write(90+servoMovement);
    /*
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
      */
}
