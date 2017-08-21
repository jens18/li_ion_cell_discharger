/* 
 * Name: 
 * Battery Capacity Checker
 *
 * Description:
 * Discharge 2 Li-Ion cells from >4.1V voltage to 2.5V while recording the discharge 
 * profile and cell capacity. Discharge process can be started by pressing a momentary
 * push button switch. A red LED lights up to indicate that the discharge process is
 * still continuing (2.5V cut off has not been reached). 
 * 
 * Circuit:
 * https://goo.gl/photos/qm96asJ2DXB2iUNHA
 * 
 * Example:
 * https://goo.gl/photos/hopcskTPDCRb9MaL8
 *
 * Author:
 * Jens Kaemmerer (jens@mesgtone.net) 
 *
 * Credits:
 * Single cell circuit and source code from:
 * YouTube Video: https://www.youtube.com/embed/qtws6VSIoYk
 * http://AdamWelch.Uk
 */

#define ledPinBatt1 13
#define ledPinBatt2 12
#define ledPinBatt3 11

// digital pin to enable/disable discharge circuit via 50N05L MOSFET
#define gatePinBatt1 10
#define gatePinBatt2 9
#define gatePinBatt3 8

#define buttonPinBatt1 7
#define buttonPinBatt2 6
#define buttonPinBatt3 5

// analog in
#define highPinBatt1 A0
#define lowPinBatt1 A1

#define highPinBatt2 A2
#define lowPinBatt2 A3

#define highPinBatt3 A4
#define lowPinBatt3 A5

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
  int ledPin;
  int buttonPin;
  boolean finished;
  unsigned long previousMillis;
  unsigned long millisPassed;
} 
Batt;  

Batt batt[numBatt] = { 
  { 
    0.0, 0.0, 0.0, 0.0, gatePinBatt1, highPinBatt1, lowPinBatt1, ledPinBatt1, buttonPinBatt1, true, 0, 0   }
  ,
  { 
    0.0, 0.0, 0.0, 0.0, gatePinBatt2, highPinBatt2, lowPinBatt2, ledPinBatt2, buttonPinBatt2, true, 0, 0   }
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

void printBattResult(int i, struct Batt *b) {
  Serial.print(i + 1);
  Serial.print("\t"); 
  Serial.print(b->battVolt);
  Serial.print("\t");
  Serial.print(b->current);
  Serial.print("\t");
  Serial.print(b->mAh);
  Serial.print("\t");
  Serial.println(b->finished ? "Final Capacity" : "Discharge"  );  
}


void setup() {

  Serial.begin(9600);
  Serial.println("Battery Capacity Checker v2");

  // voltRef = readVcc()/1000.0;
  voltRef = 5.0;

  Serial.print("voltRef: ");
  Serial.println(voltRef);

  Serial.println("battIdx  battVolt   current     mAh  state");

  // initialize state
  for( int i = 0; i < numBatt; i++ ) {

    b = &batt[i];

    pinMode(b->gatePin, OUTPUT);
    digitalWrite(b->gatePin, LOW);

    pinMode(b->ledPin, OUTPUT);
    digitalWrite(b->ledPin, LOW);
    
    pinMode(b->buttonPin, INPUT_PULLUP);
  }

  delay(2000);
}

void loop() {

  for(int i = 0; i < numBatt; i++) {

    b = &batt[i];

    // only measure if the discharge process has NOT finished (otherwise undefined voltage will result)
    if(b->finished == false)
    {   
      b->battVolt = analogRead(b->highPin) * voltRef / 1023.0;
      b->shuntVolt = analogRead(b->lowPin) * voltRef / 1023.0;

      /* adjustment to match multimeter measurement, voltage drop on wire to A0/A1 input */
      b->battVolt = b->battVolt + 0.30;
      b->shuntVolt = b->shuntVolt + 0.30;
      
      // battery is not inserted, low value is read
      if(b->battVolt < 1.5) 
      {
        b->battVolt = 0;
      }

      if(b->battVolt >= battLow)
      {
        digitalWrite(b->gatePin, HIGH);
        currentMillis = millis();
        b->millisPassed = currentMillis - b->previousMillis;
        b->current = (b->battVolt - b->shuntVolt) / shuntRes;
        b->mAh = b->mAh + (b->current * 1000.0) * (b->millisPassed / 3600000.0);
        b->previousMillis = currentMillis;
      } 
      else { 
        // turn off discharge circuit
        digitalWrite(b->gatePin, LOW);
        digitalWrite(b->ledPin, LOW);
        b->finished = true;   
      }
    }
    
    if (digitalRead(batt[i].buttonPin) == LOW)
    {
      batt[i].finished = false;
      batt[i].mAh = 0;
      batt[i].current = 0;
      batt[i].battVolt = 0;
      digitalWrite(batt[i].ledPin, HIGH); 
    }

    printBattResult(i, b);
  }
  
  delay(interval);
}    


