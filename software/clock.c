#include <stdio.h>
#include <time.h>
#include <wiringPi.h>

void nixiePin(int p, int v){
  if (p != -1) {
    digitalWrite(p, v);
  }
}

void nixiePins(int p1, int p2, int p4, int p8, int v){
  nixiePin(p1,v&1);
  nixiePin(p2,v&2);
  nixiePin(p4,v&4);
  nixiePin(p8,v&8);
}

void initWiringPi() {
  wiringPiSetup();
  int i = 0;
  for (i = 0; i < 7; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}

int main ()
{
  time_t now;
  struct tm * ntm;

  initWiringPi();

  while(1) {
    time (&now);
    ntm = localtime (&now);
    printf ("%d : %d : %d \n", ntm->tm_hour, ntm->tm_min, ntm->tm_sec);
    nixiePins(6, 5, 4, 3, ntm->tm_sec%10);

    delay(1000);
  }

  return 0;
}
