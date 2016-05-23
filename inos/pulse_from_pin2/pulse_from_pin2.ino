
// Pin 13 has an LED connected on most Arduino boards.
#define LED_PIN 13

// 0:
// 1: ready for next trigger
int reset = 1;

int duration = 1000; // ms

  // put your setup code here, to run once:
void setup() {

  Serial.begin(9600); 
  pinMode(LED_PIN, OUTPUT);  // Set pin as an output
  
}


  // put your main code here, to run over and over again forever: 
void loop() {

  // turn the LED off
  digitalWrite(LED_PIN, LOW);

  int inByte = Serial.read();
  printf(" received %d \n", inByte);
  Serial.println(inByte);

  // if we have input and RESET is done
  if ((Serial.available() > 0) && (reset == 1)) {  
    // create the pulse on output pin
    digitalWrite(LED_PIN, HIGH);      
    delay(duration);                   // Delay
    reset = 0;
  }

  // if input is absent
  if (!(Serial.available())) {    
    reset = 1;
  }
}

// input: 0 ------- reset: 0 ------->-------- LED OFF ------ reset: 1
// input: 0 ------- reset: 1 ------->-------- LED OFF ------ reset: 1
// ...
// input: 0 ------- reset: 1 ------->-------- LED OFF ------ reset: 1
// input: 1 ------- reset: 1 ------->-------- LED ON  ------ reset: 0
// input: 1 ------- reset: 0 ------->-------- LED OFF ------ reset: 0
// ...
// input: 1 ------- reset: 0 ------->-------- LED OFF ------ reset: 0
// input: 0 ------- reset: 0 ------->-------- LED OFF ------ reset: 1

