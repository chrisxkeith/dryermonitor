// ... IN PROGRESS
// http://opensource.org/licenses/MIT
// Please credit : chris.keith@gmail.com
// Whole project overview : https://goo.gl/BTyQBJ

#include <stdio.h>

enum test_status_type {
  AUTOMATED,
  MANUAL,
  NONE
};
const test_status_type testStatus = AUTOMATED;

const long MAX_LONG = 2147483647;
const unsigned long MAX_UNSIGNED_LONG = 4294967295;
const unsigned int MAX_MESSAGE_SIZE = 127;
const long TIMEOUT_INDICATOR = -1;

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char gMessage[MAX_MESSAGE_SIZE] = "";
char prevMessage[MAX_MESSAGE_SIZE] = "";
double previousAmps = -1.0;

                // Observed (approximate) amperage when dryer drum is turning.
                // Also hair dryer on 'Lo'.
double AMPS = 7.0;

unsigned long minSecToMillis(unsigned long minutes, unsigned long seconds) {
  return (minutes * 60 * 1000) + (seconds * 1000);
}

char* toFormattedInterval(unsigned long i) {
  unsigned long hours = i / minSecToMillis(60, 0);
  unsigned long remainder = i % minSecToMillis(60, 0);
  unsigned long minutes = remainder / minSecToMillis(1, 0);
  remainder = remainder % minSecToMillis(1, 0);
  unsigned long seconds = remainder / minSecToMillis(0, 1);
  snprintf(gMessage, MAX_MESSAGE_SIZE, "%02i:%02i:%02i", (int)hours, (int)minutes, (int)seconds);
  return gMessage;
}

char logLine[MAX_MESSAGE_SIZE];
void log(char* message) {
  if (strcmp(prevMessage, message) != 0) {
    snprintf(logLine, MAX_MESSAGE_SIZE, "%s\t%s", toFormattedInterval(millis()), message);
    Serial.println(logLine);
    strncpy(prevMessage, message, MAX_MESSAGE_SIZE - 1);
  }
}

void setup() {
  Serial.begin(9600);
}

void sendMessage(char* message) {
  log("Message would have been sent...");
}

char *ftoa(char *a, double f, int precision){
   long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
   
   char *ret = a;
   long heiltal = (long)f;
   itoa(heiltal, a, 10);
   while (*a != '\0') a++;
   *a++ = '.';
   long desimal = abs((long)((f - heiltal) * p[precision]));
   if (desimal < p[precision - 1]) {  //are there leading zeros?
     *a='0'; a++;
   }
   itoa(desimal, a, 10);
   return ret;
}

char * allocAndFormat(char** bufPtr, double d) {
    if ((*bufPtr = (char *)malloc(20)) == NULL) {
      return "double malloc failed";
    }
    return ftoa(*bufPtr, d, 2);
}

// Check if two longs are within 'epsilon' of each other.
boolean withinRangeL(long v1, long v2, long epsilon) {
  return abs(v1 - v2) < epsilon;
}

// Check if two doubles are within 'epsilon' of each other.
boolean withinRangeD(double v1, double v2, double epsilon) {
  return abs(v1 - v2) <= epsilon;
}

// Check if two doubles are NOT within 'epsilon' of each other.
boolean outsideRangeD(double v1, double v2, double epsilon) {
  return abs(v1 - v2) > epsilon;
}

