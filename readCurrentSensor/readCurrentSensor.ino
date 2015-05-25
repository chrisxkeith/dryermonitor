const int sensorPin = A0;	// select the input pin for the electrical current sensor.

void setup() {
  Serial.begin(9600);
  Serial.println("ampl\tpeakVlt\tRMS\tamps");
}

void loop() {
  int minVal = 1024;
  int maxVal = 0;
  const int iters = 150;
  for (int i = 0; i < iters; i++) {
    int sensorValue = analogRead(sensorPin); 
    minVal  = min(sensorValue, minVal);
    maxVal  = max(sensorValue, maxVal);
    delay(1);
  }
  int amplitude = maxVal - minVal;
  float peakVoltage = (5.0 / 1024) * amplitude / 2;
  float rms = peakVoltage / 1.41421356237;
  float amps = rms * 30; // TODO: what is 30?
  Serial.print(amplitude);
  Serial.print("\t");
  Serial.print(peakVoltage);
  Serial.print("\t");
  Serial.print(rms);
  Serial.print("\t");
  Serial.print(amps); 
  Serial.println("");
  
  delay(5000);          
}

