// ... IN PROGRESS
// http://opensource.org/licenses/MIT
// Please credit : chris.keith@gmail.com
// Whole project overview : https://goo.gl/BTyQBJ

#include <stdio.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Twitter.h>

enum test_status_type {
  AUTOMATED,
  MANUAL,
  NONE
};
const test_status_type testStatus = AUTOMATED;

const long MAX_LONG = 2147483647;
const unsigned long MAX_UNSIGNED_LONG = 4294967295;
const unsigned int MAX_MESSAGE_SIZE = 96;
const unsigned long TIMEOUT_INDICATOR = MAX_UNSIGNED_LONG;

                // If any interval takes longer than an hour, something has gone wrong.
const unsigned long ONE_HOUR = minSecToMillis(60, 0);

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char gMessage[MAX_MESSAGE_SIZE] = "";
double previousAmps = -1.0;
unsigned long previousLogTime = MAX_UNSIGNED_LONG;

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
    unsigned long logInterval = 0;
    if (previousLogTime != MAX_UNSIGNED_LONG) {
      logInterval = (millis() - previousLogTime) / 1000;
    }
    previousLogTime = millis();
    snprintf(logLine, MAX_MESSAGE_SIZE, "%s [%d]\t%s",
            toFormattedInterval(previousLogTime), (unsigned int)logInterval, message);
    Serial.println(logLine);
}

byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF5, 0xFE };
// byte ip[] = { 192, 168, 100, 9 };
Twitter twitter("TODO : Replace this text with your token");

void postToTwitter(char msg[]) {
  if (twitter.post(msg)) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("OK.");
    } else {
      Serial.print("failed : code ");
      Serial.println(status);
    }
  } else {
    Serial.println("twitter.post() failed.");
  }
}

void setup() {
  Serial.begin(9600);
/*
  log("Before Ethernet.begin");
  if (Ethernet.begin(mac) == 0) {
    log("Ethernet.begin failed.");
  }
  log("After Ethernet.begin");
  log(Ethernet.localIP());
  postToTwitter("Starting dryer monitor.");
*/
}

void sendMessage(char* message) {
  char msg[MAX_MESSAGE_SIZE];
  snprintf(msg, MAX_MESSAGE_SIZE, "Msg: %s", message);
  log(msg);
//  postToTwitter(message);
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

/*
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
      "amps=%s\tmax=%d\tmin=%d\tampl=%d\tVolt=%s\tRMS=%s\twatts=%s", 
      ampsBuf, maxVal, minVal, amplitude, voltageBuf, rmsBuf, wattsBuf) <= 0) {
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
*/
void logAmps(double amps) {
  if (withinRangeD(amps, previousAmps, 0.5)) {
    return;
  }
  char ampsBuf[20];
  char vals[32];
  if (snprintf(vals, 32, "amps=%s", ftoa(ampsBuf, amps, 2)) <= 0) {
    log("snprintf failed.");
  } else {
    log(vals);
  }
  previousAmps = amps;
}

double readActualAmps() {
  int minVal = 1023;
  int maxVal = 0;
  const int iters = 500; // get this number of readings 1 ms apart.
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
  logAmps(amps);
  return amps;
}
                // When the dryer goes into wrinkle guard mode, it is off for 04:45,
                // then powers on for 00:15.
unsigned long wrinkleGuardOff = minSecToMillis(4, 45);
unsigned long wrinkleGuardOn = minSecToMillis(0, 15);
                // Give onesself 30 seconds to get to the dryer.
unsigned int warningInterval = minSecToMillis(0, 30);

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
  TEST_WRINKLE_CYCLE_ON_SECS, 24 // simulate a 'dryer unused' period.
};
const unsigned int TOTAL_TEST_CYCLE_SECONDS = 94; // Change if total test cycle time changes.
bool stateAtSecondsDelta[TOTAL_TEST_CYCLE_SECONDS];
unsigned long testStartTime = MAX_UNSIGNED_LONG;

