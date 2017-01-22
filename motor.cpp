#include <wiringPi.h>
#include <thread> 
#include <mutex>
#include <atomic>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

class Motor;
void ThreadHelper(Motor*);

class Encoder;
void ThreadHelperE(Encoder*);

class Encoder
{

std::thread* mthread;
volatile bool stop_thread = false;
int p1;
int p2;
int speed = 0;
long lasttime = 0;
long time = 0;
int counter;

public:
Encoder(int p1, int p2)
{
	this->p1 = p1;
	this->p2 = p2;
	pinMode (p1, INPUT) ;
	pinMode (p2, INPUT) ;
	stop_thread = false;
	mthread = new std::thread(&ThreadHelperE,this);
}

~Encoder()
{
	stop_thread = true;
	mthread->join();
}
 
void calcSpeed()
{
	speed = 10000/(time - lasttime);
	lasttime = time;
	//printf("%d\n", speed);
}	
 
void Thread()
{

	int lastP1 = LOW;
	int lastP2 = LOW;
	int state = 1;
	while(!stop_thread)
	{
		int P1 = digitalRead(p1);
		int P2 = digitalRead(p2);
		if (state == 1)
		{
			if(P1 != lastP1)
			{
				if(lastP2 == P2)
				{
					if(P2 == HIGH && lastP1 == LOW && P1 == HIGH)
					{	
						counter--;
						//calcSpeed();
					} else if(P2 == LOW && lastP1 == HIGH && P1 == LOW)
					{
						counter--;
						calcSpeed();
					} else
					{
						counter++;
						//calcSpeed();	
					}
				}
				state = 2;
				lastP1 = P1;
				lastP2 = P2;

			}
		}
		else if (state == 2)
		{
			if(P2 != lastP2)
			{
				if(lastP1 == P1)
				{
					if(P1 == HIGH && lastP2 == LOW && P2 == HIGH)
					{	
						counter++;
						//calcSpeed();
					} else if(P1 == LOW && lastP2 == HIGH && P2 == LOW)
					{
						counter++;	
						//calcSpeed();
					} else
					{
						counter--;	
						//calcSpeed();
					}	
						
					
				}		
				state = 1;
				lastP1 = P1;
				lastP2 = P2;
			}
		}
		time++;
		if(time >= lasttime + 1500)
		{
			speed = 0;
		}	
		usleep(100);
	}
}

int Speed()
{

	return speed;
}
int Counter()
{
	return counter;
}
      	
}; //Encoder

void ThreadHelperE(Encoder* e)
{
	e->Thread();
}



class Motor
{
int p1;
int p2;
int power = 1;
int speed;

std::thread* mthread;
volatile bool stop_thread = false;

public:

Encoder e;

Motor(int a, int b, int p1, int p2) : e(p1, p2)

{
	this->p1 = a;
	this->p2 = b;
	digitalWrite (p1, LOW);
	digitalWrite (p2, LOW);
	pinMode (p1, OUTPUT) ;
	pinMode (p2, OUTPUT) ;
	stop_thread = false;
	mthread = new std::thread(&ThreadHelper,this);
}

~Motor()
{
	 stop_thread = true;
	 mthread->join();
}

void Run(int speed)
{
	this->speed = speed;
}
void RunDegrees(int speed, int degrees)
{
	
	int lastcounter = e.Counter();
	Run(speed);
	while(lastcounter - e.Counter() < degrees*2)
	{
		usleep(1000);
		printf("%d %d %d\n", e.Counter(), lastcounter, degrees);
	}
	printf("stopped\n");
	Stop();	
		
	
}

void Stop()
{
	speed = 0;
}

 
void ThreadMain()
{

	while(!stop_thread)
	{
		if (e.Speed() > 300)
		{
			usleep(100);
			//printf("%d\n", e.Speed());
			continue;
			
		}
		
		power = (e.Speed() - speed) * -0.6;
		if (power >= 100)
		{
			power = 100;
			
		} else if (power <= -100)
		{
			power = -100;
		}	
		//printf("P %d\n", power);
	  if (power)
	   {
			if(power > 0)
			{
				int k = power * 100;
				int l = 10000 - k;
				//printf("K %d\n", k);
				//printf("L %d\n", l);
				digitalWrite (p1, HIGH) ; usleep (k) ;
				digitalWrite (p1,  LOW) ; usleep (l) ;
				
			}
			else
			{
				int k = -power * 100;
				int l = 10000 - k;
				//printf("K %d\n", k);
				//printf("L %d\n", l);
				digitalWrite (p2, HIGH) ; usleep (k) ;
				digitalWrite (p2,  LOW) ; usleep (l) ;
			}
			
	   } else {
			digitalWrite (p1, LOW) ; usleep(10000) ;
	   }
	}
}
      	
}; //Motor

void ThreadHelper(Motor* m)
{
	m->ThreadMain();
}



int main (void)
{
  wiringPiSetup () ;
  Motor m1(3, 4, 5, 6);
  m1.RunDegrees(100, 360);
  


  return 0 ;
} 
