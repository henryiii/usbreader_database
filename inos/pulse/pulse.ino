
// Pin 13 has an LED connected on most Arduino boards.
#define LED_PIN 13

int duration = 1000; // ms
bool first = true;

  // put your setup code here, to run once:
void setup()
{
  // initialize the digital pin as an output.
    pinMode(LED_PIN, OUTPUT);
}

  // put your main code here, to run over and over again forever: 
void loop()
{

    if( first ){
        // turn the LED on (HIGH is the voltage level)
    	digitalWrite(LED_PIN, HIGH);
	first = false;
	delay(duration);  // wait
	// turn the LED off by making the voltage LOW
	digitalWrite(LED_PIN, LOW);
    }

}
