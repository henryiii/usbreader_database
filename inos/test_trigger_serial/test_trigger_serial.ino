int reset;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(13, OUTPUT);  // Set pin 13 as an output
  //pinMode(8, INPUT);    // Set pin 8 as an input (read trigger signal from the switch
  
}

void loop() {
digitalWrite(13, LOW);
  // put your main code here, to run repeatedly:
  while (Serial.available() > 0) {
    digitalWrite(13, HIGH);
    delay(200);
    digitalWrite(13, LOW);
    
    
  }
  
 //digitalWrite(13, LOW);         // Set the output LOW (waiting for a trigger)
  //if ((digitalRead(8)==HIGH) && (reset == 1)) {  // If we have a trigger (switch is pushed, pin 8 jumped from LOW to HIGH) and RESET is done
  //  digitalWrite(13, HIGH);      // Create the pulse on output pin
  //  delay(200);                   // Delay 20 miliseconds
  //  reset = 0;                   // Set the board to unreset state 
  //}
  //if (digitalRead(8)!=HIGH) {    // If the trigger stoped (input pin 8 jumped from HIGH back to LOW) 
  //  reset = 1;                   // Reset the board, ready for the next trigger
  //}
}
