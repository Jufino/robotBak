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
extern "C"{
	#include <bot.h>
	#include "semafor.h"
}
#include <termios.h>
#include <fcntl.h>

#define PORT 1212
#define Wifi 1
#define nahravaj 0

using namespace std;
using namespace cv;
int prekazka = 0;
int prekazka_command = 0;
int prekazka_last = 0;
void quit(char* msg, int retval);
int obchadzaj(int *data,int objekt){
	if(data[2] < 35 && objekt != -1){ 
			if(prekazka == 0){
				if(data[0] >= data[4]){
					prekazka = 1;	
					prekazka_command = 1;
				}
				else{
					prekazka = 2;
					prekazka_command = 2;
				}	
			}
			else{
				if(data[0] < 25 && prekazka == 1){
					if(prekazka_last != 0){
						prekazka = -1;
						prekazka_command = -1;
					}
					else{
						prekazka = 2;
						prekazka_command = 2;
						prekazka_last = 1;
					}
				}
				else if(data[4] < 25 && prekazka == 2){
					if(prekazka_last != 0){
                               			prekazka = -1;
						prekazka_command = -1;
                        		}
                        		else{
                                		prekazka = 1;
						prekazka_command = 1;
                                		prekazka_last = 2;
                       	 		}	
				}
				else if(data[1] < 20 && data[3] < 20) {
                        		prekazka_command = 5;
                		}
				else if(data[3] < 20){
                       	 		prekazka_command = 3;
                		}
                		else if(data[1] < 20){
                        		prekazka_command = 4;
                		}
			
				else if(prekazka_command != 2 && prekazka_command != 1){
					prekazka_command = prekazka;
				}
			}
	}
	else if(data[1] > 40 && data[2] > 40 && data[3] > 40){
		prekazka = 0;
		prekazka_last = 0;
		prekazka_command = 0;
	}
	//printf("%d %d %d %d %d %d obj:%d\n",data[0],data[1],data[2],data[3],data[4],data[5],objekt);
	//printf("%d\n",prekazka_command);
	if(data[0] < 25 && data[4] < 25){
                m_(0,0);
                printf("maly priestor\n");
		return 1;
        }
        else if(data[0] < 17){
                m_(90,180);
                printf("ochrana - vlavo\n");
		return 1;
        }
        else if(data[4] < 17){
                m_(270,180);
                printf("ochrana - vpravo\n");
		return 1;
        }
	else if(data[5] <17){
		m_(0,180);
		printf("ochrana - vzadu\n");
		return 1;
	}
	else if(data[2] < 17){
		m_(180,180);
                printf("ochrana - vpredu\n");
                return 1;
	}
	else{
		switch(prekazka_command){
			case 2:
				m_(90,180);
				printf("vpravo\n");
				return 1;
			case 1:
				m_(270,180);
				printf("vlavo\n");
				return 1;
			case -1:
				m_(0,0);
				printf("stoj\n");
				return 1;		
			case 3: motors(120,120,120,120);
				printf("rotacia v smere\n");
				return 1;
			case 4:	motors(-120,-120,-120,-120);
				printf("rotacia v protismere\n");
				return 1;
			case 5: m_(180,180);
				printf("cuvanie\n");
				return 1;
			default:
			        if(data[1] < 25 && data[4] < 20){
					printf("maly priestor\n");
					m_(0,0);
					return 1;					
				}
				else if(data[3] < 25 && data[0] < 20){
					m_(0,0);
					printf("maly priestor\n");
					return 1;
                                }
				else if(data[1] < 20){//ochrana narazenia
        			        printf("ochrana - vlavo\n");
               			 	m_(90,180);
                			return 1;
        			}
        			else if(data[3] < 20){//ochrana narazenia
               		 		printf("ochrana - vpravo\n");
                			m_(270,180);
                			return 1;
        			}
				else{
					m_(0,0);
					printf("netreba obchadzat\n");
					return 0;
				}
		}	
	}
}
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
int iLowH_green = 25;
int iHighH_green = 40;
int iLowS_green = 0;
int iHighS_green  = 255;
int iLowV_green  = 140;
int iHighV_green  = 255;
int iLowH_orange = 0;
int iHighH_orange = 20;
int iLowS_orange = 100;
int iHighS_orange  = 255;
int iLowV_orange  = 150;
int iHighV_orange  = 255;

