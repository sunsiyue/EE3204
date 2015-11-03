#include "headsock.h"

float str_cli1(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len, int *num_of_errors, int *total);                
int  send_data(char data[DATALEN+1], int len, int sockfd, struct sockaddr *addr, int addrlen);
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in

int main(int argc, char *argv[])
{
	int len, sockfd;
	struct sockaddr_in ser_addr;
	char **pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	int num_of_errors = 0;
	int total = 0;
	FILE *fp;

	float ti, rt;
	
	if (argc!= 2)
	{
		printf("parameters not match.");
		exit(0);
	}

	if ((sh=gethostbyname(argv[1]))==NULL) {             //get host's information
		printf("error when gethostbyname");
		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);             //create socket
	if (sockfd<0)
	{
		printf("error in socket");
		exit(1);
	}

	addrs = (struct in_addr **)sh->h_addr_list;
	printf("canonical name: %s\n", sh->h_name);
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);			//printf socket information
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(MYUDP_PORT);
	//copy the first address
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}
	
	//while(1){
	ti = str_cli1(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len, &num_of_errors, &total);   // receive and send
	//}
	
	rt = (len/(float)ti);                                         //caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s) number of errors %d total %d\n", ti, (int)len, rt, num_of_errors, total);
	
	fclose(fp);
	close(sockfd);
	exit(0);
}

float str_cli1(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len, int *num_of_errors, int *total)
{
	
	char *buf;
	long lsize, ci;
	char sends[DATALEN+1];
	int slen;
	int code;
	
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	
	ci = 0;

	char identifier = '1';

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

	// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  	// copy the file into the buffer.
	fread (buf,1,lsize,fp);
	
	/*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';									//append the end byte
	
	gettimeofday(&sendt, NULL);							//get the current time
	
	while(ci<= lsize)
	{
		if ((lsize+1-ci) <= DATALEN)
			slen = lsize+1-ci;
		else 
			slen = DATALEN;
			
		memcpy(sends, (buf+ci), slen);
		sends[slen] = identifier;
		
		code = send_data(sends, slen+1, sockfd, addr, addrlen);
		*total = *total + 1;
		
		while (code != 0){
			*num_of_errors = *num_of_errors + 1;
			if (code == 1){
				code = send_data(sends, slen+1, sockfd, addr, addrlen);
				*total = *total +1;
			}
			else{
				exit(2);
			}
		}
		
		//printf("Received acknowledgement\n");
		
		ci += slen;
		//printf("Sent %ld\n", ci);
		
		if (identifier == '1'){
			identifier = '0';
		}
		else{
			identifier = '1';
		}
	}
	
	*len= ci;                                                      
	gettimeofday(&recvt, NULL);
	tv_sub(&recvt, &sendt);                                                                 // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

//0 OK
//1 resend
//2 exit
int  send_data(char data[DATALEN+1], int len, int sockfd, struct sockaddr *addr, int addrlen){
	
	struct ack_so ack;
	struct sockaddr_in addrR;
	socklen_t lenR;
	int n;
	int count = 0;
	
	//Keep retrying
	//force stop after retry several times as server may already end
	while ((n = sendto(sockfd, data, len, 0, addr, addrlen)) == -1){
		printf("send error!");								//send the data
		
		count ++;
		if (count > 100) return 2;
	}
	
	//Waiting for acknowledgement -- resend will happen here
	if ((n=recvfrom(sockfd, &ack, 2, 0, (struct sockaddr *)&addrR, &lenR)) == -1) {      //receive the packet
		printf("error receiving");
		return 1;
	}
	
	if (ack.num == 1 && ack.len == 0){
		return 0;
	}
		
	//NACK
	else if(ack.num == 0 && ack.len == 0){
		return 1;		
	} 
		
	else{
		return 1;		
	}
	
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
