// http://opensource.org/licenses/MIT
// Please credit : chris.keith@gmail.com
// ... IN PROGRESS ...

#include <stdio.h>

const long MAX_LONG = 2147483647;
const unsigned long MAX_UNSIGNED_LONG = 4294967295;

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char* message;

void setup() {
  Serial.begin(9600);
  if ((message = (char *)malloc(255)) == NULL) {
    Serial.println("message malloc failed.");
  }
}

unsigned long minSecToMillis(unsigned long minutes, unsigned long seconds) {
  return (minutes * 60 * 1000) + (seconds * 1000);
}

void toTwitter(char* message) {
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

char * allocAndFormat(char** bufPtr, double d) {
    if ((*bufPtr = (char *)malloc(20)) == NULL) {
      return "double malloc failed";
    }
    return ftoa(*bufPtr, d, 1);
}

double readAmps() {
  int minVal = 1024;
  int maxVal = 0;
  const int iters = 150;
  for (int i = 0; i < iters; i++) {
    int sensorValue = analogRead(sensorPin); 
    minVal  = min(sensorValue, minVal);
    maxVal  = max(sensorValue, maxVal);
    delay(1L);
  }
  
  int amplitude = maxVal - minVal;
  double peakVoltage = (5.0 / 1024) * amplitude / 2;
  double rms = peakVoltage / 1.41421356237;
  double amps = rms * 30; // TODO: what is 30?
  double watts = peakVoltage * amps;
  
  char* voltageBuf;
  allocAndFormat(&voltageBuf, peakVoltage);
  char* rmsBuf;
  allocAndFormat(&rmsBuf, rms);
  char* ampsBuf;
  allocAndFormat(&ampsBuf, amps);
  char* wattsBuf;
  allocAndFormat(&wattsBuf, watts);
  if (sprintf(message, "ampl=%d\tVolt=%s\tRMS=%s\tamps=%s\twatts=%s", 
        amplitude, voltageBuf, rmsBuf, ampsBuf, wattsBuf) <= 0) {
    message = "sprintf failed.";
  }
  Serial.println(message);
  free(voltageBuf);
  free(rmsBuf);
  free(ampsBuf);
  free(wattsBuf);
  
  return amps;
}

void monitorDryer() {
  const double DRYER_AMPS = 7.0;
  const double DRYER_THRESHOLD = DRYER_AMPS * 0.9;
  const unsigned long WRINKLE_GUARD_SLEEP = minSecToMillis(4, 45);
  const unsigned long WRINKLE_GUARD_ACTIVE = minSecToMillis(0, 15);

  while (readAmps() < DRYER_THRESHOLD) {
    delay(60 * 1000);    
  }
  // Could be in a drying cycle or in a wrinkle guard active state.
  unsigned long t = millis();
  const unsigned long MAX_WRINKLE_GUARD_CYCLES = 50; // if we ever hit this, there's a bug.
}

void loop() {
  if (message != NULL) {
    readAmps();
    // Until code has been tested and debugged.
    // monitorDryer();
    delay(minSecToMillis(60, 0));          
  } else {
    delay(MAX_UNSIGNED_LONG);
  }
}

