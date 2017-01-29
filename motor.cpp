#include <wiringPi.h>
#include <thread> 
#include <mutex>
#include <atomic>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

bool quit = false;
void sig_handler(int signum)
{
    quit = true;
}

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
int counter = 0;

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
//		Read();
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

int Read()
{
	
	printf("%d/5 %d/6 %d/10 %d/11\n", digitalRead(5),
	digitalRead(6), digitalRead(10), digitalRead(11));	
}
      	
}; //Encoder

void ThreadHelperE(Encoder* e)
{
	e->Thread();
}



class Motor
{
bool loop = false;
int lastcounter = 0;
int degrees = 0;
int p1;
int p2;
int power = 1;
int speed = 0;

std::thread* mthread;
volatile bool stop_thread = false;

public:

Encoder e;

Motor(int a, int b, int p1, int p2) : e(p1, p2)

{
	this->p1 = a;
	this->p2 = b;
	digitalWrite (a, LOW);
	digitalWrite (b, LOW);
	pinMode (a, OUTPUT);
	pinMode (b, OUTPUT);
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
void RunDegrees(int s, int d)
{
	loop = true;
	lastcounter = e.Counter();
	speed = s;
	degrees = d;
		
	
}

void Stop()
{
	speed = 0;
}

bool Stopped()
{
	return !loop;
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
		//printf("%d  %d  T=%d\n", lastcounter, e.Counter(), lastcounter - e.Counter());
		if ( (((lastcounter - e.Counter() > degrees*2) && (speed > 0)) ||
		      ((lastcounter - e.Counter() < -degrees*2) && (speed < 0)))
			&& loop)
		{
			speed = 0;
			printf("Stop\n");
			loop = false;
		}
		
		power = (e.Speed() - speed) * -0.6;
		//printf("P%d, E%d S%d\n", power, e.Speed(), speed);
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



int main(int argc, char *argv[])
{
  wiringPiSetup () ;
  Motor m1(3, 4, 5, 6);
  Motor m2(2, 1, 10, 11);
  
  
	 signal(SIGINT, sig_handler);
	 int sockfd, newsockfd, portno;
	 socklen_t clilen;
	 char buffer[256];
	 struct sockaddr_in serv_addr, cli_addr;
	 int n;
	 if (argc < 2) {
		 fprintf(stderr,"ERROR, no port provided\n");
		 exit(1);
	 }
	 sockfd = socket(AF_INET, SOCK_STREAM, 0);
	 if (sockfd < 0) 
		error("ERROR opening socket");
	 bzero((char *) &serv_addr, sizeof(serv_addr));
	 portno = atoi(argv[1]);
	 serv_addr.sin_family = AF_INET;
	 serv_addr.sin_addr.s_addr = INADDR_ANY;
	 serv_addr.sin_port = htons(portno);
	 if (bind(sockfd, (struct sockaddr *) &serv_addr,
			  sizeof(serv_addr)) < 0) 
			  error("ERROR on binding");
	 listen(sockfd,5);
	 clilen = sizeof(cli_addr);
	 newsockfd = accept(sockfd, 
				 (struct sockaddr *) &cli_addr, 
				 &clilen);
	 if (newsockfd < 0) 
		  error("ERROR on accept");
	 while(!quit)
	 {
		 bzero(buffer,256);
		 n = read(newsockfd,buffer,255);
		 if (n < 0) error("ERROR reading from socket");
		 printf("%s",buffer);



		std::string command = std::string(buffer, strlen(buffer));
		
		if(command.find("m1") != std::string::npos)
		{
			std::string speed = command.substr (3);
			int speed1 = std::stoi(speed);
			printf("Motor 1 %d\n", speed1);
			m1.Run(speed1);
		}
		
		if(command.find("m2") != std::string::npos)
		{
			std::string speed = command.substr (3);
			int speed1 = std::stoi(speed);
			printf("Motor 2 %d\n", speed1);
			m2.Run(speed1);
		}
		
		if(command.find("mA") != std::string::npos)
		{
			std::string speed = command.substr (3);
			int speed1 = std::stoi(speed);
			printf("Motor All %d\n", speed1);
			m2.Run(speed1);
			m1.Run(speed1);
		}
		
		
		
		
		 if (n < 0) 
		 {
			printf("ERROR writing to socket");
			break;
		}
	 }
	 close(newsockfd);
	 close(sockfd);
	 return 0;
} 