char vals[MAX_MESSAGE_SIZE];
void logValues(int maxVal, int minVal, int amplitude, double peakVoltage,
               double rms, double amps, double watts) {
  if (withinRangeD(amps, previousAmps, 0.5)) {
    return;
  }
  char* voltageBuf;
  allocAndFormat(&voltageBuf, peakVoltage);
  char* rmsBuf;
  allocAndFormat(&rmsBuf, rms);
  char* ampsBuf;
  allocAndFormat(&ampsBuf, amps);
  char* wattsBuf;
  allocAndFormat(&wattsBuf, watts);
  if (snprintf(vals, MAX_MESSAGE_SIZE,
      "max=%d\tmin=%d\tampl=%d\tVolt=%s\tRMS=%s\tamps=%s\twatts=%s", 
      maxVal, minVal, amplitude, voltageBuf, rmsBuf, ampsBuf, wattsBuf) <= 0) {
    log("snprintf failed.");
  } else {
    log(vals);
  }
  free(voltageBuf);
  free(rmsBuf);
  free(ampsBuf);
  free(wattsBuf);
  previousAmps = amps;
}

double readActualAmps() {
  int minVal = 1023;
  int maxVal = 0;
  const int iters = 500; // get 150 readings 1 ms apart.
  for (int i = 0; i < iters; i++) {
    int sensorValue = analogRead(sensorPin);
    minVal  = min(sensorValue, minVal);
    maxVal  = max(sensorValue, maxVal);
    delay(1L);
  }
  int amplitude = maxVal - minVal;

  // Noise ??? causing amplitude to never be zero. Kludge around it.
  if (amplitude < 18) { // 18 > observed max noise.
    amplitude = 0;
  }

  // Arduino at 5.0V, analog input is 0 - 1023.
  double peakVoltage = (5.0 / 1024) * amplitude / 2;
  double rms = peakVoltage / 1.41421356237;

// Regarding the current sensor, the sensor we both have is configured in such
// a way that when 1V of AC is passing through it on the output side, it means
// that 30A is passing through it on the input side.  So, measure the amount
// of AC voltage by computing the Root Mean Squared, and then multiply the
// number of volts by 30 to arrive at the number of amps.

  double amps = rms * 30;
  double watts = peakVoltage * amps;
  logValues(maxVal, minVal, amplitude, peakVoltage, rms, amps, watts);
  return amps;
}

const int TEST_ACTIVE_CYCLE_ON_SECS = 16;
const int TEST_ACTIVE_CYCLE_OFF_SECS = 2;
const int TEST_WRINKLE_CYCLE_ON_SECS = 4;
const int TEST_WRINKLE_CYCLE_OFF_SECS = 8;
const int TEST_CYCLE_SECONDS[] = {
  TEST_ACTIVE_CYCLE_ON_SECS, TEST_ACTIVE_CYCLE_OFF_SECS,
  TEST_WRINKLE_CYCLE_ON_SECS, TEST_WRINKLE_CYCLE_OFF_SECS,
  TEST_WRINKLE_CYCLE_ON_SECS, TEST_WRINKLE_CYCLE_OFF_SECS,
  TEST_WRINKLE_CYCLE_ON_SECS, TEST_WRINKLE_CYCLE_OFF_SECS,
  TEST_WRINKLE_CYCLE_ON_SECS, TEST_WRINKLE_CYCLE_OFF_SECS,
  TEST_WRINKLE_CYCLE_ON_SECS, TEST_WRINKLE_CYCLE_OFF_SECS
};
const unsigned int TOTAL_TEST_CYCLE_SECONDS = 78; // re-set if change TEST_* const's above.
bool stateAtSecondsDelta[TOTAL_TEST_CYCLE_SECONDS];
unsigned int testStartTime = 0;

double readAutomatedAmps() {
  if (testStartTime == 0) {
    testStartTime = millis();
    int cycleIndex = 0;
    int accum = TEST_CYCLE_SECONDS[cycleIndex];
    bool on = true;
    for (unsigned int i = 0; i < sizeof(stateAtSecondsDelta); i++) {
     if (i >= accum) {
      accum += TEST_CYCLE_SECONDS[++cycleIndex];
      on = ! on;
     }
     stateAtSecondsDelta[i] = on;
    }
  }
  double amps;
  int seconds = ((millis() - testStartTime) / 1000) % TOTAL_TEST_CYCLE_SECONDS;
  if (stateAtSecondsDelta[seconds]) {
    amps = AMPS;
  } else {
    amps = 0.0;
  }
  logValues(0, 0, 0, 0.0, 0.0, amps, 0.0);
  return amps;
}

