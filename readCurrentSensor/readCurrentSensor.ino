// ... IN PROGRESS
// http://opensource.org/licenses/MIT
// Please credit : chris.keith@gmail.com
// Whole project overview : https://goo.gl/BTyQBJ

#include <stdio.h>

const long MAX_LONG = 2147483647;
const unsigned long MAX_UNSIGNED_LONG = 4294967295;
const unsigned int MAX_MESSAGE_SIZE = 127;

const int sensorPin = A0;	// select the input pin for the electrical current sensor.
char gMessage[MAX_MESSAGE_SIZE] = "";
char prevMessage[MAX_MESSAGE_SIZE] = "";

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

char* fmtMessage(char* message) {
    char* m;
    if ((m = (char *)malloc(MAX_MESSAGE_SIZE)) == NULL) {
      Serial.println("m malloc failed.");
    } else {
      snprintf(m, MAX_MESSAGE_SIZE, "%s\t%s", toFormattedInterval(millis()), message);
      Serial.println(m);
      strncpy(prevMessage, message, MAX_MESSAGE_SIZE - 1);
    }
    return m;
}

void log(char* message) {
  if (strcmp(prevMessage, message) != 0) {
    char* m = fmtMessage(message);
    if (m != NULL) {
      free(m);
    }
  }
}

void setup() {
  log("Program start.");
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
  if ((m = (char *)malloc(MAX_MESSAGE_SIZE)) == NULL) {
    log("m malloc failed.");
  } else {
    if (snprintf(m, MAX_MESSAGE_SIZE, "ampl=%d\tVolt=%s\tRMS=%s\tamps=%s\twatts=%s", 
          amplitude, voltageBuf, rmsBuf, ampsBuf, wattsBuf) <= 0) {
      m = "snprintf failed.";
    }
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

                // If any interval takes longer than an hour, something has gone wrong.
const unsigned int MAX_INTERVAL = minSecToMillis(60, 0);
                // Observed (approximate) amperage when dryer drum is turning.
double AMPS = 7.0;

int waitForPowerOn(const unsigned int maxInterval) {
  unsigned int now = millis();
  while (! withinRangeD(readAmps(), AMPS, AMPS * 0.9)) {
    delay(minSecToMillis(0, 1));
    if (millis() - now > maxInterval) {
      return -1;
    }
  }
  return millis() - now;
}

int waitForPowerOff() {
  unsigned int now = millis();
  while (withinRangeD(readAmps(), AMPS, AMPS * 0.9)) {
    delay(minSecToMillis(0, 1));
    if (millis() - now > MAX_INTERVAL) {
      return -1;
    }
  }
  return millis() - now;
}

                // When the dryer goes into wrinkle guard mode, it is off for 04:45,
                // then powers on for 00:15.
unsigned long wrinkleGuardOff = minSecToMillis(4, 45);
unsigned long wrinkleGuardOn = minSecToMillis(0, 15);
                // Give onesself some time to get to the dryer.
unsigned int warningInterval = minSecToMillis(0, 30);

                // Assume that the only power-on interval that takes 'wrinkleGuardOn' milliseconds
                // is indeed the wrinkle guard interval (and not some other stage in the cycle).
                // TODO : If this doesn't work, try checking for both
                // on and off wrinkle guard intervals.
void handlePowerOn() {
  if (waitForPowerOff() == -1) return;
  if (waitForPowerOn(MAX_INTERVAL) == -1) return;
  int onInterval = 0;
  while (! withinRangeL(onInterval, wrinkleGuardOn, 2000)) {
    onInterval = waitForPowerOff();
    if (onInterval == -1) {
      return;
    }
    if (waitForPowerOn(MAX_INTERVAL) == -1) return;
  }
  if (waitForPowerOff() == -1) return;
                // Now it's at the beginning of the second wrinkle guard cycle.
  while (withinRangeL(onInterval, wrinkleGuardOn, 2000)) {
    delay(wrinkleGuardOff - warningInterval);
    sendMessage("Dryer will start 15 second tumble cycle soon.");
    if (waitForPowerOn(MAX_INTERVAL) == -1) return;
    onInterval = waitForPowerOff();
    if (onInterval == -1) {
      return;
    }
  }
}

void loop() {
  if (true) {     // Manually test sensor unit.
    AMPS = 3.8;   // For hair dryer at "Lo" setting.
    if (true) {   // Just print readings.
      readAmps();
      delay(minSecToMillis(0, 3));
    } else {      // Manually do a simulation. on=30/off=2/30/2/20/10/20/10/...
      wrinkleGuardOff = minSecToMillis(0, 20);
      wrinkleGuardOn = minSecToMillis(0, 10);
      warningInterval = minSecToMillis(0, 5);
      waitForPowerOn(MAX_INTERVAL);
      handlePowerOn();
    }
  } else {
                  // Assume that sensor unit has been initialized while dryer is off.
    waitForPowerOn(MAX_UNSIGNED_LONG);
    handlePowerOn();
                  // Assume (if we start another load) it will take us more than 1 minute.
    delay(minSecToMillis(1, 0));
  }
}

