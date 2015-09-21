/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma once
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <winsock.h>
#include <iostream>
#include <windows.h>
#include <sys/stat.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace std;

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0);

#define WINDOW_SIZE 8;
const int WSIZE = WINDOW_SIZE;
#define SEQ_MAX 8;
//port data types

#define SERVER_PORT 5001
#define ROUTER_PORT 7001
int port=SERVER_PORT;

//socket data types
SOCKET s;

SOCKET s1;
SOCKADDR_IN sa;      // filled by bind

SOCKADDR_IN sa1;     // fill with server info, IP, port
socklen_t length = sizeof(sa1);
union {struct sockaddr generic;
	struct sockaddr_in ca_in;}ca;

	int calen=sizeof(ca); 

	//buffer data types
	char szbuffer[128];

	char *buffer;
	int ibufferlen;
	int ibytesrecv;

	int ibytessent;

	//host data types
	char localhost[11];

	HOSTENT *hp;

	//wait variables
	int nsa1;
	int r,infds=1, outfds=0;
	fd_set readfds;

	//control packet
	typedef enum { CLIENT_REQ = 1, ACK_CLIENT, ACK_SERVER, FILE_NOT_EXIST, INVALID,NAK,INITIAL,DUP } HandshakeType;
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
	int f_size ;
	char f_name[FILE_NAME_SIZE];
	//message frame
#define STOPNWAIT 1 //Bits width For stop and wait
	const int MAX_SIZE = 80;
	struct MESSAGE_FRAME {
		unsigned char header;
		unsigned int snwseq;
		char data[MAX_SIZE];//or error message
		int data_length;
	} message_frame;
