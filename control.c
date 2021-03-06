#include <stdio.h>  
#include <unistd.h>  
 
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

#define SETBITSPEED(opt, s)		\
do					\
{					\
	cfsetispeed(&(opt), B##s);	\
	cfsetospeed(&(opt), B##s);	\
} while(0)

#define CMD_PREFIX	0xeeee
#define CMD_SUFFIX	0xdddddddd

union PARA
{
	char c[4];
	long int d;
} para[4];
union CMD
{
	char c[4];
	long int d;
	struct {
		char flag[2];
		char cmd;
		char rev;
	};
} cmd;

char buff[20];
void do_run_cmd(int fd, char buff[20]){
	write(fd, buff, 20);
}

void fill_cmd_head(union CMD *cmd){
	cmd->flag[0] = 0xee;
	cmd->flag[1] = 0xee;
	cmd->rev = 0;
}

void fill_data_tail(union PARA *para){
	para->d = CMD_SUFFIX;
}

void run_cmd(int fd, union CMD *cmd, union PARA para[4]){
	int i, j;
	fill_cmd_head(cmd);

	fill_data_tail(&para[3]);


	memcpy(buff, cmd->c, 4);
	memcpy(buff+4, para, 16);

	for(j = 0; j < 20; j++)
		printf("%x,", buff[j]);

	printf("\n");

	do_run_cmd(fd, buff);
}


void testfunction(int fd){
	union CMD cmd;
	union PARA para[4];
	int nread;
	char buff[20];

	int i, j;
	memset(&cmd, 0, 4);
	memset(&para, 0, 16);

	cmd.flag[0] = 0xee;
	cmd.flag[1] = 0xee;

	cmd.cmd = 'B';
	cmd.rev = 0;

	for(i = 0; i < 3; i++)
		para[i].d = 500;

	para[3].d = CMD_SUFFIX;
	
	memcpy(buff, cmd.c, 4);
	memcpy(buff+4, para, 16);

	for(j = 0; j < 20; j++)
		printf("%x,", buff[j]);

	printf("\n");

	write(fd, buff, 20);
}

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	/* 五个参量 fd打开文件 speed设置波特率 bit数据位设置   neent奇偶校验位 stop停止位 */

	struct termios newtio,oldtio;

	if ( tcgetattr( fd,&oldtio) != 0) { 
		perror("SetupSerial 1");
		return -1;
	}

	bzero( &newtio, sizeof( newtio ) );

	newtio.c_cflag |= CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	switch( nBits )
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}

	switch( nEvent )
	{
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E': 
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;
		case 'N': 
			newtio.c_cflag &= ~PARENB;
			break;
	}

	SETBITSPEED(newtio, 9600);

	if( nStop == 1 )
		newtio.c_cflag &= ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |= CSTOPB;

	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;

	tcflush(fd,TCIOFLUSH);

	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}

/*
	   struct termios options;  // 串口配置结构体
	   tcgetattr(fd,&options); //获取当前设置
	   bzero(&options,sizeof(options));

	   options.c_cflag  |= B9600 | CLOCAL | CREAD; // 设置波特率，本地连接，接收使能
	   options.c_cflag &= ~CSIZE; //屏蔽数据位
	   options.c_cflag  |= CS8; // 数据位为 8 ，CS7 for 7 
	   options.c_cflag &= ~CSTOPB; // 一位停止位， 两位停止为 |= CSTOPB
	   options.c_cflag &= ~PARENB;  // 无校验
	   //options.c_cflag |= PARENB; // 有校验
	   //options.c_cflag &= ~PARODD;// 偶校验
	   //options.c_cflag |= PARODD; // 奇校验

	   options.c_cc[VTIME] = 0; // 等待时间，单位百毫秒 （读）。后有详细说明
	   options.c_cc[VMIN] = 0; // 最小字节数 （读）。后有详细说明
	   tcflush(fd, TCIOFLUSH); // TCIFLUSH刷清输入队列。
	   				//TCOFLUSH刷清输出队列。 
					//TCIOFLUSH刷清输入、输出队列。
	   tcsetattr(fd, TCSANOW, &options); // TCSANOW立即生效；
	   					//TCSADRAIN：Wait until everything has been transmitted；
						//TCSAFLUSH：Flush input and output buffers and make the change

*/

	printf("set done!\n");
	return 0;
}
int open_port(char const *dev)
{
	/* fd 打开串口 comport表示第几个串口 */
	int fd = open(dev, O_RDWR|		//O_RDWR 读写方式打开；
			   O_NOCTTY|		//O_NOCTTY 不允许进程管理串口（不太理解，一般都选上）；
			   O_NDELAY);		//O_NDELAY 非阻塞（默认为阻塞，打开后也可以使用fcntl()重新设置）

	if (-1 == fd){
		perror("Can't Open Serial Port");
		return(-1);
	}
	else 
		printf("open %s .....\n", dev);

	if(fcntl(fd, F_SETFL, 0) < 0)
		printf("fcntl failed!\n");
	else
		printf("fcntl=%d\n", fcntl(fd, F_SETFL,0));

	if(isatty(STDIN_FILENO)==0)
		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");

	printf("fd-open=%d\n",fd);

	return fd;
}
int main(int argc, char **argv)  
{  
	int ch;  
	char buff[1024] = {0};
	int nread,i;
	char *dev[]={"/dev/ttyUSB0"};
	int fd; //For serial device

	memset(&cmd, 0, 4);
	memset(para, 0, 16);


	///////////////////////
	// Command options
	//
	opterr = 0;  
	while ((ch = getopt(argc, argv, "m:x:y:z:l:")) != -1)  
	{  
		switch(ch)  
		{  
			case 'm':  
				cmd.cmd = optarg[0];
				break;	
			case 'x':  
				para[0].d = atoi(optarg);
				break;	
			case 'y':  
				para[1].d = atoi(optarg);
				break;	
			case 'z':  
				para[2].d = atoi(optarg);
				break;	
			case 'l':  
				sscanf(optarg, "%c,%ld,%ld,%ld",
						&cmd.cmd, &para[0].d, &para[1].d, &para[2].d);
				break;	
			case '?':
			default:
				printf("Here should print USAGE\n");  
		}  
		printf("opt [%c] with optopt: %s\n", ch, optarg);	
	}
	//////////////////////


	//////////////////////////
	// Open serial port
	// By default open ttyUSB0
	//
	if((fd = open_port(dev[0])) < 0){
		perror("open_port error");
		return -1;
	}

	//////////////////////////
	// Set serial port
	//
	if((i=set_opt(fd,9600,8,'N',1))<0){
		perror("set_opt error");
		return -2;
	}


	run_cmd(fd, &cmd, para);
	//testfunction(fd);	

	close(fd);

	return 0;
}
