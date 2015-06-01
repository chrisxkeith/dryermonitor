#include <stdio.h>

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char* message;

void setup() {
  Serial.begin(9600);
  Serial.println("ampl\tpeakVlt\tRMS\tamps");
  if ((message = (char *)malloc(255)) == NULL) {
    Serial.println("message malloc failed.");
  }
}

void toTwitter(char* message) {
}

void toSerial(char* message) {
  Serial.println("here");
}

char *ftoa(char *a, double f, int precision){
   long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
   
   char *ret = a;
   long heiltal = (long)f;
   itoa(heiltal, a, 10);
   while (*a != '\0') a++;
   *a++ = '.';
   long desimal = abs((long)((f - heiltal) * p[precision]));
   itoa(desimal, a, 10);
   return ret;
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
  double peakVoltage = (5.0 / 1024) * amplitude / 2;
  double rms = peakVoltage / 1.41421356237;
  double amps = rms * 30; // TODO: what is 30?
  if (message == NULL) {
    Serial.println("message malloc failed.");
  } else {
    char* buf;
    if ((buf = (char *)malloc(20)) == NULL) {
      Serial.println("buf malloc failed");
    } else {
      if (sprintf(message, "%d\t%s\t%f\t%f", amplitude, ftoa(buf, peakVoltage, 1), rms, amps) <= 0) {
        message = "sprintf failed.";
      }
      Serial.println(message);
      free(buf);
    }
  }
  delay(5000);          
}