#define PACKET_SIZE 80
	//const int PSIZE = PACKET_SIZE;
	int psize = 128;
	struct packet {
		int type;
		int seq;
		int size;
		char data[128];
		int seqnum;
	};
	//acknowledgement
	int ack;
	int ack2;
	fd_set readset;
	//others
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
			bytes_recvd = recvfrom(sock, (char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa1, &length);
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
			bytes_recvd = recvfrom(sock, (char *)&ack, sizeof(ack), 0, (struct sockaddr*)&sa1, &length);
			return false;
		}
		else{
			return true;
		}
	}
	bool getHandshakeResponse(int sock, const timeval timeout){
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		int bytes_recvd;
		int outfds = select(1, &readfds, NULL, NULL, &timeout);
		if (outfds == 0){
			return true;
		}
		else if (outfds == 1){
			bytes_recvd = recvfrom(sock, (char *)&handshake, sizeof(handshake), 0, (struct sockaddr*)&sa1, &length);
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
			bytes_recvd = recvfrom(sock, (char *)&message_frame, sizeof(message_frame), 0, (struct sockaddr*)&sa1, &length);
			return false;
		}
		else{
			return true;
		}
	}
	void resetHandshake(){
		handshake.client_number = 0;
		handshake.server_number = 0;
		sprintf_s(handshake.direction,"");
		sprintf_s(handshake.file_name, "");
		handshake.file_size=0;
		handshake.type = INITIAL;
	}
	void printPkt(struct packet pkt, int io) {
		if (io == 0){
			if (pkt.type == 1)
				printf("RECV DATA:\t  Seq %d\t Size %d\n",
					 pkt.seqnum, pkt.size);
			else if (pkt.type ==2)
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
	int main(void){
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 300000;
		const struct timeval *tp = &timeout;

		WSADATA wsadata;

		try{        		 
			if (WSAStartup(0x0202,&wsadata)!=0){  
				cout<<"Error in starting WSAStartup()\n";
			}else{
				buffer="WSAStartup was suuccessful\n";   
				WriteFile(test,buffer,sizeof(buffer),&dwtest,NULL); 

				
			}  

			//Display info of local host

			gethostname(localhost,10);
			cout<<"hostname: "<<localhost<< endl;

			if((hp=gethostbyname(localhost)) == NULL) {
				cout << "gethostbyname() cannot get local host info?"
					<< WSAGetLastError() << endl; 
				exit(1);
			}

			//Create the server socket
			if((s = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET) 
				throw "can't initialize socket";
			// For UDP protocol replace SOCK_STREAM with SOCK_DGRAM 


			//Fill-in Server Port and Address info.
			memset(&sa, 0, sizeof(sa));
			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);
			sa.sin_addr.s_addr = htonl(INADDR_ANY);


			//Bind the server port

			if (bind(s,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR)
				throw "can't bind the socket";
			cout << "Bind was successful" << endl;

			//wait loop

			while(1)

			{
				cout << "server starting" << endl;
				resetHandshake();
				message_frame.header = 'i'; // initial packet.
				while (getHandshakeResponse(s, *tp) || handshake.type != CLIENT_REQ){ ; };
				f_size = handshake.file_size;
				sprintf_s(f_name,handshake.file_name);
				cout << "SERVER: received client number " << handshake.client_number << endl;
				if (strcmp(handshake.direction, "get")==0){
					
					if (!isFileExist(handshake.file_name)){
						handshake.type = FILE_NOT_EXIST;
						cout << "File does not exist on server." << endl;
					}		
					else{
						handshake.type = ACK_CLIENT;
						handshake.file_size = getSize(handshake.file_name);
						
						
					}
				}
				else if (strcmp(handshake.direction, "put")==0){
					if (isFileExist(handshake.file_name)){
						sprintf_s(f_name , strcat(handshake.file_name, "copy"));
					}
						handshake.type = ACK_CLIENT;
				}
				else if (strcmp(handshake.direction, "list") == 0){
					handshake.type = ACK_CLIENT;
				}
				else if (strcmp(handshake.direction, "rename") == 0){
					if (!isFileExist(handshake.file_name)){
						handshake.type = FILE_NOT_EXIST;
						cout << "File does not exist on server." << endl;
					}
					else{
						// change name
						int result = rename(handshake.file_name, handshake.newName);
						if (result == 0)
							cout << "file renamed successfully" << endl;
						else
							cout << "Error renaming file" << endl;
					}
				}
				else if (strcmp(handshake.direction, "delete") == 0){
					if (!isFileExist(handshake.file_name)){
						handshake.type = FILE_NOT_EXIST;
						cout << "File does not exist on server." << endl;
					}
					else{
						// delete file
						int result = remove(handshake.file_name);
						if (result == 0)
							cout << "file renamed successfully" << endl;
						else
							cout << "Error renaming file" << endl;
					}
				}
				else{
					handshake.type = INVALID;
					cout << "server invalid request" << endl;
				}

				
				if (handshake.type== ACK_CLIENT){
					// Second way. Server receives client number from client.
					int randnum = rand();
					
					    // send syn_ack to client. May get lost, but client will send syn packet again.
					
					do{	
						
							handshake.server_number = randnum;
							if (handshake.type == CLIENT_REQ){
								handshake.type = ACK_CLIENT;
								handshake.file_size = getSize(handshake.file_name);
							}

							ibytessent = 0;
							ibytessent = sendto(s, (const char *)&handshake, sizeof(handshake), 0, (struct sockaddr*)&sa1, sizeof(sa1));
							cout << "SERVER: sending server number " << handshake.server_number << endl;
							if (ibytessent == SOCKET_ERROR)
								throw "Send failed\n";
							while (getHandshakeResponse(s, *tp)){ ; }
							if (handshake.type == ACK_CLIENT && (strcmp(handshake.direction, "get") == 0 || strcmp(handshake.direction, "put") == 0 || strcmp(handshake.direction, "list") == 0))
								continue;
							if (handshake.type == ACK_SERVER)
								break;

							if (strcmp(handshake.direction, "get") != 0 && strcmp(handshake.direction, "put") != 0 && strcmp(handshake.direction, "list") != 0){
							handshake.type = NAK;
							break;
						}
					} while (1);
				


					// --------------------------After the handshake. Server side ---------------------------

					if (handshake.type == ACK_SERVER||handshake.type==NAK){
						//cout << "Server finished handshaking." << endl;
					
						if (strcmp(handshake.direction, "get")==0){
							FILE *file = fopen(f_name, "rb");
							struct stat st;
							stat(f_name, &st);
							int total = st.st_size / psize;
							if (st.st_size % psize > 0)
								total++;
							printf("Required packets: %d\n", total);
							printf("----------------------------------------\n");
							int startSeq = handshake.client_number % 2;
							int maxSeq = WSIZE + 2;
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
										(struct sockaddr*) &sa1, &length) < 0) {
										printf("Packet lost \n");
										curr = 0;
										continue;
									}

									printPkt(in, 0);

									// slide window forward if possible
									if (in.type != 2)
										printf("IGNORE %d: not ACK packet\n", in.seq);
									else if (in.seq < seq_base || in.seq > seq_base + WSIZE)
										printf("IGNORE %d: unexpected ACK\n", in.seq);
									else if (in.seq >= seq_base) {
										seq_base = in.seq + 1;
										printf("Sliding window to %d/%d\n", seq_base, total);
										curr = 0;
									}
								}
								else {
									if (curr >= WSIZE || seq_base + curr >= total) {
										printf("Timeout\n");
										curr = 0;
									}
									else if (seq_base + curr >= total)
										continue;

									bzero((char *)&out, sizeof(out));
									out.type = 1; // data packet
									out.seq = seq_base + curr;
									out.seqnum = (startSeq+out.seq)%maxSeq;
									fseek(file, out.seq * psize, SEEK_SET);
									out.size = fread(out.data, 1, psize, file);

									// send next packet in window
									if (sendto(s, (char*)&out, sizeof(out), 0,
										(struct sockaddr*) &sa1, sizeof(sa1)) < 0)
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
								(struct sockaddr*) &sa1, sizeof(sa1)) < 0)
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
										(struct sockaddr*) &sa1, &length) < 0) {
										printf("Packet lost\n");
										continue;
									}
									printPkt(in, 0);

									if (in.type == 3 && in.seq == out.seq)
										break;
								}
								else {
									if (sendto(s, (char*)&out, sizeof(out), 0,
										(struct sockaddr*) &sa1, sizeof(sa1)) < 0)
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
							//int size = getSize(handshake.file_name);
							//ifstream outgoing(handshake.file_name, ios::in | ios::binary);
							ibytessent = 0;
							//int totalbytes = 0;
							//bool cfinished = false;
							bool reachMax = false;
							int max_attempts = 10;
							int n_attempts = 0;
							cout << "SERVER: Listing files for client." << endl;
							WIN32_FIND_DATA search_data;

							memset(&search_data, 0, sizeof(WIN32_FIND_DATA));

							HANDLE handle = FindFirstFile(".\\*", &search_data);


							while (handle != INVALID_HANDLE_VALUE)
							{
								message_frame.snwseq = sequenceNumber;
								sprintf_s(message_frame.data, search_data.cFileName);
								ibufferlen = sizeof(message_frame.data);
								message_frame.data_length = ibufferlen;
								bool resent = false;
								do{
									
									if (!resent)
										cout << "SERVER: Sending packet " << sequenceNumber << endl;
									else
										cout << "SERVER: Resending packet " << sequenceNumber << endl;
									int tempBytes = sendto(s, (const char *)&message_frame, sizeof(message_frame), 0, (struct sockaddr*)&sa1, sizeof(sa1));
									
									resent = true;
								} while (getACK(s, *tp) || ack != message_frame.snwseq);
								sequenceNumber = (sequenceNumber == 0 ? 1 : 0);
								
							if (FindNextFile(handle, &search_data) == FALSE){
								do{
									++n_attempts;
									if (n_attempts > max_attempts){
										cout << "SERVER: did not receive ACK from server after " << n_attempts << " attempts " << endl;
										reachMax = true;
										break;
									}
									message_frame.header = 'l';
									sendto(s, (const char *)&message_frame, sizeof(message_frame), 0, (struct sockaddr*)&sa1, sizeof(sa1));
								} while (getACK(s, *tp));
								
								if (!reachMax){
									cout << "SERVER: Files have been listed successfully" << endl;
									break;
								}
								else if (reachMax){
									break;
								}
									
								}
	
							}
								
						}
						//else if (strcmp(handshake.direction, "put")==0){
						else if (strcmp(handshake.direction, "put") == 0||handshake.type == NAK){
							int startSeq = handshake.server_number;
							int maxSeq = WSIZE + 2;
							int curr = 0;
							FILE* file = fopen(f_name, "wb");
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
									(struct sockaddr*) &sa1, &length) < 0)
									printf("Packet lost\n");
								else {


									// check packet number and type
									if (in.seq > curr) {
										printf("IGNORE %d: expected %d\n", in.seqnum, (curr+1)%maxSeq);
										continue;
									}
									else if (in.seq < curr) {
										printf("IGNORE %d: expected %d\n", in.seqnum, (curr + 1) % maxSeq);
										out.seq = in.seq; // send ACK anyways
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
										curr++;
									}

									if (sendto(s, (char*)&out, sizeof(out), 0,
										(struct sockaddr*) &sa1, sizeof(sa1)) < 0)
										error("ERROR sending ACK\n");
									printPkt(out, 1);
								}
							}

							// send FIN ACK
							bzero((char *)&out, sizeof(out));
							out.type = 3; // FIN packet
							out.seq = curr;
							out.size = 0;
							if (sendto(s, (char*)&out, sizeof(out), 0,
								(struct sockaddr*) &sa1, sizeof(sa1)) < 0)
								error("ERROR sending FIN\n");
							printPkt(out, 1);

							printf("Exiting client\n");
							fclose(file);

						}
					}
				}
				else{
					ibytessent = 0;
					ibytessent = sendto(s, (const char *)&handshake, sizeof(handshake), 0, (struct sockaddr*)&sa1, sizeof(sa1));
					if (ibytessent == SOCKET_ERROR)
						throw "Send failed\n";
					cout << "Server sent error message to client." << endl;
					
				}

			}//wait loop

		} //try loop

		//Display needed error message.

		catch(char* str) { cerr<<str<<WSAGetLastError()<<endl;}

		//close Client socket
		closesocket(s1);		

		//close server socket
		closesocket(s);

		/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
		WSACleanup();
		system("pause");
		return 0;
	}




