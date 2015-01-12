#include <opencv2/opencv.hpp>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>                                                         
#include <sys/socket.h>                                                         
#include <arpa/inet.h>  

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <iostream>

#define SDA 3
#define SCL 2
extern "C"
{
	#include <bot.h>
	#include "semafor.h"
}
#include <termios.h>
#include <fcntl.h>

#define PORT 1212
#define Wifi 1

using namespace std;
using namespace cv;

void quit(char* msg, int retval);

int serversock,clientsock; 
int zap = 1;
void send_img(int socket, Mat img,int kvalita,bool ok)
{
	if(ok == true){		
		vector<uchar> buff;
		vector<int> param = vector<int>(2);
        	param[0] = CV_IMWRITE_JPEG_QUALITY;
        	param[1] = kvalita;
		imencode(".jpg", img, buff, param);
		char len[10];
		sprintf(len, "%.8d", buff.size());
		send(socket, len, strlen(len), 0);
		send(socket, &buff[0], buff.size(), 0);
		buff.clear();
	}
	else{
                char len[10];
                sprintf(len, "%.8d", -1);
		send(socket, len, strlen(len), 0);
	}
}
int iLowH = 0;
int iHighH = 22;
int iLowS = 100;
int iHighS = 255;
int iLowV = 150;
int iHighV = 255;
void *readKey(void*){
	int option;
	while(zap){
		printf("1. zmena low hue (%d)\n",iLowH);
		printf("2. zmena high hue (%d)\n",iHighH);
		printf("3. zmena low saturation (%d)\n",iLowS);
		printf("4. zmena high saturation (%d)\n",iHighS);
		printf("5. zmena low value(%d)\n",iLowV);
		printf("6. zmena high value(%d)\n",iHighV);
		scanf("%d",&option);
		switch(option){
			case 1:scanf("%d",&iLowH); break;
			case 2:scanf("%d",&iHighH); break;
			case 3:scanf("%d",&iLowS); break;
			case 4:scanf("%d",&iHighS); break;
			case 5:scanf("%d",&iLowV); break;
			case 6:scanf("%d",&iHighV); break;
		}
	}
}
IplImage *img1;
IplImage *img2;
char imageChoose=0;
int sem_id=0;
CvCapture* camera;
struct timespec tstart={0,0}, tend={0,0};
void start(){
	clock_gettime(CLOCK_MONOTONIC, &tstart);
}
void koniec(char a){
	clock_gettime(CLOCK_MONOTONIC, &tend);
        if(a==1)printf("fps %.5f\n",1/(((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec)));
	else	printf("cas %.5f s\n",((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));
}
int vzdialenost=-1;	//vzdialenost objektu v cm
int pozicia_X=0;	//pozicia 0 az sirka
#define vyska 240
#define sirka 320
void *getImg(void *arg){
	while(zap){
		img1 = cvQueryFrame(camera);
		semWait(sem_id,0);
		imageChoose = 1;
                semPost(sem_id,0);
		img2 = cvQueryFrame(camera);
		semWait(sem_id,0);
                imageChoose = 2;
                semPost(sem_id,0);
	}
}
Mat test,imgOrig;
void comSocket(){
	char recvdata[30];
		int bytes = recv(clientsock, recvdata, 30, 0);
        	if (bytes == 0){
 	       		printf("Connection should be closed now\n");
               		close(clientsock);
        		if ((clientsock = accept(serversock, NULL, NULL)) == -1) quit("accept() failed", 1);
       	 	}
        	if (strcmp(recvdata, "img") == 0){
 			send_img(clientsock,imgOrig,70,true);
        	}
        	if (strcmp(recvdata, "img1") == 0){
        		send_img(clientsock,test,70,true);
       	 	}
}
float suma =0;

void sigctrl(int param);
void sigpipe(int param);
int main(int argc, char** argv)
{
	int index_last = -1;
	int imageChooseMain=0;
	Mat imgHSV;	
	Mat imgThresholded;
	struct timespec tstart={0,0}, tend={0,0};	
        double P = 1.2;
	double I = 0;
	double sum=0;
	double sum_Kalib = 0;
	double e;
	double akcia;
	double max_akcia = 200;
	camera = cvCaptureFromCAM(0);
	cvSetCaptureProperty( camera, CV_CAP_PROP_FRAME_WIDTH, sirka);
	cvSetCaptureProperty( camera, CV_CAP_PROP_FRAME_HEIGHT, vyska);
	initRobot();
	
	if(Wifi == 1){
	        struct sockaddr_in server;	
   	        if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 		quit("socket() failed", 1);
		memset(&server, 0, sizeof(server));                                     
		server.sin_family = AF_INET;                                            
		server.sin_port = htons(PORT);
		server.sin_addr.s_addr = INADDR_ANY;                                   
		if (bind(serversock, (struct sockaddr *)&server, sizeof(server)) == -1) quit("bind() failed", 1);                                       
                if (listen(serversock, 10) == -1) 					quit("listen() failed.", 1);                                    
		printf("Waiting for connection on port %d\n", PORT);
		if ((clientsock = accept(serversock, NULL, NULL)) == -1) 		quit("accept() failed", 1);                                     
		signal(SIGPIPE, sigpipe);

	}        
	printf("Connection ok\n");

	sem_id = semCreate(getpid(),1);   //vytvor semafor
        semInit(sem_id,0,1);
	semInit(sem_id,1,1);

	pthread_t vlaknoKey;
      pthread_create(&vlaknoKey,NULL,&readKey,NULL);
	signal(SIGINT, sigctrl);
	pthread_t vlaknoImg;
        pthread_create(&vlaknoImg,NULL,&getImg,NULL);
	int index;
	int max;
	while(zap) {
		semWait(sem_id,0);
		imageChooseMain = imageChoose;
		semPost(sem_id,0);
		if(imageChooseMain !=0){
			if(imageChooseMain == 1)	imgOrig = cvarrToMat(img1);
			else if(imageChooseMain == 2)   imgOrig = cvarrToMat(img2);
              		imageChoose = 0;
			cvtColor(imgOrig, imgHSV, COLOR_BGR2HSV);
        		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), test); 
      		 
        		int i=0;  
        		vector< vector<Point> > contours;  
        		findContours(test, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE); //find contours  
        		vector<double> areas(contours.size());  
     
        		max = -1;
			index = -1;
			for(i = 0; i < contours.size(); i++){  
            			areas[i] = contourArea(Mat(contours[i]));  
	    			if (areas[i] > max){
					max = areas[i];
					index = i;
	    			}
        		}  
        		if(Wifi == 1) drawContours(test, contours, index, Scalar(255), CV_FILLED);  
			Rect r;
			Point center; 
        		if (contours.size() >= 1 && index != -1){  
            			r = boundingRect(contours[index]);
	    			if(max>2500){
					center.x = r.x + (r.width/2);
                                        center.y = r.y+ r.height;
					vzdialenost = 1.1312318*center.y+3.21945;
					if(Wifi == 1){
	    					rectangle(imgOrig, Point(r.x,r.y),Point(r.x+r.width,r.y+r.height), CV_RGB(0, 255, 0), 3, 8, 0); 
						circle(imgOrig,center,5,Scalar( 0, 0, 255 ),-1,8);
						char text[20];
	                                	sprintf(text, "%d,%d,%d", center.x,center.y,vzdialenost);
						putText(imgOrig, text, Point(center.x+2,center.y), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255,0,0), 1, CV_AA);  
           				}
					pozicia_X = center.x;
				}
				else{
					vzdialenost = -1;
				}  
			}
			if(Wifi == 1) comSocket();
			if (vzdialenost > 120 && vzdialenost != -1){
				e = 160-pozicia_X;
				sum+= e*I;
				if(sum > 150) sum=100;
				else if(sum < -150) sum = -100;
				akcia = P*e+sum;
				if(akcia > max_akcia) akcia = max_akcia;
				else if(akcia < -max_akcia) akcia = -max_akcia;
				if (akcia < 0){
					motors(-(int)(max_akcia+akcia),-(int)(max_akcia),(int)(max_akcia),(int)(max_akcia));
				}
				else{
					motors(-(int)(max_akcia),-(int)(max_akcia),(int)(max_akcia),(int)(max_akcia-akcia));
				}
			}
			else if(vzdialenost == -1){
				e = 160-pozicia_X;
				if(e > 0){
					motors(-120,-120,-120,-120);
				}
				else{
					motors(120,120,120,120);
				}
			}
			else if(vzdialenost < 70){
				motors(150,150,-150,-150);
			}
			else {
				e = 160 - pozicia_X;
				if(e > 70 || e < -70){
					sum_Kalib+= e*0.035;
                                	if(sum_Kalib > 100) sum_Kalib=50;
                                	else if(sum_Kalib < -100) sum_Kalib = -50;
					akcia = e*0.7+sum_Kalib; 
				}
				if(akcia > max_akcia) akcia = 110;
                                else if(akcia < -max_akcia) akcia = -110;
				if(e > 70){
                                        motors(-(int)akcia,-(int)akcia,-(int)akcia,-(int)akcia);
                                }
                                else if (e < -70){
                                        motors(-(int)akcia,-(int)akcia,-(int)akcia,-(int)akcia);
                                }
				else{
					sum_Kalib=0;
					motors(0,0,0,0);
				}
			}
		}
	}
}

void quit(char* msg, int retval){
	fprintf(stderr, "%s\n", msg);
	exit(retval);
}

void sigctrl(int param){
  printf("Server sa vypina\n");
  zap = 0;
  sleep(2);
  semRem(sem_id); 
  if(Wifi == 1){
	close(serversock);
        close(clientsock);
  }
  exit(0);
  
}
void sigpipe(int param){
	if(Wifi == 1){
		printf("Client sa odpojil\n");
  		semRem(sem_id);
                close(serversock);
                close(clientsock);
		exit(0);
	}
}

