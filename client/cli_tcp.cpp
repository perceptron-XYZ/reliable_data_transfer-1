char* getmessage(char *);

/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winsock.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <windows.h>
#include <sys/stat.h>
#include <fstream>
#include <time.h>
using namespace std;

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0);
#define WINDOW_SIZE 8;
int WSIZE = WINDOW_SIZE;
#define SEQ_MAX 8;
//user defined port number
#define CLIENT_PORT 5000;
#define ROUTER_PORT 7000;
int port=CLIENT_PORT;
int rport = ROUTER_PORT;

//socket data types
SOCKET s;
SOCKADDR_IN sa;         // filled by bind
SOCKADDR_IN sa_in;      // fill with server info, IP, port
fd_set readfds; //fd_set is a type
fd_set readset;
socklen_t length = sizeof(sa_in);

//buffer data types
char szbuffer[128];
char *buffer;
int ibufferlen=0;
int ibytessent;
int ibytesrecv=0;
int outfds = 0;


//host data types
HOSTENT *hp;
HOSTENT *rp;

char localhost[11],
     remotehost[11];

// data for control packet
typedef enum { CLIENT_REQ = 1, ACK_CLIENT, ACK_SERVER, FILE_NOT_EXIST, INVALID,LAST,INITIAL,DUP } HandshakeType;
char command[128];
char filename[128];
bool isCommandValid = false;
const int FILE_NAME_SIZE = 128;
const int MESSAGE_MAX_LENGTH = 128;
struct THREE_WAY_HS{
	int client_number;
	int server_number;
	char direction[11];
	char file_name[FILE_NAME_SIZE];
	int file_size;
	HandshakeType type;
	char newName[20];
} handshake;
int f_size;
//message frame
#define STOPNWAIT 1 //Bits width For stop and wait
const int MAX_SIZE = 80;
const int N_WINDOW = WINDOW_SIZE;
struct MESSAGE_FRAME {
	unsigned char header;
	unsigned int snwseq;
	char data[MAX_SIZE];//or error message
	int data_length;
} message_frame;
#define PACKET_SIZE 80
//const int psize = PACKET_SIZE;
int psize = 128;
struct packet{
	int type;
	int seq;
	int size;
	char data[128];
	int seqnum;
};
char confirmedFilename[20];
int ack;
int ack2;
//other

HANDLE test;

DWORD dwtest;


//reference for used structures

/*  * Host structure

    struct  hostent {
    char    FAR * h_name;             official name of host *
    char    FAR * FAR * h_aliases;    alias list *
    short   h_addrtype;               host address type *
    short   h_length;                 length of address *
    char    FAR * FAR * h_addr_list;  list of addresses *
#define h_addr  h_addr_list[0]            address, for backward compat *
};

 * Socket address structure

 struct sockaddr_in {
 short   sin_family;
 u_short sin_port;
 struct  in_addr sin_addr;
 char    sin_zero[8];
 }; */
void printPkt(struct packet pkt, int io) {
	if (io == 0){
		if (pkt.type == 1)
			printf("RECV DATA:\t  Seq %d\t Size %d\n",
			pkt.seqnum, pkt.size);
		else if (pkt.type == 2)
			printf("RECV ACK:\t   Seq %d\t\n",
			pkt.seqnum);
	}
	else{
		if (pkt.type == 1)
			printf("SENT DATA:\t   Seq %d\t Size %d\n",
			pkt.seqnum, pkt.size);
		else if (pkt.type == 2)
			printf("SENT ACK:\t   Seq %d\t\n",
			pkt.seqnum);

	}
	return;
}

