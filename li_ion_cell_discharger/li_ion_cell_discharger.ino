/* 
 * Battery Capacity Checker
 * Uses Nokia 5110 Display
 * Uses 1 Ohm power resister as shunt - Load can be any suitable resister or lamp
 * 
 * YouTube Video: https://www.youtube.com/embed/qtws6VSIoYk
 * 
 * http://AdamWelch.Uk
 * 
 * Required Library - LCD5110_Graph.h - http://www.rinkydinkelectronics.com/library.php?id=47
 */


// digital pin to enable/disable discharge circuit via 50N05L MOSFET
#define gatePinBatt1 10
#define gatePinBatt2 9

// analog in
#define highPinBatt1 A0
#define lowPinBatt1 A1

#define highPinBatt2 A2
#define lowPinBatt2 A3

#define numBatt 2

// constants
float shuntRes = 1.0;  // In Ohms - Shunt resistor resistance
int interval = 5000;  //Interval (ms) between measurements
// final voltage of a discharged battery (under load)
// http://lygte-info.dk/info/BatteryLowVoltage%20UK.html
float battLow = 2.5;

float voltRef = 5.0; // Reference voltage
unsigned long currentMillis = 0;

typedef struct Batt {
  float mAh;
  float current;
  float battVolt;
  float shuntVolt;
  int gatePin;
  int highPin;
  int lowPin;
  boolean finished;
  unsigned long previousMillis;
  unsigned long millisPassed;
} Batt;  

Batt batt[numBatt] = { 
  { 0.0, 0.0, 0.0, 0.0, gatePinBatt1, highPinBatt1, lowPinBatt1, false, 0, 0 },
  { 0.0, 0.0, 0.0, 0.0, gatePinBatt2, highPinBatt2, lowPinBatt2, false, 0, 0 }
};  

Batt *b;

/*
 * readVcc
 *
 * https://hackingmajenkoblog.wordpress.com/2016/02/01/making-accurate-adc-readings-on-the-arduino/
 */ 
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}


void setup() {

  Serial.begin(9600);
  Serial.println("Battery Capacity Checker v1.1");
  
  voltRef = readVcc()/1000.0;
  
  
  Serial.print("voltRef: ");
  Serial.println(voltRef);
  
  Serial.println("battIdx  battVolt   current     mAh  state");

  for( int i = 0; i < numBatt; i++ ) {
    pinMode(batt[i].gatePin, OUTPUT);
    digitalWrite(batt[i].gatePin, LOW);
  }
  
  delay(2000);
}

void loop() {
    
  for(int i = 0; i < numBatt; i++) {
    
    b = &batt[i];
    
    b->battVolt = analogRead(b->highPin) * voltRef / 1023.0;
    b->shuntVolt = analogRead(b->lowPin) * voltRef / 1023.0;
  
    /* adjustment to match multimeter measurement, voltage drop on wire to A0/A1 input */
    b->battVolt = b->battVolt + 0.05;
    b->shuntVolt = b->shuntVolt + 0.05;

    //Serial.print("battVolt - shuntVolt: ");
    //Serial.println(b->battVolt - b->shuntVolt);

    if(b->battVolt >= battLow && b->finished == false)
    {
      digitalWrite(b->gatePin, HIGH);
      currentMillis = millis();
      b->millisPassed = currentMillis - b->previousMillis;
      b->current = (b->battVolt - b->shuntVolt) / shuntRes;
      b->mAh = b->mAh + (b->current * 1000.0) * (b->millisPassed / 3600000.0);
      b->previousMillis = currentMillis;
     }
  
    if(b->battVolt < battLow)
    {
      // turn off discharge circuit
      digitalWrite(b->gatePin, LOW);
    
      b->finished = true;   
    }
    
    Serial.print(i);
    Serial.print("\t"); 
    Serial.print(b->battVolt);
    Serial.print("\t");
    Serial.print(b->current);
    Serial.print("\t");
    Serial.print(b->mAh);
    Serial.print("\t");
    Serial.println(b->finished ? "Final Capacity" : "Discharge"  );
  }
  
  delay(interval);
}    

