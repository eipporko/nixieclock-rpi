#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <wiringPi.h>
#include <softPwm.h>
#include <pthread.h>

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

#define  LED_RED_PIN        24
#define  LED_GREEN_PIN      25
#define  LED_BLUE_PIN       26

#define  PIR_PIN            27
#define  RELAY_PIN          28
#define  UNDEFINED_PIN      29

#define  BLINK_IN_DELAY     400
#define  BLINK_OUT_DELAY    200

/***********************************************************
*  GLOBAL VARIABLES
***********************************************************/
static volatile int globalCounter = 0 ;
volatile int lastEncoded;
unsigned char MSB;
unsigned char LSB;
int rencoderEnabled = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int rencoderUpdated = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int fadeMap[3][3] = { {LED_RED_PIN, LED_GREEN_PIN, -1},
                      {-1, LED_GREEN_PIN, LED_BLUE_PIN},
                      {LED_RED_PIN, -1, LED_BLUE_PIN} };


/***********************************************************
*  TIME FUNCTIONS
***********************************************************/
static long timeSpecToMillis(struct timespec* ts)
{
  return round( (double)ts->tv_sec * 1000 + (double)ts->tv_nsec / 1000000.0 );
}

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
  pthread_mutex_lock(&lock);

  rencoderEnabled = (rencoderEnabled == 0);
  if (rencoderEnabled)
      pthread_cond_signal(&cond);

  pthread_mutex_unlock(&lock);

  printf("btnISR\n");
}

void updateEncoder() {
  if (rencoderEnabled) {
    int MSB = digitalRead(ROTARY_A_PIN);
    int LSB = digitalRead(ROTARY_B_PIN);

    int encoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | encoded;

    if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
      globalCounter = mod(++globalCounter, 300);
    if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
      globalCounter = mod(--globalCounter, 300);

    lastEncoded = encoded;

    pthread_mutex_lock(&mutex);
    rencoderUpdated = 1;
    pthread_mutex_unlock(&mutex);
  }
}

PI_THREAD (rgbLED) {

  int blinkIn = 1;
  int counter = globalCounter;
  int pos = counter/100;
  int channelDecrease = fadeMap[pos][pos];
  int channelIncrease = fadeMap[pos][mod(pos+1,3)];

  struct timespec timestamp, now;
  clock_gettime(CLOCK_MONOTONIC, &timestamp);

  while (1) {

    if (rencoderEnabled) {

      if (blinkIn) {
        if (rencoderUpdated) {
          counter = globalCounter;
          pos = counter/100;
          channelDecrease = fadeMap[pos][pos];
          channelIncrease = fadeMap[pos][mod(pos+1,3)];

          softPwmWrite(channelIncrease, mod(counter, 100));
          softPwmWrite(channelDecrease, 100 - mod(counter, 100));

          pthread_mutex_lock(&mutex);
          rencoderUpdated = 0;
          pthread_mutex_unlock(&mutex);
        }
        delay(10);
      }
      else {
        softPwmWrite(LED_RED_PIN, LOW);
        softPwmWrite(LED_GREEN_PIN, LOW);
        softPwmWrite(LED_BLUE_PIN, LOW);
        delay(10);

        pthread_mutex_lock(&mutex);
        rencoderUpdated = 1;
        pthread_mutex_unlock(&mutex);
      }

      clock_gettime(CLOCK_MONOTONIC, &now);
      long t = timeSpecToMillis(&now) - timeSpecToMillis(&timestamp);
      if ((blinkIn && t > BLINK_IN_DELAY) || (!blinkIn && t > BLINK_OUT_DELAY))
      {
        blinkIn = (blinkIn == 0);
        clock_gettime(CLOCK_MONOTONIC, &timestamp);
      }

    }
    else {

      softPwmWrite(channelIncrease, mod(counter, 100));
      softPwmWrite(channelDecrease, 100 - mod(counter, 100));

      pthread_mutex_lock(&lock);
      while(!rencoderEnabled) pthread_cond_wait(&cond, &lock);
      pthread_mutex_unlock(&lock);
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


/***********************************************************
*  SETUP FUNCTIONS
***********************************************************/
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

  if(wiringPiISR(ROTARY_A_PIN, INT_EDGE_BOTH, &updateEncoder) < 0){
    fprintf(stderr, "Unable to init ISR\n");
    exit(1);
  }

  if(wiringPiISR(ROTARY_B_PIN,INT_EDGE_BOTH, &updateEncoder) < 0){
    fprintf(stderr, "Unable to init ISR\n");
    exit(1);
  }

  pullUpDnControl(PIR_PIN, PUD_UP);

  if(wiringPiISR(PIR_PIN, INT_EDGE_BOTH, &pirISR) < 0){
    fprintf(stderr, "Unable to init ISR\n");
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
  struct timespec start, end;

  struct tm * ntm;

  initWiringPi();

  testClock();

  while(1) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    time (&now);
    ntm = localtime (&now);
    printf ("%d : %d : %d \n", ntm->tm_hour, ntm->tm_min, ntm->tm_sec);

    nixiePins(ntm->tm_hour/10, 5);
    nixiePins(ntm->tm_hour%10, 4);
    nixiePins(ntm->tm_min/10, 3);
    nixiePins(ntm->tm_min%10, 2);
    nixiePins(ntm->tm_sec/10, 1);
    nixiePins(ntm->tm_sec%10, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long duration = timeSpecToMillis(&start) - timeSpecToMillis(&end);
    int tdelay = 1000 - duration;
    if (tdelay > 0)
      delay(1000 - duration );
  }

  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&lock);
  return 0;
}