void error(char *msg) {
	perror(msg);
	exit(0);
}
bool isFileExist(char* filename){
	struct stat results;
	stat(filename, &results);
	int size = results.st_size;
	return size > 0;
}
int getSize(char* filename){
	struct stat results;
	stat(filename, &results);
	int size = results.st_size;
	return size;
}
bool getHandshakeResponse(int sock,const timeval timeout)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	int bytes_recvd;
	int outfds = select(1, &readfds, NULL, NULL, &timeout);
	if (outfds == 0){
		return true;
	}
	else if (outfds == 1){
		bytes_recvd = recvfrom(sock, (char *)&handshake, sizeof(handshake), 0, (struct sockaddr*)&sa_in, &length);
		return false;
	}
	else{
		return true;
	}
}
bool getFrame(int sock, const timeval timeout){
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	int bytes_recvd;
	int outfds = select(1, &readfds, NULL, NULL, &timeout);
	if (outfds == 0){
		return true;
	}
	else if (outfds == 1){
		bytes_recvd = recvfrom(sock, (char *)&message_frame, sizeof(message_frame), 0, (struct sockaddr*)&sa_in, &length);
		return false;
	}
	else{
		return true;
	}
}
bool getACK(int sock, const timeval timeout)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	int bytes_recvd;
	int outfds = select(1, &readfds, NULL, NULL, &timeout);
	if (outfds == 0){
		return true;
	}
	else if (outfds == 1){
		bytes_recvd = recvfrom(sock, (char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa_in, &length);
		return false;
	}
	else{
		return true;
	}
}
bool getACK2(int sock, const timeval timeout)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	int bytes_recvd;
	int outfds = select(1, &readfds, NULL, NULL, &timeout);
	if (outfds == 0){
		return true;
	}
	else if (outfds == 1){
		bytes_recvd = recvfrom(sock, (char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa_in, &length);
		return false;
	}
	else{
		return true;
	}
}
void resetHandshake(){
	handshake.client_number = 0;
	handshake.server_number = 0;
	sprintf_s(handshake.direction, "");
	sprintf_s(handshake.file_name, "");
	handshake.file_size = 0;
	handshake.type = INITIAL;
}
int main(void){
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 300000;
	const struct timeval *tp = &timeout;
	WSADATA wsadata;

	try {

		if (WSAStartup(0x0202,&wsadata)!=0){  
			cout<<"Error in starting WSAStartup()" << endl;
		} else {
			buffer="WSAStartup was successful\n";   
			WriteFile(test,buffer,sizeof(buffer),&dwtest,NULL); 

		
		}  


		//Display name of local host.

		gethostname(localhost,10);
		cout<<"Local host name is \"" << localhost << "\"" << endl;

		if((hp=gethostbyname(localhost)) == NULL) 
			throw "gethostbyname failed\n";

		while (1){
			//Create the socket
			if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
				throw "Socket failed\n";
			memset(&sa, 0, sizeof(sa));
			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);
			sa.sin_addr.s_addr = htonl(INADDR_ANY); //host to network
			//bind the port to the socket
			if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR)
				throw "can't bind the socket";
			cout << "bind was succesful" << endl;
			resetHandshake();
			message_frame.header = 'i';
			cout << "Enter Router Hostname:" << flush ;   
			cin >> remotehost ;
			if (strcmp(remotehost, "quit") == 0)
				break;
			//cout << "Remote host name is: \"" << remotehost << "\"" << endl;
			if((rp=gethostbyname(remotehost)) == NULL)
				throw "remote gethostbyname failed\n";
			
			//Specify server address for client to connect to server.
			memset(&sa_in,0,sizeof(sa_in));
			memcpy(&sa_in.sin_addr,rp->h_addr,rp->h_length);
			sa_in.sin_family = rp->h_addrtype;   
			sa_in.sin_port = htons(rport);


			cout << "Enter command :" << flush;
			cin >> command;
			if (strcmp(command, "list") == 0){
				sprintf_s(filename, "");
			}
			else{
				cout << "Enter filename:" << flush;
				cin >> filename;
			}	
			sprintf_s(handshake.direction, command);
			sprintf_s(handshake.file_name, filename);
			handshake.type=CLIENT_REQ;
			handshake.client_number = rand();

			if (strcmp(command, "put") == 0){
				if (!isFileExist(handshake.file_name)){
					cout << "File not found on local." << endl;
					closesocket(s);
					continue;
				}				
				else{
					sprintf_s(handshake.direction, "put");
					handshake.file_size = getSize(handshake.file_name);
					f_size = handshake.file_size;
				}
					
			}
			else if (strcmp(command, "get") == 0){
				char newName[20];
				char choice;
				bool exit = false;
				sprintf_s(newName, handshake.file_name);
				while (isFileExist(newName)){
					cout << "Name conflict. Do you want to 1 -rename, 2 - overwrite or 3- cancel?[1|2|3]" << endl;
					cin >> choice;
					if (choice == '1'){
						cout << "Enter new name:";
						cin >> newName;
						sprintf_s(confirmedFilename, newName);
					}
					else if(choice == '2'){
						sprintf_s(confirmedFilename, handshake.file_name);
						break;
					}
					else if (choice == '3'){
						exit = true;
						closesocket(s);
						break;
					}
					
				}if (exit)
					continue;
				sprintf_s(handshake.direction, "get");
			}
			else if (strcmp(command, "list") == 0){
				sprintf_s(handshake.direction, "list");
			}
			else if (strcmp(command, "rename") == 0){
				char rename;
				char newName[20];
				cout << "CONFIRM:Are you sure you want to change the file name?[Y|N]" << endl;
				cin >> rename;
				if (rename == 'Y'){
					cout << "Enter the new name:" << endl;
					cin >> newName;
					sprintf_s(handshake.direction, "rename");
					sprintf_s(handshake.newName, newName);
				}else
					continue;			
			}
			else if (strcmp(command, "delete") == 0){
				char isDelete;
				cout << "CONFIRM: Are you sure you want to delete file" << handshake.file_name <<"?[Y|N]"<< endl;
				cin >> isDelete;
				if (isDelete == 'Y'){
					sprintf_s(handshake.direction, "delete");
				}else
					continue;
				
			}
			else{
				cout << "Invalid direction." << endl;
				closesocket(s);
				continue;
			}

			
			do{
				//first way. client sends client number to server.
				ibytessent=0;  
				//handshake.type = CLIENT_REQ;
				cout << "Client: Sending client number " << handshake.client_number << endl;
				
				ibytessent = sendto(s,(const char *)&handshake,sizeof(handshake),0,(struct sockaddr*)&sa_in,sizeof(sa_in) );
				//cout << handshake.direction << endl;
				if (ibytessent == SOCKET_ERROR)
					throw "Send failed\n";  
			} while (getHandshakeResponse(s,*tp));
			f_size = handshake.file_size;
			cout << "received acksyn" << endl;
			if (handshake.type==FILE_NOT_EXIST)
				cout << "Requested File Not Found On Server." << endl;
			else if (handshake.type==ACK_CLIENT){
			
				cout << "Client received client number ack from server : " << handshake.client_number << ". And server number: " << handshake.server_number << endl;
				handshake.type=ACK_SERVER;
				int sequence_number = handshake.server_number % SEQ_MAX;
				// send the ack to server.
				if (strcmp(handshake.direction, "get") == 0 || strcmp(handshake.direction, "list") == 0){
					do{
						ibytessent = sendto(s, (const char *)&handshake, sizeof(handshake), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
					} while (getFrame(s, *tp));
				}
				else if (strcmp(handshake.direction, "put") == 0){
					ibytessent = sendto(s, (const char *)&handshake, sizeof(handshake), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
				}
				//cout << handshake.type << endl;
				if (ibytessent == SOCKET_ERROR)
					throw "Send failed\n";
				else
					cout << "Client finished handshaking with server." << endl;
				
			// -----------------------------handshake completes here------------------------------------------
				
				if (strcmp(handshake.direction, "get") == 0){
					int curr = 0;
					int startSeq = handshake.client_number;
					int maxSeq = WSIZE + 2;
					FILE* file = fopen(confirmedFilename, "wb");
					struct packet in, out;
					// prepare ACK template
					bzero((char *)&out, sizeof(out));
					out.type = 2; // ACK packet
					out.seq = curr - 1;
					out.size = 0;

					// set random seed
					srand(time(NULL));

					// gather responses
					while (1) {
						if (recvfrom(s, (char*)&in, sizeof(in), 0,
							(struct sockaddr*) &sa_in, &length) < 0)
							printf("Packet lost\n");
						else {


							// check packet number and type
							if (in.seq > curr) {
								printf("IGNORE %d: expected %d\n", in.seqnum, (curr+1)%maxSeq);
								continue;
							}
							else if (in.seq < curr) {
								printf("IGNORE %d: expected %d\n", in.seqnum, (curr+1)%maxSeq);
								out.seq = in.seq; // send ACK anyways
								out.seqnum = in.seqnum;
							}
							else {
								if (in.type == 3) {
									// FIN signal received
									break;
								}
								else if (in.type != 1) {
									printf("IGNORE %d: not a data packet\n", in.seqnum);
									continue;
								}

								// only write for expected data packet
								fwrite(in.data, 1, in.size, file);
								out.seq = curr;
								out.seqnum =in.seqnum;
								curr++;
							}

							if (sendto(s, (char*)&out, sizeof(out), 0,
								(struct sockaddr*) &sa_in, sizeof(sa_in)) < 0)
								error("ERROR sending ACK\n");
							printPkt(out, 1);
						}
					}

					// send FIN ACK
					bzero((char *)&out, sizeof(out));
					out.type = 3; // FIN packet
					out.seq = curr;
					out.seqnum = in.seqnum;
					out.size = 0;
					if (sendto(s, (char*)&out, sizeof(out), 0,
						(struct sockaddr*) &sa_in, sizeof(sa_in)) < 0)
						error("ERROR sending FIN\n");
					printPkt(out, 1);

					printf("Exiting client\n");
					fclose(file);
				} 
				else if (strcmp(handshake.direction, "put")==0){
					int startSeq = handshake.server_number;
					int maxSeq = WSIZE + 2;
					FILE *file = fopen(filename, "rb");
					struct stat st;
					stat(filename, &st);
					int total = st.st_size / psize;
					if (st.st_size % psize > 0)
						total++;
					printf("Required packets: %d\n", total);
					printf("----------------------------------------\n");

					// initialize GBN state
					int curr = 0;
					int seq_base = 0;
					struct packet in, out;
					// start GBN procedure
					while (seq_base < total) {
						FD_ZERO(&readset);
						FD_SET(s, &readset);
						if (select(s + 1, &readset, NULL, NULL, &timeout) < 0) {
							error("ERROR on select\n");
						}
						else if (FD_ISSET(s, &readset)) {
							if (recvfrom(s, (char*)&in, sizeof(in), 0,
								(struct sockaddr*) &sa_in, &length) < 0) {
								printf("Packet lost \n");
								curr = 0;
								continue;
							}

							printPkt(in, 0);

							// slide window forward if possible
							if (in.type != 2)
								printf("IGNORE %d: not ACK packet\n", in.seqnum);
							else if (in.seq < seq_base || in.seq > seq_base + WSIZE)
								printf("IGNORE %d: unexpected ACK\n", in.seqnum);
							else if (in.seq >= seq_base) {
								seq_base = in.seq + 1;
								printf("Sliding window to %d/%d\n", seq_base, total);
								curr = 0;
							}
						}
						else {
							if (curr >= WSIZE || seq_base + curr >= total) {
								printf("Timeout on \n");
								curr = 0;
							}
							else if (seq_base + curr >= total)
								continue;

							bzero((char *)&out, sizeof(out));
							out.type = 1; // data packet
							out.seq = seq_base + curr;
							out.seqnum = (startSeq + out.seq) % maxSeq;
							fseek(file, out.seq * psize, SEEK_SET);
							out.size = fread(out.data, 1, psize, file);

							// send next packet in window
							if (sendto(s, (char*)&out, sizeof(out), 0,
								(struct sockaddr*) &sa_in, sizeof(sa_in)) < 0)
								error("ERROR sending packet\n");
							printPkt(out, 1);
							curr++;
						}
					}

					printf("All packets sent and acknowledged\n");

					// send FIN
					bzero((char *)&out, sizeof(out));
					out.type = 3; // FIN packet
					out.seq = seq_base;
					out.size = 0;
					if (sendto(s, (char*)&out, sizeof(out), 0,
						(struct sockaddr*) &sa_in, sizeof(sa_in)) < 0)
						error("ERROR sending FIN\n");
					printPkt(out, 1);

					// wait for FIN ACK, resend if necessary
					while (1) {
						FD_ZERO(&readset);
						FD_SET(s, &readset);
						if (select(s + 1, &readset, NULL, NULL, &timeout) < 0) {
							error("ERROR on select\n");
						}
						else if (FD_ISSET(s, &readset)) {
							if (recvfrom(s, (char*)&in, sizeof(in), 0,
								(struct sockaddr*) &sa_in, &length) < 0) {
								printf("Packet lost\n");
								continue;
							}
							printPkt(in, 0);

							if (in.type == 3 && in.seq == out.seq)
								break;
						}
						else {
							if (sendto(s, (char*)&out, sizeof(out), 0,
								(struct sockaddr*) &sa_in, sizeof(sa_in)) < 0)
								error("ERROR sending FIN\n");
							printPkt(out, 1);
						}
					}

					// cleanup
					printf("File transfer complete\n");
					printf("----------------------------------------\n");
					fclose(file);

					
				}
				else if (strcmp(handshake.direction, "list") == 0){
					int sequenceNumber = handshake.client_number % 2;
					int size = f_size;				
					ibytesrecv = 0;
					
					while (1){
						while (getFrame(s, *tp)){ ; } // keep waiting until a new packet has come in.
						if (message_frame.header == 'l'){
							cout << "Client: finish receiving list of files from server." << endl;
							break;
						}
						ibytesrecv = message_frame.data_length;
						if (ibytesrecv == SOCKET_ERROR)
							throw "Receive error in server program\n";
						
						if (message_frame.snwseq != sequenceNumber){
							// send nak
							ack = message_frame.snwseq;
							ibytessent = sendto(s, (const char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
							//cout << "Client:   dropped packet " << ack << endl;
							if (ibytessent == SOCKET_ERROR)
								throw "Client: Failed sending NAK";

						}
						else{
							// send ack
							ack = message_frame.snwseq;
							ibytessent = sendto(s, (const char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
							cout << message_frame.data<<endl;
							if (ibytessent == SOCKET_ERROR)
								throw "Client: Failed sending ACK";
							
							sequenceNumber = (sequenceNumber == 0 ? 1 : 0);
						}
					}
					for (int i = 0; i < 10; ++i)
						sendto(s, (const char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
					
				}
			}
			closesocket(s);
		}

		cout << "Bye" << endl;

	} // try loop

	//Display any needed error response.

	catch (char *str) { cerr<<str<<":"<<dec<<WSAGetLastError()<<endl;}

	//close the client socket
	closesocket(s);

	/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
	WSACleanup();
	system("pause");
	return 0;
}