double readAmps() {
  if (testStatus == AUTOMATED) {
    return readAutomatedAmps();
  }
  return readActualAmps();
}

                // If any interval takes longer than an hour, something has gone wrong.
const unsigned int ONE_HOUR = minSecToMillis(60, 0);

int waitForPowerChange( boolean (*f)(double, double, double), const long timeoutDelta) {
  unsigned int now = millis();
  while (! (*f)(readAmps(), AMPS, AMPS * 0.9)) {
    delay(minSecToMillis(0, 1));
    if (millis() - now > timeoutDelta) {
      return TIMEOUT_INDICATOR;
    }
  }
  return millis() - now;
}

int waitForPowerOn(const long timeoutDelta) {
  return waitForPowerChange(withinRangeD, timeoutDelta);
}

int waitForPowerOff(const long timeoutDelta) {
  return waitForPowerChange(outsideRangeD, timeoutDelta);
}

                // When the dryer goes into wrinkle guard mode, it is off for 04:45,
                // then powers on for 00:15.
unsigned long wrinkleGuardOff = minSecToMillis(4, 45);
unsigned long wrinkleGuardOn = minSecToMillis(0, 15);
                // Give onesself 30 seconds to get to the dryer.
unsigned int warningInterval = minSecToMillis(0, 30);

                // Assume that the only power-on interval that takes 'wrinkleGuardOn' milliseconds
                // is indeed the wrinkle guard interval (and not some other stage in the cycle).
                // TODO : If this doesn't work, try checking for both
                // on and off wrinkle guard intervals.
void handlePowerOn() {
  if (waitForPowerOff(ONE_HOUR) == TIMEOUT_INDICATOR) return;
  if (waitForPowerOn(ONE_HOUR) == TIMEOUT_INDICATOR) return;
  int onInterval = 0;
  while (! withinRangeL(onInterval, wrinkleGuardOn, 1000)) {
    onInterval = waitForPowerOff(ONE_HOUR);
    if (onInterval == TIMEOUT_INDICATOR) {
      return;
    }
    if (waitForPowerOn(ONE_HOUR) == TIMEOUT_INDICATOR) return;
  }
  if (waitForPowerOff(ONE_HOUR) == TIMEOUT_INDICATOR) return;
                // Now it's at the beginning of the second wrinkle guard cycle.
  while (withinRangeL(onInterval, wrinkleGuardOn, 1000)) {
    delay(wrinkleGuardOff - warningInterval);
    sendMessage("Dryer will start 15 second tumble cycle soon.");
    if (waitForPowerOn(ONE_HOUR) == TIMEOUT_INDICATOR) return;
    onInterval = waitForPowerOff(ONE_HOUR);
    if (onInterval == TIMEOUT_INDICATOR) {
      return;
    }
  }
}

void doRun() {
                  // Assume that sensor unit has been initialized while dryer is off.
  waitForPowerOn(MAX_UNSIGNED_LONG);
  handlePowerOn();
}

void automatedTest() {
  wrinkleGuardOff = minSecToMillis(0, TEST_WRINKLE_CYCLE_OFF_SECS);
  wrinkleGuardOn = minSecToMillis(0, TEST_WRINKLE_CYCLE_ON_SECS);
  warningInterval = minSecToMillis(0, TEST_WRINKLE_CYCLE_OFF_SECS - 1);
  doRun();
}

void loop() {
  switch (testStatus) {
    case AUTOMATED: {
      automatedTest();
      break;
    }
    case MANUAL: {
      readActualAmps();
      break;
    }
    case NONE:
    default: {
      doRun();
      break;
    }
  }
  delay(minSecToMillis(0, 1));
}

