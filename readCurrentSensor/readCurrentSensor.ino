// ... IN PROGRESS
// http://opensource.org/licenses/MIT
// Please credit : chris.keith@gmail.com
// Whole project overview : https://goo.gl/BTyQBJ

#include <stdio.h>

const long MAX_LONG = 2147483647;
const unsigned long MAX_UNSIGNED_LONG = 4294967295;

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char* gMessage;

unsigned long minSecToMillis(unsigned long minutes, unsigned long seconds) {
  return (minutes * 60 * 1000) + (seconds * 1000);
}

char* toFormattedInterval(unsigned long i) {
  unsigned long hours = i / minSecToMillis(60, 0);
  unsigned long remainder = i % minSecToMillis(60, 0);
  unsigned long minutes = remainder / minSecToMillis(1, 0);
  remainder = remainder % minSecToMillis(1, 0);
  unsigned long seconds = remainder / minSecToMillis(0, 1);
  sprintf(gMessage, "%02i:%02i:%02i", (int)hours, (int)minutes, (int)seconds);
  return gMessage;
}

void log(char* message) {
  char* m;
  if ((m = (char *)malloc(255)) == NULL) {
    Serial.println("m malloc failed.");
  } else {
    sprintf(m, "%s\t%s", toFormattedInterval(millis()), message);
    Serial.println(m);
    free(m);
  }
}

void setup() {
  Serial.begin(9600);
  if ((gMessage = (char *)malloc(255)) == NULL) {
    Serial.println("message malloc failed.");
  }
  log("Program start.");
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
  double amps = rms * 30; // TODO: what is 30? check emails from mtoren.
  double watts = peakVoltage * amps;
  
  char* voltageBuf;
  allocAndFormat(&voltageBuf, peakVoltage);
  char* rmsBuf;
  allocAndFormat(&rmsBuf, rms);
  char* ampsBuf;
  allocAndFormat(&ampsBuf, amps);
  char* wattsBuf;
  allocAndFormat(&wattsBuf, watts);
  char* m = NULL;
  if ((m = (char *)malloc(255)) == NULL) {
    log("m malloc failed.");
  } else {
    if (sprintf(m, "ampl=%d\tVolt=%s\tRMS=%s\tamps=%s\twatts=%s", 
          amplitude, voltageBuf, rmsBuf, ampsBuf, wattsBuf) <= 0) {
      m = "sprintf failed.";
    }
  }
  if (m != NULL) {
    log(m);
    free(m);
  }
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

long waitForPowerOnInterval(unsigned long maxIters, unsigned int delaySeconds) {
  // const double AMPS = 3.8; // for hair dryer at low setting.
  // Observed (approximate) amperage when dryer drum is turning.
  const double AMPS = 7.0;

  unsigned long i = 0;
  unsigned long intervalStart = millis();
  while (withinRangeD(readAmps(), AMPS, AMPS * 0.9)) {
    // Prevent infinite loop if something went wrong.
    if (i >= maxIters) {
      return -1;
    }
    i++;
    delay(minSecToMillis(0, delaySeconds));
  }
  return millis() - intervalStart;
}

// Assume that the only 15 second power-on interval is the wrinkle guard interval,
// and ignore all other power-on interals.
// TODO : If this doesn't work, try checking for both sleep and power-on wrinkle guard interval(s).
long handlePowerOn() {
  unsigned long powerOffInterval = WRINKLE_GUARD_SLEEP;
  while (withinRangeL(powerOffInterval, WRINKLE_GUARD_SLEEP, 5)) {
    unsigned long powerOnInterval = MAX_UNSIGNED_LONG;
    while (! withinRangeL(powerOnInterval, WRINKLE_GUARD_ACTIVE, 2)) {
      unsigned long interval = waitForPowerOnInterval(20, 1);
      unsigned long intervalStart = millis();
      // Give onesself 30 seconds (approximate) to get to the dryer.
      delay(WRINKLE_GUARD_SLEEP - minSecToMillis(0,30));
      sendMessage("Dryer should start 15 second tumble cycle in 30 seconds.");

      interval = waitForPowerOnInterval(600, 1);
      powerOffInterval = millis() - intervalStart;
    }
  }
  return powerOffInterval;
}

void monitorDryer() {
  if (waitForPowerOnInterval(20, (WRINKLE_GUARD_ACTIVE * 10) / 9) <= 0 > 0) {
    handlePowerOn();
  }
}

void loop() {
  if (gMessage == NULL) {
    delay(MAX_UNSIGNED_LONG);
  } else {
    if (true) { // for testing sensor unit.
      readAmps();
      delay(minSecToMillis(0, 3));
    } else {
      monitorDryer();
      delay(minSecToMillis(4, 15));
    }
  }
}