void automatedTest() {
  if (testStartTime == MAX_UNSIGNED_LONG) {
    char buf[32];
    wrinkleGuardOff = minSecToMillis(0, TEST_WRINKLE_CYCLE_OFF_SECS);
    wrinkleGuardOn = minSecToMillis(0, TEST_WRINKLE_CYCLE_ON_SECS);
    warningInterval = minSecToMillis(0, TEST_WRINKLE_CYCLE_OFF_SECS - 1);
    int cycleIndex = 0;
    bool on = true;
    int accum = TEST_CYCLE_SECONDS[cycleIndex];
    for (unsigned int i = 0; i < sizeof(stateAtSecondsDelta); i++) {
      if (i >= accum) {
        accum += TEST_CYCLE_SECONDS[++cycleIndex];
        on = ! on;
       }
      stateAtSecondsDelta[i] = on;
      snprintf(buf, 32, "i: %d, state: %d", i, (int)on);
//      log(buf);
    }
    testStartTime = millis();
    snprintf(buf, 32, "testStartTime: %d", testStartTime);
    log(buf);
  }
  doRun();
}

double readAutomatedAmps() {
  unsigned long seconds = ((millis() - testStartTime) / 1000) % TOTAL_TEST_CYCLE_SECONDS;
  double amps;
  if (stateAtSecondsDelta[(int)seconds]) {
    amps = AMPS;
  } else {
    amps = 0.0;
  }
  logAmps(amps);
  return amps;
}

double readAmps() {
  if (testStatus == AUTOMATED) {
    return readAutomatedAmps();
  }
  return readActualAmps();
}

unsigned long waitForPowerChange( boolean (*f)(double, double, double), const unsigned long timeoutDelta) {
  unsigned long start = millis();
  while ((*f)(readAmps(), AMPS, AMPS * 0.9)) {
    delay(minSecToMillis(0, 1));
    if (millis() - start > timeoutDelta) {
      return TIMEOUT_INDICATOR;
    }
  }
  return millis() - start;
}

unsigned long waitForPowerOn(const long timeoutDelta) {
  return waitForPowerChange(outsideRangeD, timeoutDelta);
}

unsigned long waitForPowerOff(const long timeoutDelta) {
  return waitForPowerChange(withinRangeD, timeoutDelta);
}

void doRun() {
                  // Assume that sensor unit has been initialized while dryer is off.
  waitForPowerOn(MAX_UNSIGNED_LONG);
                // Assume that the only power-on interval that takes 'wrinkleGuardOn' milliseconds
                // is indeed the wrinkle guard interval (and not some other stage in the cycle).
                // TODO : If this doesn't work, try checking for both on and off wrinkle guard intervals.
  unsigned long onInterval;
  while (true) {
    onInterval = waitForPowerOff(ONE_HOUR);
    if (onInterval == TIMEOUT_INDICATOR) {
      log("doRun: TIMEOUT 1.");
      return;
    }
    // Allow 3 seconds for delta check.
    if (withinRangeL(onInterval, wrinkleGuardOn, 3000)) {
      break;
    }
    if (waitForPowerOn(ONE_HOUR) == TIMEOUT_INDICATOR) {
      log("doRun: TIMEOUT 2.");
      return;
    }
  }
  if (waitForPowerOff(ONE_HOUR) == TIMEOUT_INDICATOR) {
    log("doRun: TIMEOUT 3.");
    return;
  }
                // Now it's at the beginning of the second wrinkle guard off cycle.
  while (withinRangeL(onInterval, wrinkleGuardOn, 3000)) {
    delay(wrinkleGuardOff - warningInterval);
    char m[MAX_MESSAGE_SIZE];
    snprintf(m, MAX_MESSAGE_SIZE, "Dryer will start 15 second tumble cycle in %d seconds.",
            warningInterval / 1000);
    sendMessage(m);
    unsigned long offInterval = waitForPowerOn(ONE_HOUR); 
    if (offInterval == TIMEOUT_INDICATOR) {
      log("doRun: TIMEOUT 4.");
      return;
    }
//    snprintf(m, MAX_MESSAGE_SIZE, "offInterval : %d millis.", (int)offInterval);
//    log(m);
    
    onInterval = waitForPowerOff(ONE_HOUR);
//    snprintf(m, MAX_MESSAGE_SIZE, "onInterval : %d millis.", (int)onInterval);
//    log(m);
    if (onInterval == TIMEOUT_INDICATOR) {
      log("doRun: TIMEOUT 5.");
      return;
    }
  }
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
}

