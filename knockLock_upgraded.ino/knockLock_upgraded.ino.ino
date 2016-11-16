#include <Arduino.h>
#include <Servo.h>
#include <EEPROM.h>

int addr = 0;
Servo myServo;

const int piezo = A0;
const int programSwitch = 2;
const int switchPin = 3;
const int greenLed = 4;
const int redLed = 5;

int knockVal;
int switchVal;

boolean done = false; 

const int quietKnock = 10;
const int loudKnock = 100;

boolean locked = true;
int numberOfKnocks = 0;

const int minpiezovalue = 40;        
const int rejectValue = 25;       
const int averageRejectValue = 15;
const int knockFadeTime = 150;    
const int lockTurnTime = 650;     

const int maximumKnocks = 20;      
const int knockComplete = 1200;     


// Variables.
int secretCode[maximumKnocks] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
int knockReadings[maximumKnocks];  
int piezoValue = 0;          
int programButtonPressed = false;   
void listenToSecretKnock(){
  Serial.println("knock starting");

  int i = 0;
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]=0;
  }

  int currentKnockNumber=0;             
  int startTime=millis();                
  int now=millis();

  digitalWrite(greenLed, LOW);            
  if (programButtonPressed==true){
     digitalWrite(redLed, LOW);                         
  }
  delay(knockFadeTime);                                
  digitalWrite(greenLed, HIGH);
  if (programButtonPressed==true){
     digitalWrite(redLed, HIGH);
  }
  do {
    piezoValue = analogRead(piezo);
    if (piezoValue >=minpiezovalue){                  
      
      Serial.println("knock.");
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                            
      startTime=now;
      digitalWrite(greenLed, LOW);
      if (programButtonPressed==true){
        digitalWrite(redLed, LOW);                       
      }
      delay(knockFadeTime);                             
      digitalWrite(greenLed, HIGH);
      if (programButtonPressed==true){
        digitalWrite(redLed, HIGH);
      }
    }

    now=millis();

    //did we timeout or run out of knocks?
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));

 
  if (programButtonPressed==false){       
    if (validateKnock() == true){
      triggerLocker();
    } else {
      Serial.println("Secret knock failed.");
      digitalWrite(greenLed, LOW);     
      for (i=0;i<4;i++){
        digitalWrite(redLed, HIGH);
        delay(100);
        digitalWrite(redLed, LOW);
        delay(100);
      }
      digitalWrite(greenLed, HIGH);
    }
  } else { 
    validateKnock();
    Serial.println("New lock stored.");
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, HIGH);
    for (i=0;i<3;i++){
      delay(100);
      digitalWrite(redLed, HIGH);
      digitalWrite(greenLed, LOW);
      delay(100);
      digitalWrite(redLed, LOW);
      digitalWrite(greenLed, HIGH);
    }
  }
}


void triggerLocker(){
  if(locked == true)
  {
    locked = false;
    Serial.println("box unlocked!");
    int i=0;
  
    myServo.write(0);
  
    delay (lockTurnTime); 
  
    for (i=0; i < 5; i++){
        digitalWrite(greenLed, LOW);
        delay(100);
        digitalWrite(greenLed, HIGH);
        delay(100);
    }
  }
  else
  {
    locked = true;
        Serial.println("box locked!");
        int i=0;
      
        myServo.write(90);
      
        delay (lockTurnTime);  
      
        for (i=0; i < 5; i++){
            digitalWrite(greenLed, LOW);
            delay(100);
            digitalWrite(greenLed, HIGH);
            delay(100);
        }
  }


}

boolean validateKnock(){
  int i=0;

  // premiere verification : a-t-on eu le bon nombre de knocks ?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;            
  Serial.println("debug1");

  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){
      secretKnockCount++;
       
    }

    if (knockReadings[i] > maxKnockInterval){
      maxKnockInterval = knockReadings[i];
    }
  }
  Serial.println("debug2");

  int val = analogRead(0) / 4;
  addr = 0;

  // Cas où on enregistre un nouveau secret
  if (programButtonPressed==true){

      
      for (i=0;i<maximumKnocks;i++){
        secretCode[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);
        EEPROM.write(addr, secretCode[i]);
        addr = addr + 1;
        Serial.println(secretCode[i]);
      }
      Serial.println("debug3");

      // allume les leds avec le pattern enregistré
      digitalWrite(greenLed, LOW);
      digitalWrite(redLed, LOW);
      delay(1000);
      for (i = 0; i < maximumKnocks ; i++){
        digitalWrite(greenLed, LOW);
        digitalWrite(redLed, LOW);
        // only turn it on if there's a delay
        if (secretCode[i] > 0){
          delay( map(secretCode[i],0, 100, 0, maxKnockInterval));
          digitalWrite(greenLed, HIGH);
          digitalWrite(redLed, HIGH);
        }
        delay(50);
      }
    return false;
  }

  if (currentKnockCount != secretKnockCount){
    Serial.println("debug4");

    return false;
  }

  /*  On reprensente les intervales de temps de 0 à 100 pour comparer
  le temps relatif entre les knocks, et non le temps absolu, laissant moins de marge à l'erreur.
  */
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (i=0;i<maximumKnocks;i++){ // Normalize the times
    Serial.println("debug5");
    knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);
    Serial.println(i);
    Serial.println(knockReadings[i]);
    timeDiff = abs(knockReadings[i]-secretCode[i]);
    if (timeDiff > rejectValue){
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // si c'est trop imprecis
  if (totaltimeDifferences/secretKnockCount>averageRejectValue){
    return false;
  }

  return true;

}

void readSecret()
{
  Serial.println("reading secret");
    int val = analogRead(0) / 4;
  addr = 0;
  int i = 0;

  do
  {
    secretCode[i] = EEPROM.read(addr);
    Serial.println(secretCode[i]);
    i++;
    addr = addr + 1;
    delay(100);
  }while(secretCode[i-1] != 0 && i < maximumKnocks);

  done = true;
}

void setup() {
  myServo.attach(9);

  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  pinMode(switchPin, INPUT);
  pinMode(programSwitch, INPUT);

  Serial.begin(9600);

  digitalWrite(greenLed, HIGH);

  myServo.write(90);

  Serial.println("the box is locked!");
}

void loop() {
  
  if(done == false)
  {
    
    readSecret();
  }
  piezoValue = analogRead(piezo);
  

  if (digitalRead(programSwitch)==HIGH){
    programButtonPressed = true;
    digitalWrite(redLed, HIGH);
  } else {
    programButtonPressed = false;
    digitalWrite(redLed, LOW);
  }




  if (piezoValue >=minpiezovalue){
    listenToSecretKnock();
  }
}



