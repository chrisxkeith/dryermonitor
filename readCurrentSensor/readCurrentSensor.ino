// http://opensource.org/licenses/MIT
// Please credit : chris.keith@gmail.com
// ... IN PROGRESS ...

#include <stdio.h>

const long MAX_LONG = 2147483647;
const unsigned long MAX_UNSIGNED_LONG = 4294967295;

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char* message;
unsigned long sensorstart;

void setup() {
  Serial.begin(9600);
  if ((message = (char *)malloc(255)) == NULL) {
    Serial.println("message malloc failed.");
  }
  sensorstart = millis();
}

unsigned long minSecToMillis(unsigned long minutes, unsigned long seconds) {
  return (minutes * 60 * 1000) + (seconds * 1000);
}

void sendMessage(char* message) {
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
  const int iters = 150; // get 150 readings 1 ms apart.
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
  // TODO : add timestamp
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

// Check if two longs are within 'epsilon' of each other.
boolean withinRangeL(long v1, long v2, long epsilon) {
  return abs(v1 - v2) < epsilon;
}

// Check if two doubles are within 'epsilon' of each other.
boolean withinRangeD(double v1, double v2, double epsilon) {
  return abs(v1 - v2) < epsilon;
}

// When the dryer goes into wrinkle guard mode, it is off for 04:45,
// then powers on for 00:15.
const unsigned long WRINKLE_GUARD_SLEEP = minSecToMillis(4, 45);
const unsigned long WRINKLE_GUARD_ACTIVE = minSecToMillis(0, 15);
const unsigned long WRINKLE_GUARD_CYCLE = WRINKLE_GUARD_SLEEP + WRINKLE_GUARD_ACTIVE;

// Observed (approximate) amperage when dryer drum is turning.
const double AMPS = 7.0;

void monitorDryer() {

  // Wait until dryer power indicates that drum is turning.
  while (! withinRangeD(readAmps(), AMPS, AMPS * 0.9)) {
    // Wait 90% of active power-on cycle to increase the odds of finding it.
    delay((WRINKLE_GUARD_ACTIVE * 10) / 9);
  }

// Assume that the only 15 second power-on interval is the wrinkle guard interval,
// and ignore all other power-on interals.
// If this doesn't work, try checking for both sleep and power-on wrinkle guard cycle(s).

  unsigned long powerOffInterval = WRINKLE_GUARD_SLEEP;
  while (withinRangeL(powerOffInterval, WRINKLE_GUARD_SLEEP, 5)) {
    
    unsigned long powerOnInterval = MAX_UNSIGNED_LONG;
    unsigned long intervalStart;
    while (! withinRangeL(powerOnInterval, WRINKLE_GUARD_ACTIVE, 2)) {
      // TODO : add a timeout to handle errors and edge cases.
      intervalStart = millis();
      while (withinRangeD(readAmps(), AMPS, AMPS * 0.9)) {
        delay(minSecToMillis(0, 1));    
      }
      powerOnInterval = millis() - intervalStart;
    }
    
    intervalStart = millis();
    // Give onesself 30 seconds (approximate) to get to the dryer.
    delay(WRINKLE_GUARD_SLEEP - minSecToMillis(0,30));
    sendMessage("");

    while (withinRangeD(readAmps(), AMPS, AMPS * 0.9)) {
      delay(minSecToMillis(0, 1));    
    }
    powerOffInterval = millis() - intervalStart;
  }
}

void loop() {
  if (message == NULL) {
    delay(MAX_UNSIGNED_LONG);
  } else {
    if (true) { // for testing sensor unit.
      readAmps();
      delay(minSecToMillis(0, 3));
    } else {
      monitorDryer();
    }
  }
}

