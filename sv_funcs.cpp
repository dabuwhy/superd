/* 13S103046 王岳
sv_funcs.cpp - TCPechod, TCPchargend, TCPdaytimed, TCPtimed */

#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <iostream>
#include <io.h>
#include <map>
using namespace std;
#define	BUFFERSIZE	4096		/* max read buffer size	*/
extern char tcproot[128];
extern char ftproot[128];
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
void	TCPechod(void *), TCPchargend(void *), TCPdaytimed(void *),TCPtimed(void *);
void	errexit(const char *, ...);

/*------------------------------------------------------------------------
 * TCPecho - do TCP ECHO on the given socket
 *------------------------------------------------------------------------
 */
void TCPechod(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	char	buf[BUFFERSIZE];
	int	cc;

	while (cc = recv(fd, buf, sizeof(buf), 0)) {
		if (cc == SOCKET_ERROR)
			break;//errexit("echo recv: errnum %d\n", GetLastError());
		if (send(fd, buf, cc, 0) == SOCKET_ERROR)
			break;//errexit("echo send: errnum %d\n", GetLastError());
	}
	closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}
void UDPechod(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	//char	buf[BUFFERSIZE];
	int alen=sizeof(((SOCKET_INF *)w)->fsin);


	//while (recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin),&alen)!=SOCKET_ERROR) 
	do{
		if(((SOCKET_INF *)w)->bPasv){
			((SOCKET_INF *)w)->bPasv=false;
			char        LocalAddr[80];
			gethostname(LocalAddr,sizeof(LocalAddr));
			LPHOSTENT	lp=gethostbyname(LocalAddr);
			struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);

			printf("%s  %d  %s:",inet_ntoa((((SOCKET_INF *)w)->fsin).sin_addr),(((SOCKET_INF *)w)->fsin).sin_port,LocalAddr);
			printf("%s  %s\n",inet_ntoa(*hostAddr),((SOCKET_INF *)w)->directory);
			
		}
		(void) sendto(fd,((SOCKET_INF *)w)->buffRecv,128,0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin),sizeof(((SOCKET_INF *)w)->fsin));
	}while (recvfrom(fd,((SOCKET_INF *)w)->buffRecv,sizeof(((SOCKET_INF *)w)->buffRecv),0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin),&alen)!=SOCKET_ERROR);
	//closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}
#define	LINELEN		72

/*------------------------------------------------------------------------
 * TCPchargend - do TCP CHARGEN on the given socket
 *------------------------------------------------------------------------
 */
void TCPchargend(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	char	c, buf[LINELEN+2];	/* print LINELEN chars + \r\n */

	c = ' ';
	buf[LINELEN] = '\r';
	buf[LINELEN+1] = '\n';
	while (1) {
		int	i;

		for (i=0; i<LINELEN; ++i) {
			buf[i] = c++;
			if (c > '~')
				c = ' ';
		}
		if (send(fd, buf, LINELEN+2, 0) == SOCKET_ERROR)
			break;
	}
	closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}

/*------------------------------------------------------------------------
 * TCPdaytimed - do TCP DAYTIME protocol
 *------------------------------------------------------------------------
 */
void TCPdaytimed(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	char	buf[LINELEN];
	time_t	now;

	(void) time(&now);
	sprintf(buf, "%s", ctime(&now));
	(void) send(fd, buf, strlen(buf), 0);
	closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}

#define	WINEPOCH	2208988800	/* Windows epoch, in UCT secs	*/

/*------------------------------------------------------------------------
 * TCPtimed - do TCP TIME protocol
 *------------------------------------------------------------------------
 */
void TCPtimed(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	time_t	now;

	(void) time(&now);
	now = htonl((u_long)(now + WINEPOCH));
	(void) send(fd, (char *)&now, sizeof(now), 0);
	closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}

