#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SHELLSCRIPT "\
#/bin/bash \n\
sudo chmod 777 /dev/adxl* \n\
"

int main()
{
	int  read_buf_x;
	int  read_buf_y;
	int  read_buf_z;
        int adxl_x,adxl_y,adxl_z;
        char option;
	printf("shell script to provide execute permission to device files....\n");
	puts(SHELLSCRIPT);	
	system(SHELLSCRIPT);
        printf("\n*******Accelerometer output*******\n");

       /* adxl_x = open("/dev/adxl_x", O_RDWR);
	if(adxl_x<0){printf("file adxl_x cannot be opened\n");exit(-1);}
	adxl_y = open("/dev/adxl_y", O_RDWR);
	if(adxl_y<0){printf("file adxl_y cannot be opened\n");exit(-1);}
	adxl_z = open("/dev/adxl_z", O_RDWR);
	if(adxl_z<0){printf("file adxl_z cannot be opened\n");exit(-1);}
        read(adxl_x,read_buf_x,2);
	read(adxl_y,read_buf_y,2);
	read(adxl_z,read_buf_z,2);
	close(adxl_x);
	close(adxl_y);
	close(adxl_z);*/

        while(1) {
                printf("****Please Enter the Option******\n");
                printf("        x. read adxl_x               \n");
                printf("        y. read adxl_y               \n");
                printf("        z. read adxl_z               \n");
		printf("	e. exit			     \n");
                printf("*********************************\n");
                scanf(" %c", &option);
                printf("Your Option = %c\n", option);
                
                switch(option) {
                        case 'x':
				adxl_x = open("/dev/adxl_x", O_RDWR);
				if(adxl_x<0){printf("file adxl_x cannot be opened\n");exit(-1);}
				read(adxl_x,&read_buf_x,4);
				close(adxl_x);
                                printf("adxl_x = %u \n\n", read_buf_x);
				printf("Done!\n\n");		
                                break;
                        case 'y':
				adxl_y = open("/dev/adxl_y", O_RDWR);
				if(adxl_y<0){printf("file adxl_y cannot be opened\n");exit(-1);}
				read(adxl_y,&read_buf_y,4);
				close(adxl_y);
				printf("adxl_y = %u \n\n", read_buf_y);
                                printf("Done!\n\n");
				break;
			case 'z':
				adxl_z = open("/dev/adxl_z", O_RDWR);
				if(adxl_z<0){printf("file adxl_z cannot be opened\n");exit(-1);}
				read(adxl_z,&read_buf_z,4);
				close(adxl_z);
				printf("adxl_z = %u \n\n", read_buf_z);
                                printf("Done!\n\n");
				break;
                        case 'e':      
				close(adxl_x);close(adxl_y);close(adxl_z);
                                exit(1);
                                break;
                        default:
                                printf("Enter Valid option = %c\n",option);
                                break;
                }
        }
        return 0;
}
