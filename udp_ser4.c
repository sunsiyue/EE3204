#include "headsock.h"

void str_ser1(int sockfd, int error_rate);                                                           // transmitting and receiving function

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in my_addr;
	int error_rate = 0;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {			//create socket
		printf("error in socket");
		exit(1);
	}

	if (argc != 2){
		printf("Parameters doesn't match");
		exit(2);
	}

	error_rate = atoi(argv[1]);
	if (error_rate > 100)
		error_rate = 100;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);  //make sin_zero all 0 for 8 byte
	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {           //bind socket
		printf("error in binding");
		exit(1);
	}
	printf("start receiving\n");
	
	while(1){
		str_ser1(sockfd, error_rate);                        // send and receive
	}
	
	close(sockfd);
	exit(0);
}

void str_ser1(int sockfd, int error_rate)
{
	char buf[BUFSIZE];
	FILE *fp;
	char recvs[DATALEN];
	char identifier = '2';
	struct ack_so ack;
	
	int error_index = 0;
	int end = 0, n = 0;

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	bzero(&addr, sizeof(addr));
	
	long lseek=0;
		
	printf("receiving data!\n");

	while(!end)
	{
		if ((n = recvfrom(sockfd, &recvs, DATALEN+1, 0, (struct sockaddr *)&addr, &len)) == -1)                                   //receive the packet
		{
			printf("error when receiving\n");
			//Send NACK to ask the client to resend
			ack.num = 2;
		}

		if (recvs[n-2] == '\0')									//if it is the end of the file
		{
			end = 1;
			n --;
		}
		
		//Received new data
		if (n > 0 && (recvs[n-1] != identifier) && error_index >= error_rate){
			memcpy((buf+lseek), recvs, n-1);
			lseek += n-1;
			identifier = recvs[n-1];
		}
		
		//Send acknowledgement
		//NACK
		if(ack.num == 2 || error_index < error_rate){
			ack.num = 0;
		}
		else
			ack.num = 1;
		
		ack.len = 0;
	
		//Keep retrying
		while ((n = sendto(sockfd, &ack, 2, 0,  (struct sockaddr *)&addr, len)) == -1){
			printf("Error in sending\n");
		}
		
		error_index = (error_index + 1) % 100;
	}
	
	if ((fp = fopen ("myUDPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);

}