void *readKey(void*){
	int option;
	while(zap){
		printf("1. zmena low hue (%d)\n",iLowH_green);
		printf("2. zmena high hue (%d)\n",iHighH_green);
		printf("3. zmena low saturation (%d)\n",iLowS_green);
		printf("4. zmena high saturation (%d)\n",iHighS_green);
		printf("5. zmena low value(%d)\n",iLowV_green);
		printf("6. zmena high value(%d)\n",iHighV_green);
		scanf("%d",&option);
		switch(option){
			case 1:scanf("%d",&iLowH_green); break;
			case 2:scanf("%d",&iHighH_green); break;
			case 3:scanf("%d",&iLowS_green); break;
			case 4:scanf("%d",&iHighS_green); break;
			case 5:scanf("%d",&iLowV_green); break;
			case 6:scanf("%d",&iHighV_green); break;
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
float vzdialenost=-1;	//vzdialenost objektu v cm
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
Mat orange_mask,green_mask,imgOrig,test,imgSend;
void comSocket(){
	char recvdata[30];
		int bytes = recv(clientsock, recvdata, 30, 0);
        	if (bytes == 0){
 	       		printf("Connection should be closed now\n");
               		close(clientsock);
        		if ((clientsock = accept(serversock, NULL, NULL)) == -1) quit("accept() failed", 1);
       	 	}
        	if (strcmp(recvdata, "img") == 0){
 			send_img(clientsock,imgSend,70,true);
        	}
        	else if (strcmp(recvdata, "img1") == 0){
        		send_img(clientsock,orange_mask,70,true);
       	 	}
		else if (strcmp(recvdata, "img2") == 0){
                        send_img(clientsock,green_mask,70,true);
                }
}
float suma =0;

void sigctrl(int param);
void sigpipe(int param);
int main(int argc, char** argv)
{
	int imageChooseMain=0;
	Mat imgHSV;		
        double P = 1.2;
	double I = 0;
	double sum=0;
	double sum_Kalib = 0;
	double e;
	double akcia;
	double max_akcia = 200;
	int data[6];
//	int codec = CV_FOURCC('M', 'J', 'P', 'G');
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
	int index_green;
	int max_green;
	int index_orange;
        int max_orange;
	int index_people=-1;

/*		VideoWriter video("capture.avi",codec, 20, cvSize((int)sirka,(int)vyska) );
		VideoWriter video1("test.avi",codec, 20, cvSize((int)sirka,(int)vyska) );
		// Check if the video was opened
    		if(!video.isOpened() || !video1.isOpened()){
        		printf("problem s otvorenim suboru");
        		return -1;
    		}
*/	
	while(zap) {
		if(waitKey(1) == 27) break;
		getULT(&data[0]);
		//printf("%d %d %d %d %d %d\n",data[0],data[1],data[2],data[3],data[4],data[5]);
		semWait(sem_id,0);
		imageChooseMain = imageChoose;
		semPost(sem_id,0);
		if(imageChooseMain !=0){
			if(imageChooseMain == 1)	imgOrig = cvarrToMat(img1);
			else if(imageChooseMain == 2)   imgOrig = cvarrToMat(img2);
			if(imageChooseMain == 1)        cvarrToMat(img1).copyTo(imgSend);
                        else if(imageChooseMain == 2)   cvarrToMat(img2).copyTo(imgSend);
              		imageChoose = 0;
			cvtColor(imgOrig, imgHSV, COLOR_BGR2HSV);
        		inRange(imgHSV, Scalar(iLowH_green, iLowS_green, iLowV_green), Scalar(iHighH_green, iHighS_green, iHighV_green), green_mask); 
			inRange(imgHSV, Scalar(iLowH_orange, iLowS_orange, iLowV_orange), Scalar(iHighH_orange, iHighS_orange, iHighV_orange), orange_mask);
			unsigned int i=0;  
			unsigned int o=0;
        		vector< vector<Point> > contours_green;  
			vector< vector<Point> > contours_orange;
        		findContours(green_mask, contours_green, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE); //find contours  
			findContours(orange_mask, contours_orange, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE); //find contours
        		vector<double> areas_green(contours_green.size());  
			vector<double> areas_orange(contours_orange.size());
     

			Rect r_orange;
                        Rect r_green;
			max_orange = -1;
                        index_orange = -1;
                        max_green = -1;
			index_green = -1;
			for(i = 0; i < contours_orange.size(); i++){
                                r_orange = boundingRect(contours_orange[i]);
				for(o = 0; o < contours_green.size(); o++){
					r_green = boundingRect(contours_green[o]);
					if (abs(r_green.x+r_green.width/2-r_orange.x-r_orange.width/2)< 60){
						if((r_green.y > r_orange.y && 2*r_orange.y+r_orange.height-r_green.y > -30)||
						   (r_green.y < r_orange.y && 2*r_green.y+r_green.height-r_orange.y > -30)){
						areas_orange[i] = contourArea(Mat(contours_orange[i]));
						areas_green[o] = contourArea(Mat(contours_green[o]));
						if(areas_orange[i] > 300 && areas_green[o] > 300){
							if(areas_orange[i] > max_orange){
								if(areas_green[o] > max_green){
									max_orange  = areas_orange[i];
									max_green = areas_green[o];
									index_orange = i;
									index_green = o;
								}
							}
						}	
						}
					}
                         		       		
				}
                        }
        		if(Wifi == 1 || nahravaj == 1){ 
				drawContours(orange_mask, contours_orange, index_orange, Scalar(255), CV_FILLED);  
				drawContours(green_mask, contours_green, index_green, Scalar(255), CV_FILLED);
			}
			Point center; 
        		if (contours_orange.size() >= 1 && index_orange != -1 && contours_green.size() >=1 && index_green != -1){  
            			r_orange = boundingRect(contours_orange[index_orange]);
				r_green = boundingRect(contours_green[index_green]);
				//printf("obsah_orange  = %d, obsah_green = %d,abs = %d\n",max_orange,max_green,abs(r_green.x+r_green.width/2-r_orange.x-r_orange.width/2));
					center.x = (r_orange.x + (r_orange.width/2)); //+ r_green.x +(r_green.width/2))/2 ;
					if(r_orange.y < r_green.y){ 
 						center.y = r_orange.y + (r_green.y-r_orange.y+r_green.height)/2;
						index_people = 1;
					}
					else{
						center.y = r_green.y + (r_orange.y-r_green.y+r_orange.height)/2;
						index_people = 2;
					}
					vzdialenost = (int)(((1.1-0.292)/tan(0.1359+0.00180333*(240-center.y)))*100);
					if(Wifi == 1 || nahravaj == 1){
						if(r_orange.y < r_green.y)
	    					rectangle(imgSend, Point((r_orange.x+r_green.x)/2,r_orange.y),Point((r_orange.x + r_orange.width + r_green.x +r_green.width)/2,r_green.y + r_green.height), CV_RGB(0, 255, 0), 3, 8, 0); 
						else
						rectangle(imgSend, Point((r_orange.x+r_green.x)/2,r_green.y),Point((r_orange.x + r_orange.width + r_green.x +r_green.width)/2,r_orange.y + r_orange.height), CV_RGB(0, 255, 0), 3, 8, 0);
						circle(imgSend,center,5,Scalar( 0, 0, 255 ),-1,8);
						char text[20];
	                                	sprintf(text, "%d,%d,%2.2f,P%d", center.x,center.y,vzdialenost,index_people);
						putText(imgSend, text, Point(10,20), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,0,255), 1, CV_AA);
           				}
					pozicia_X = center.x;
				}
				else{
					vzdialenost = -1;
					index_people=-1;
				}  
			if(Wifi == 1) comSocket();
/*			if(nahravaj == 1){
				video << imgOrig;
				video1 << test;
			}*/
  		if(obchadzaj(&data[0],vzdialenost) == 0){
			if (vzdialenost > 240 && vzdialenost != -1){
					e = 160-pozicia_X;
					sum= e*I;
					if(sum > 150) sum=100;
					else if(sum < -150) sum = -100;
					akcia = P*e+sum;
					if(akcia > max_akcia) akcia = max_akcia;
					else if(akcia < -max_akcia) akcia = -max_akcia;
					if (akcia < 0)	motors(-(int)(max_akcia+akcia),-(int)(max_akcia),(int)(max_akcia),(int)(max_akcia));
					else		motors(-(int)(max_akcia),-(int)(max_akcia),(int)(max_akcia),(int)(max_akcia-akcia));
			}
			else if(vzdialenost == -1){
				e = 160-pozicia_X;
				if(e > 0){
					motors(-110,-110,-110,-110);
				}
				else{
					motors(110,110,110,110);
				}
			}
			else if(vzdialenost < 200 && (160-pozicia_X < 45 && 160-pozicia_X > -45)){
				if(data[5] > 30) m_(180,180);
				else	         motors(0,0,0,0);
			}
			else {
				//printf("Kalib\n");
				e = 160 - pozicia_X;
				if(e > 70 || e < -70){
					sum_Kalib+= e*0.04;
                                	if(sum_Kalib > 100) sum_Kalib=50;
                                	else if(sum_Kalib < -100) sum_Kalib = -50;
					akcia = e*0.7+sum_Kalib; 
				}
				if(akcia > max_akcia) akcia = 120;
                                else if(akcia < -max_akcia) akcia = -120;
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
}

void quit(char* msg, int retval){
	fprintf(stderr, "%s\n", msg);
	exit(retval);
}

void sigctrl(int param){
  printf("Server sa vypina\n");
  m_(0,0);
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