void UDPtimed(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	int alen=sizeof(((SOCKET_INF *)w)->fsin);
	char	buf[2048];
	//if (recvfrom(fd, buf, sizeof(buf), 0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin), &alen) == SOCKET_ERROR)
	//		errexit("recvfrom: error %d\n", GetLastError());
	time_t	now;
	(void) time(&now);
	now = htonl((u_long)(now + WINEPOCH));

	char        LocalAddr[80];
	gethostname(LocalAddr,sizeof(LocalAddr));
	LPHOSTENT	lp=gethostbyname(LocalAddr);
	struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);

	printf("%s  %d  %s:",inet_ntoa((((SOCKET_INF *)w)->fsin).sin_addr),(((SOCKET_INF *)w)->fsin).sin_port,LocalAddr);
	printf("%s  %s\n",inet_ntoa(*hostAddr),((SOCKET_INF *)w)->directory);


	(void) sendto(fd, (char *)&now, sizeof(now), 0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin), sizeof(((SOCKET_INF *)w)->fsin));
	//closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}
void ReadHttpHeaderLine(SOCKET &fd,char *request1,char *buf,int &bufLength){
	int nBytesThisTime=bufLength,nLength=0;
	char *pch1=buf,*pch2;
	do{
		if((pch2=(char*) memchr(pch1,'\n',nBytesThisTime))!=NULL){
			//buf[nBytesThisTime]='\0';
			//printf("%s",buf);

			nLength=pch2-buf+1;
			nLength=(nLength>99)?99:nLength;
			memcpy(request1,buf,nLength);
			bufLength-=nLength;
			memmove(buf,pch2+1,bufLength);
			break;
		}
		pch1+=nBytesThisTime;
		
		TIMEVAL tv={10,0};
		FD_SET fdd={1,fd};
		if(select(0,&fdd,NULL,NULL,&tv)==0){
			break;//printf("Receive timeout\n");
			return;
		}
		if((nBytesThisTime=recv(fd, buf+bufLength, BUFFERSIZE-bufLength, 0))==SOCKET_ERROR){
			break;//printf("Receive error\n");
			return;
		}
		
		
		bufLength+=nBytesThisTime;
	}while(TRUE);
	*(request1+nLength)='\0';
}
void Write(SOCKET &fd,const char* pch,const int size){
	int a=0,b;
	const char *p=pch;
	do{
		TIMEVAL tv={10,0};
		FD_SET fdd={1,fd};
		if(select(0,NULL,&fdd,NULL,&tv)==0){
			break;//printf("Send timeout\n");
			return;
		}
		if((b=send(fd,p,size-a,0))<0){
			break;//printf("Send error\n");
			return;
		}
		a+=b;
		p+=b;
	}while(a<size);
}
void readsrc(ifstream &f,char *custom,int &i){
	char dire[1024],tmp[4096];
	strcpy(dire,tcproot);
	int j;
	for(j=strlen(dire);dire[j]!='"';j++){
		f.get(dire[j]);
	}
	/*dire[j]='\0';
	ifstream f1(dire,ios::binary);
	if(!f1){
		printf("open src error\n");
		return;
	}
	for(;!f1.eof();i++){
		f1.get(custom[i]);
	}*/
	custom[i]='"';i++;
}

	
void TCPhttpd(void *w){SOCKET fd=((SOCKET_INF *)w)->s;

	map<string, char *> m_typeMap;
	m_typeMap[".doc"]	= "application/msword";
	m_typeMap[".bin"]	= "application/octet-stream";
	m_typeMap[".dll"]	= "application/octet-stream";
	m_typeMap[".exe"]	= "application/octet-stream";
	m_typeMap[".pdf"]	= "application/pdf";
	m_typeMap[".ai"]	= "application/postscript";
	m_typeMap[".eps"]	= "application/postscript";
	m_typeMap[".ps"]	= "application/postscript";
	m_typeMap[".rtf"]	= "application/rtf";
	m_typeMap[".fdf"]	= "application/vnd.fdf";
	m_typeMap[".arj"]	= "application/x-arj";
	m_typeMap[".gz"]	= "application/x-gzip";
	m_typeMap[".class"]	= "application/x-java-class";
	m_typeMap[".js"]	= "application/x-javascript";
	m_typeMap[".lzh"]	= "application/x-lzh";
	m_typeMap[".lnk"]	= "application/x-ms-shortcut";
	m_typeMap[".tar"]	= "application/x-tar";
	m_typeMap[".hlp"]	= "application/x-winhelp";
	m_typeMap[".cert"]	= "application/x-x509-ca-cert";
	m_typeMap[".zip"]	= "application/zip";
	m_typeMap[".cab"]	= "application/x-compressed";
	m_typeMap[".arj"]	= "application/x-compressed";
	m_typeMap[".aif"]	= "audio/aiff";
	m_typeMap[".aifc"]	= "audio/aiff";
	m_typeMap[".aiff"]	= "audio/aiff";
	m_typeMap[".au"]	= "audio/basic";
	m_typeMap[".snd"]	= "audio/basic";
	m_typeMap[".mid"]	= "audio/midi";
	m_typeMap[".rmi"]	= "audio/midi";
	m_typeMap[".mp3"]	= "audio/mpeg";
	m_typeMap[".vox"]	= "audio/voxware";
	m_typeMap[".wav"]	= "audio/wav";
	m_typeMap[".ra"]	= "audio/x-pn-realaudio";
	m_typeMap[".ram"]	= "audio/x-pn-realaudio";
	m_typeMap[".bmp"]	= "image/bmp";
	m_typeMap[".gif"]	= "image/gif";
	m_typeMap[".jpeg"]	= "image/jpeg";
	m_typeMap[".jpg"]	= "image/jpeg";
	m_typeMap[".tif"]	= "image/tiff";
	m_typeMap[".tiff"]	= "image/tiff";
	m_typeMap[".xbm"]	= "image/xbm";
	m_typeMap[".wrl"]	= "model/vrml";
	m_typeMap[".htm"]	= "text/html";
	m_typeMap[".html"]	= "text/html";
	m_typeMap[".c"]		= "text/plain";
	m_typeMap[".cpp"]	= "text/plain";
	m_typeMap[".def"]	= "text/plain";
	m_typeMap[".h"]		= "text/plain";
	m_typeMap[".txt"]	= "text/plain";
	m_typeMap[".rtx"]	= "text/richtext";
	m_typeMap[".rtf"]	= "text/richtext";
	m_typeMap[".java"]	= "text/x-java-source";
	m_typeMap[".css"]	= "text/css";
	m_typeMap[".mpeg"]	= "video/mpeg";
	m_typeMap[".mpg"]	= "video/mpeg";
	m_typeMap[".mpe"]	= "video/mpeg";
	m_typeMap[".avi"]	= "video/msvideo";
	m_typeMap[".mov"]	= "video/quicktime";
	m_typeMap[".qt"]	= "video/quicktime";
	m_typeMap[".shtml"]	= "wwwserver/html-ssi";
	m_typeMap[".asa"]	= "wwwserver/isapi";
	m_typeMap[".asp"]	= "wwwserver/isapi";
	m_typeMap[".cfm"]	= "wwwserver/isapi";
	m_typeMap[".dbm"]	= "wwwserver/isapi";
	m_typeMap[".isa"]	= "wwwserver/isapi";
	m_typeMap[".plx"]	= "wwwserver/isapi";
	m_typeMap[".url"]	= "wwwserver/isapi";
	m_typeMap[".cgi"]	= "wwwserver/isapi";
	m_typeMap[".php"]	= "wwwserver/isapi";
	m_typeMap[".wcgi"]	= "wwwserver/isapi";
	char hdrFmt[]=
		"HTTP/1.0 200 OK\r\n"
		"Server: WY's Socket Server\r\n"
		"Date: %s\r\n"
		"Content-Type: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %d\r\n";
	//"Content-Type: text/html\r\n"
	char CustomHtml[]=
		"<html>\r\n"
		"<head>\r\n"
		"<title></title>\r\n"
		"</head>\r\n"
		"<body>\r\n"
		"<h1>呵呵</h1>\r\n"
		"</body></html>\r\n\r\n";
	char buf[BUFFERSIZE];
	
	int bufLength=0;
	time_t	now;
	struct tm *tm1=NULL;
	(void) time(&now);
	tm1=localtime(&now);
	char t[256],dire[256];
	BYTE custom[BUFFERSIZE];
	strftime(t,sizeof(t),"%a, %d %b %Y %H:%M:%S GMT",tm1);

	char headers[500],request1[100],request2[100],*ptoken1,*ptoken2;
	if(fd!=NULL) ReadHttpHeaderLine(fd,request1,buf,bufLength);
	//printf("%s",request1);
	ptoken1=request1;
	char *tmp=strchr(request1,' ');
	if(tmp){
		*tmp='\0';
		tmp++;
		ptoken2=tmp;
		tmp=strchr(tmp,' ');
		if(tmp){
			*tmp='\0';
		}
	}
	if(!stricmp(ptoken1,"GET")){
		do{
			if(fd!=NULL) ReadHttpHeaderLine(fd,request2,buf,bufLength);
		}while(strcmp(request2,"\r\n"));
		if(!stricmp(ptoken2,"/hehe")){
			wsprintfA(headers, hdrFmt, (const char*) t, strlen(CustomHtml));
			strcat(headers, "\r\n");
			Write(fd,headers,strlen(headers));
			Write(fd,CustomHtml, strlen(CustomHtml));
		}
		else{
			int i,j;
			strcpy(dire,tcproot);
			for(i=0,j=strlen(dire);ptoken2[i]!='\0';i++,j++){
				dire[j]=(ptoken2[i]=='/')?'\\':ptoken2[i];
			}
			dire[j]='\0';
			//printf("%s\n",dire);
			char *a=strstr(ptoken2,".");
			map<string, char *>::iterator it;
			if(a!=NULL) it = m_typeMap.find(strlwr(a));

			ifstream f(dire,ios::binary);
			if(!f){
				strcat(dire,"index.html");
				f.open(dire,ios::binary);
				if(!f){
					strcat(dire,"index.htm");
					f.open(dire,ios::binary);
					if(!f){
						return;
					}
				}
			}
			bool src=false;
			char tmp=' ';
				f.read((char *)custom,sizeof(custom));
				//printf("%s\n",custom);
				if(a!=NULL){
					
					//printf("%s",(*it).second);
					DWORD len=GetFileSize(f, NULL);
					wsprintfA(headers, hdrFmt, (const char*) t,(*it).second, len);
				}
				else wsprintfA(headers, hdrFmt, (const char*) t,"text/html", f.gcount());
				strcat(headers, "\r\n");
				Write(fd,headers,strlen(headers));
				Write(fd,(char *)custom, f.gcount());
			while(!f.eof()){
				f.read((char *)custom,sizeof(custom));
				
				Write(fd,(char *)custom, f.gcount());
			}

			
			f.close();
		}
	}
	else if(!stricmp(ptoken1,"POST")){
		do{
			if(fd!=NULL) ReadHttpHeaderLine(fd,request2,buf,bufLength);
		}while(strcmp(request2,"\r\n"));
		if(fd!=NULL) ReadHttpHeaderLine(fd,request2,buf,bufLength);
	}
	closesocket(fd);
	((SOCKET_INF *)w)->use=true;
}

/*
void TCPftpd(void *w){SOCKET fd=((SOCKET_INF *)w)->s;
	char buf[BUFFERSIZE];
	int nBytesThisTime;
	strcpy(buf,"220 wy ready\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //USER anonymous

	strcpy(buf,"331 pass\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //PASS 123

	strcpy(buf,"230 login\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //PWD

	strcpy(buf,"257 \"e:\\\" is current directory.\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //TYPE I

	strcpy(buf,"200 Type set to I\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf);//PASV

	strcpy(buf,"227 Entering Passive Mode(127,0,0,1,14,23)\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf);//LIST

	
	ftp_request_rec r;
	r.so_client=fd;
	r.so_remote=SOCKET_ERROR;
	r.port=23;




	strcpy(buf,"drwxrwxrwx   1 ftp      ftp 0 music\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf);

	closesocket(fd);
}
*/