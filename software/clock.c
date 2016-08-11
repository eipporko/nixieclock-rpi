#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include <wiringPi.h>
#include <softPwm.h>

/***********************************************************
*  CONSTANTS
***********************************************************/
#define  ADDR_C_PIN         0
#define  ADDR_B_PIN         1
#define  ADDR_A_PIN         2
#define  DATA_D_PIN	        3
#define  DATA_C_PIN         4
#define  DATA_B_PIN         5
#define  DATA_A_PIN         6
#define  STROBE_PIN         7

#define  ROTARY_A_PIN       21
#define  ROTARY_B_PIN       22
#define  ROTARY_SW_PIN      23

#define  LED_RED_PIN	      24
#define  LED_GREEN_PIN      25
#define  LED_BLUE_PIN       26

#define  PIR_PIN            27
#define  RELAY_PIN          28
#define  UNDEFINED_PIN      29


/***********************************************************
*  GLOBAL VARIABLES
***********************************************************/
static volatile int globalCounter = 0 ;
unsigned char Last_RoB_Status;
unsigned char Current_RoB_Status;

int enableLed = 0;

int fadeMap[3][3] = { {LED_RED_PIN, LED_GREEN_PIN, -1},
                      {-1, LED_GREEN_PIN, LED_BLUE_PIN},
                      {LED_RED_PIN, -1, LED_BLUE_PIN} };


/***********************************************************
*  MATHEMATIC FUNCTIONS
***********************************************************/
int mod(int a, int b) {
  int r = a % b;
  return r < 0 ? r + b : r;
}

/***********************************************************
*  PIR & RELAY FUNCTIONS
***********************************************************/
void pirISR(void)
{
  if ( digitalRead(PIR_PIN) == HIGH ) {
    digitalWrite(RELAY_PIN, HIGH);
    printf("PIR_on\n");
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
    printf("PIR_off\n");
  }
}

/***********************************************************
*  ROTARY ENCODER & RGB LED FUNCTIONS
***********************************************************/
void btnISR(void)
{
  enableLed = (enableLed == 0);

  printf("btnISR\n");
}


PI_THREAD (rotaryDeal)
{
  unsigned char flag;

  while (1) {

    Last_RoB_Status = digitalRead(ROTARY_B_PIN);

    while(!digitalRead(ROTARY_A_PIN)){
      Current_RoB_Status = digitalRead(ROTARY_B_PIN);
      flag = 1;
    }

    if(enableLed && flag == 1){
      flag = 0;

      if((Last_RoB_Status == 0)&&(Current_RoB_Status == 1)){
        globalCounter = mod(++globalCounter, 300);
      }

      if((Last_RoB_Status == 1)&&(Current_RoB_Status == 0)){
        globalCounter = mod(--globalCounter, 300);
      }
    }

  }

}

PI_THREAD (rgbLED) {

  while (1) {

    if (enableLed) {

      int counter = globalCounter;
      int pos = counter/100;
      int channelDecrease = fadeMap[pos][pos];
      int channelIncrease = fadeMap[pos][mod(pos+1,3)];

      softPwmWrite(channelIncrease, mod(counter, 100));
      softPwmWrite(channelDecrease, 100 - mod(counter, 100));
    }
    else {
      softPwmWrite(LED_RED_PIN, LOW);
      softPwmWrite(LED_GREEN_PIN, LOW);
      softPwmWrite(LED_BLUE_PIN, LOW);
    }

  }
}


/***********************************************************
*  NIXIE CLOCK FUNCTIONS
***********************************************************/
void nixiePin(int pin, int value){
  if (pin != -1) {
    digitalWrite(pin, value);
  }
}

void nixiePins(int value, int address){

  nixiePin(ADDR_A_PIN, address&1);
  nixiePin(ADDR_B_PIN, address&2);
  nixiePin(ADDR_C_PIN, address&4);

  nixiePin(DATA_A_PIN, value&1);
  nixiePin(DATA_B_PIN, value&2);
  nixiePin(DATA_C_PIN, value&4);
  nixiePin(DATA_D_PIN, value&8);

  digitalWrite(STROBE_PIN, HIGH);

  delay (1);

  digitalWrite(STROBE_PIN, LOW);

  delay (11);
  //~=85 Hz
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

void initWiringPi() {

  if(wiringPiSetup() < 0){
		fprintf(stderr, "Unable to setup wiringPi:%s\n",strerror(errno));
		exit(1);
	}

  int i = 0;
  for (i = 0; i < 8; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(PIR_PIN, INPUT);
  pinMode(ROTARY_SW_PIN, INPUT);
  pinMode(ROTARY_A_PIN, INPUT);
  pinMode(ROTARY_B_PIN, INPUT);

  softPwmCreate (LED_RED_PIN, 0, 100);
  softPwmCreate (LED_GREEN_PIN, 0, 100);
  softPwmCreate (LED_BLUE_PIN, 0, 100);

  pullUpDnControl(ROTARY_SW_PIN, PUD_UP);

  if(wiringPiISR(ROTARY_SW_PIN, INT_EDGE_FALLING, &btnISR) < 0){
    fprintf(stderr, "Unable to init ISR\n");
    exit(1);
  }

  pullUpDnControl(PIR_PIN, PUD_UP);

  if(wiringPiISR(PIR_PIN, INT_EDGE_BOTH, &pirISR) < 0){
    fprintf(stderr, "Unable to init ISR\n");
    exit(1);
  }

  if (piThreadCreate(rotaryDeal) != 0) {
    printf("Thread didn't start - rotaryDeal");
    exit(1);
  }

  if (piThreadCreate(rgbLED) != 0) {
    printf("Thread didn't start - rgbLED");
    exit(1);
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
    int tdelay = 1000 - (stop-start);
    if (tdelay > 0)
      delay(1000 - (stop-start));
  }

  return 0;
}
