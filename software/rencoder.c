#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <softPwm.h>

#define  RoAPin       11
#define  RoBPin       12
#define  SWPin        13
#define  RedPin	      14
#define  GreenPin     15
#define  BluePin      16

static volatile int globalCounter = 0 ;

unsigned char flag;
unsigned char Last_RoB_Status;
unsigned char Current_RoB_Status;

int enableLed = 0;

int fadeMap[3][3] = { {RedPin, GreenPin, -1},
											{-1, GreenPin, BluePin},
											{RedPin, -1, BluePin} };

int mod(int a, int b) {
	int r = a % b;
	return r < 0 ? r + b : r;
}

void btnISR(void)
{
	enableLed = (enableLed == 0);

	int i = 0;
	for(i=0;i<100;i++){
		softPwmWrite(GreenPin, i);
		delay(2);
	}
	delay(1000);
	for(i=99;i>=0;i--){
		softPwmWrite(GreenPin, i);
		delay(2);
	}
}

void rotaryDeal(void)
{
	Last_RoB_Status = digitalRead(RoBPin);

	while(!digitalRead(RoAPin)){
		Current_RoB_Status = digitalRead(RoBPin);
		flag = 1;
	}

	if(flag == 1){
		flag = 0;
		if((Last_RoB_Status == 0)&&(Current_RoB_Status == 1)){
			globalCounter = mod(++globalCounter, 300);
		}
		if((Last_RoB_Status == 1)&&(Current_RoB_Status == 0)){
			globalCounter = mod(--globalCounter, 300);
		}

		int pos = globalCounter/100;

		int channelDecrease = fadeMap[pos][pos];
		int channelIncrease = fadeMap[pos][mod(pos+1,3)];

		printf("%d\n", globalCounter);
		printf("Decrease [%d][%d] val: %d\n", pos, pos, (100 - mod(globalCounter, 100) ) );
		printf("Increase [%d][%d] val: %d\n", pos, mod(pos+1,3), mod(globalCounter, 100) );

		softPwmWrite(channelIncrease, mod(globalCounter, 100));
		softPwmWrite(channelDecrease, 100 - mod(globalCounter, 100));
	}
}

int main(void)
{
	if(wiringPiSetup() < 0){
		fprintf(stderr, "Unable to setup wiringPi:%s\n",strerror(errno));
		return 1;
	}

	pinMode(SWPin, INPUT);
	pinMode(RoAPin, INPUT);
	pinMode(RoBPin, INPUT);

	softPwmCreate (RedPin, 0, 100);
	softPwmCreate (GreenPin, 0, 100);
	softPwmCreate (BluePin, 0, 100);

	pullUpDnControl(SWPin, PUD_UP);

  if(wiringPiISR(SWPin, INT_EDGE_FALLING, &btnISR) < 0){
		fprintf(stderr, "Unable to init ISR\n");
		return 1;
	}

	while(1){
		rotaryDeal();
	}

	return 0;
}
