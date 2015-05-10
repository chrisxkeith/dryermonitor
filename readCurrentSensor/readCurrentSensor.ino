int sensorPin = A0;	// select the input pin for the potentiometer
int ledPin = 13;  	// select the pin for the LED
//int sensorValue = 0;  // variable to store the value coming from the sensor

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);  
  Serial.begin(9600);
  Serial.println("ampl  peakVolt  rms     amps");
}

void loop() {
  int maxVal = 0;
  int minVal = 1024;
  long startTime = micros();
  const int iters = 148;
  for (int i = 0; i < iters; i++) {
	int sensorValue = analogRead(sensorPin); 
	maxVal  = max(sensorValue, maxVal);
	minVal  = min(sensorValue, minVal);
  }
  long endTime = micros();
  long duration = (endTime - startTime) / iters;
  int amplitude = maxVal - minVal;
  float peakVoltage = (5.0 / 1024) * amplitude / 2;
  float rms = peakVoltage / 1.41421356237;
  float amps = rms * 30; // TODO: what ois 30?
  Serial.print(amplitude);
  Serial.print("    ");
  Serial.print(peakVoltage);
  Serial.print("      ");
  Serial.print(rms);
  Serial.print("    ");
  Serial.println(amps);
 
  delay(2000);          
}

