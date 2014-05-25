/* 13S103046 Õı‘¿
superd.cpp - main, doTCP */

#include <process.h>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <iostream>
#define	UDP_SERV	0
#define	TCP_SERV	1
using namespace std;
struct service {
	char	*sv_name;
	char	sv_useTCP;
	SOCKET	sv_sock;
	void	(*sv_func)(void *);
};

void	TCPechod(void *), TCPchargend(void *), TCPdaytimed(void *),	TCPtimed(void *),TCPhttpd(void *),TCPftpd(void *),UDPtimed(void *),UDPechod(void *);

SOCKET	passiveTCP(const char *, int);
SOCKET	passiveUDP(const char *);
void	errexit(const char *, ...);
void	doTCP(struct service *);

struct service svent[] =
	{	{ "echo", TCP_SERV, INVALID_SOCKET, TCPechod },
		{ "chargen", TCP_SERV, INVALID_SOCKET, TCPchargend },
		{ "daytime", TCP_SERV, INVALID_SOCKET, TCPdaytimed },
		{ "time", TCP_SERV, INVALID_SOCKET, TCPtimed },
		{ "http", TCP_SERV, INVALID_SOCKET, TCPhttpd },
		{ "ftp", TCP_SERV, INVALID_SOCKET, TCPftpd },
		{ "time", UDP_SERV, INVALID_SOCKET, UDPtimed },
		{ "daytime", UDP_SERV, INVALID_SOCKET, UDPtimed },
		{ "echo", UDP_SERV, INVALID_SOCKET, UDPechod },
		{ 0, 0, 0, 0 },
	};


#define WSVERS		MAKEWORD(2, 0)
#define	QLEN		   5
#define	LINELEN		 128

extern	u_short	portbase;	/* from passivesock()	*/
char tcproot[128];
char ftproot[128];
char user[128];
char pass[128];

sockaddr addr;

typedef struct y{
   CHAR   buffRecv[8192];
   CHAR   buffSend[8192];
   WSABUF wsaBuf;
   SOCKET s;
   SOCKET send;
	SOCKET sAccept;
   WSAOVERLAPPED o;
   DWORD dwBytesSend;
   DWORD dwBytesRecv;
   int   nStatus;
   char directory[512];
	bool use;
	bool bPasv;
	ofstream out;
	sockaddr_in fsin;
	y(){
		use=true;
	}
} SOCKET_INF, *LPSOCKET_INF;
SOCKET_INF a[1024];

/*------------------------------------------------------------------------
 * main - Super-server main program
 *------------------------------------------------------------------------
 */
void main(int argc, char *argv[]){
	struct service	*psv;		/* service table pointer	*/
	fd_set		afds, rfds;	/* readable file descriptors	*/
	WSADATA		wsdata;
	ifstream in("Readme.txt");
	if(!in){
		errexit("open root error\n", GetLastError());
	}
	in.getline(tcproot,128);
	in.getline(ftproot,128);
	in.getline(user,128);
	in.getline(pass,128);
	in.close();
	switch (argc) {
	case 1:
		break;
	case 2:
		portbase = (u_short) atoi(argv[1]);
		break;
	default:
		errexit("usage: superd [portbase]\n");
	}
	if (WSAStartup(WSVERS, &wsdata))
		errexit("WSAStartup failed\n");

	FD_ZERO(&afds);
	for (psv = &svent[0]; psv->sv_name; ++psv) {
		if (psv->sv_useTCP)
			psv->sv_sock = passiveTCP(psv->sv_name, QLEN);
		else
			psv->sv_sock = passiveUDP(psv->sv_name);
		FD_SET(psv->sv_sock, &afds);
	}



	while (1) {
		memcpy(&rfds, &afds, sizeof(rfds));
		if (select(FD_SETSIZE, &rfds, (fd_set *)0, (fd_set *)0,(struct timeval *)0) == SOCKET_ERROR)
			errexit("select error: %d\n", GetLastError());
		for (psv=&svent[0]; psv->sv_name; ++psv) {
			if (FD_ISSET(psv->sv_sock, &rfds)) {
				if (psv->sv_useTCP)
					doTCP(psv);
				else{
					int i;
					do{
						for(i=0;i<1024;i++) if(a[i].use) break;
					}while(i==1024);
					a[i].s=psv->sv_sock;
					a[i].use=false;
					a[i].bPasv=true;
					strcpy(a[i].buffRecv,"UDP");
					strcat(a[i].buffRecv,psv->sv_name);
					_beginthread((void (*)(void *))psv->sv_func, 0, (void *)&(a[i]));
					//psv->sv_func((void *)&(a[i]));
				}
				//else	psv->sv_func(psv->sv_sock);
			}
		}
	}
}

/*------------------------------------------------------------------------
 * doTCP - handle a TCP service connection request
 *------------------------------------------------------------------------
 */

void doTCP(struct service *psv){
	struct sockaddr_in fsin;	/* the request from address	*/
	int		alen,i;		/* from-address length		*/
	SOCKET		ssock;

	alen = sizeof(fsin);
	ssock = accept(psv->sv_sock, (struct sockaddr *)&fsin, &alen);
	
	char        LocalAddr[80];
	gethostname(LocalAddr,sizeof(LocalAddr));
	LPHOSTENT	lp=gethostbyname(LocalAddr);
	struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);


	int len=sizeof(addr);
	getsockname(ssock,&addr,&len);
	

	printf("%s  %d  %s:",inet_ntoa(fsin.sin_addr),fsin.sin_port,LocalAddr);
	printf("%s  %s\n",inet_ntoa(((sockaddr_in *)&addr)->sin_addr),psv->sv_name);
	if (ssock == INVALID_SOCKET)
		errexit("accept: %d\n", GetLastError());
	do{
		for(i=0;i<1024;i++) if(a[i].use) break;
		
	}while(i==1024);
	
	a[i].s=ssock;
	a[i].use=false;
	a[i].fsin=fsin;
	if(_beginthread((void (*)(void *))psv->sv_func, 0, (void *)&(a[i]))== (unsigned long) -1)
		errexit("_beginthread: %s\n", strerror(errno));
	
}


