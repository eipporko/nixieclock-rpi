#include <stdio.h>
#include <time.h>
#include <wiringPi.h>

void nixiePin(int pin, int value){
  if (pin != -1) {
    digitalWrite(pin, value);
  }
}

void nixiePins(int value, int address){
  nixiePin(2, address&1);
  nixiePin(1, address&2);
  nixiePin(0, address&4);

  nixiePin(6, value&1);
  nixiePin(5, value&2);
  nixiePin(4, value&4);
  nixiePin(3, value&8);

  delay (12); //~=85 Hz
}

void initWiringPi() {
  wiringPiSetup();
  int i = 0;
  for (i = 0; i < 7; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}

void testClock() {
  int digit = 0;
  int address = 0;
  for (digit = 0; digit < 10; digit++) {
    for (address = 0; address < 6; address++)
      nixiePins(digit, address);

    delay(250);
  }
}

int main ()
{
  time_t now;
  clock_t start, stop;

  struct tm * ntm;

  initWiringPi();

  testClock();

  while(1) {
    start = clock() / (CLOCKS_PER_SEC / 1000);
    time (&now);
    ntm = localtime (&now);
    printf ("%d : %d : %d \n", ntm->tm_hour, ntm->tm_min, ntm->tm_sec);

    nixiePins(ntm->tm_hour/10, 5);
    nixiePins(ntm->tm_hour%10, 4);
    nixiePins(ntm->tm_min/10, 3);
    nixiePins(ntm->tm_min%10, 2);
    nixiePins(ntm->tm_sec/10, 1);
    nixiePins(ntm->tm_sec%10, 0);

    stop = clock() / (CLOCKS_PER_SEC / 1000);
    delay(1000 - (stop-start));
  }

  return 0;
}
