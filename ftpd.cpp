//#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
//#include <stdlib.h>
//#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
using namespace std;
#define WSA_RECV         0
#define WSA_SEND         1

#define DATA_BUFSIZE    8192
#define MAX_NAME_LEN    128
#define MAX_PWD_LEN     128
#define MAX_RESP_LEN    1024
#define MAX_REQ_LEN     256
#define MAX_ADDR_LEN    80

#define FTP_PORT        21     // FTP 控制端口
#define DATA_FTP_PORT   20     // FTP 数据端口

#define USER_OK         331
#define LOGGED_IN       230
#define LOGIN_FAILED    530
#define CMD_OK          200
#define OPENING_AMODE   150
#define TRANS_COMPLETE  226
#define CANNOT_FIND     550
#define FTP_QUIT        221
#define CURR_DIR        257
#define DIR_CHANGED     250
#define OS_TYPE         215
#define REPLY_MARKER    504
#define PASSIVE_MODE    227

#define FTP_USER		"wy"
#define FTP_PASS		"wy"
#define DEFAULT_HOME_DIR    "e:"
extern char user[128];
extern char pass[128];
extern char ftproot[128];

#define MAX_FILE_NUM        1024

#define MODE_PORT       0
#define MODE_PASV       1

#define PORT_BIND   1821

typedef struct y{
	CHAR   buffRecv[DATA_BUFSIZE];
	CHAR   buffSend[DATA_BUFSIZE];
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

typedef struct {
	char     szFileName[MAX_PATH];
	DWORD    dwFileAttributes; 
    FILETIME ftCreationTime; 
    FILETIME ftLastAccessTime; 
    FILETIME ftLastWriteTime; 
    DWORD    nFileSizeHigh; 
    DWORD    nFileSizeLow; 
} FILE_INF, *LPFILE_INF;


BOOL WelcomeInfo( SOCKET s );
int LoginIn( LPSOCKET_INF pSocketInfo  );
int SendRes( LPSOCKET_INF pSI );
int RecvReq( LPSOCKET_INF pSI );
int DealCommand( LPSOCKET_INF pSI );
void	errexit(const char *, ...);
int GetFileList( LPSOCKET_INF &pSI ,LPFILE_INF pFI, UINT nArraySize, const char* szPath  );
char* GetLocalAddress();
char* HostToNet( char* szPath ) ;
char* NetToHost( char* szPath ) ;
char* RelativeDirectory( char* szDir );
char* AbsoluteDirectory( char* szDir );
BOOL  g_bLoggedIn;
char  g_szLocalAddr[MAX_ADDR_LEN];

extern sockaddr addr;
void TCPftpd(void *w){
	
	

	WelcomeInfo(((SOCKET_INF *)w)->s);
	//SetCurrentDirectoryA(ftproot);
	strcpy(((SOCKET_INF *)w)->directory,ftproot);
	
	sprintf(g_szLocalAddr,"%s.txt",inet_ntoa(((SOCKET_INF *)w)->fsin.sin_addr));
	
	
	
	char buff[DATA_BUFSIZE];
	memset( buff,0,DATA_BUFSIZE );
	((SOCKET_INF *)w)->wsaBuf.buf = buff;  
	((SOCKET_INF *)w)->wsaBuf.len = DATA_BUFSIZE;
	
    ((SOCKET_INF *)w)->dwBytesSend = 0;
    ((SOCKET_INF *)w)->dwBytesRecv = 0;
	((SOCKET_INF *)w)->nStatus     = WSA_RECV;    // 接收

	while(1){
		((SOCKET_INF *)w)->out.open(g_szLocalAddr,ios::app|ios::out);

		if ((((SOCKET_INF *)w)->dwBytesRecv=recv(((SOCKET_INF *)w)->s,((SOCKET_INF *)w)->buffRecv,sizeof(((SOCKET_INF *)w)->buffRecv) , NULL)) <=0){
			//printf("exit\n");
			((SOCKET_INF *)w)->out<<"exit\n";
			((SOCKET_INF *)w)->out.close();
			closesocket(((SOCKET_INF *)w)->s);
			((SOCKET_INF *)w)->use=true;
			return;
		}
		((SOCKET_INF *)w)->buffRecv[((SOCKET_INF *)w)->dwBytesRecv]='\0';

		((SOCKET_INF *)w)->out<<"recv:"<<((SOCKET_INF *)w)->buffRecv<<endl;
		//printf("recv:%s\n\n",g_sockets->buffRecv);


		if( ((SOCKET_INF *)w)->buffRecv[((SOCKET_INF *)w)->dwBytesRecv-2] == '\r'&& ((SOCKET_INF *)w)->buffRecv[((SOCKET_INF *)w)->dwBytesRecv-1] == '\n'&& ((SOCKET_INF *)w)->dwBytesRecv > 2 )  
		{                 
			if( !g_bLoggedIn )
			{
				if( LoginIn(((SOCKET_INF *)w)) == LOGGED_IN )
					g_bLoggedIn = TRUE;
			} 
			else 
			{
				if(DealCommand( ((SOCKET_INF *)w) )==FTP_QUIT)
					;
			}
		
	
		}
		((SOCKET_INF *)w)->out.close();

	}
}

int SendRes( LPSOCKET_INF pSI )
{
	static DWORD dwSendBytes = 0;
	pSI->nStatus = WSA_SEND;
    memset(&(pSI->o), 0,sizeof(WSAOVERLAPPED));
    //pSI->o.hEvent = g_events[g_index - WSA_WAIT_EVENT_0];
    pSI->wsaBuf.buf = pSI->buffSend + pSI->dwBytesSend;
    pSI->wsaBuf.len = strlen( pSI->buffSend ) - pSI->dwBytesSend;
    if (WSASend(pSI->s, &(pSI->wsaBuf), 1,&dwSendBytes,0,&(pSI->o), NULL) == SOCKET_ERROR){
        if (WSAGetLastError() != ERROR_IO_PENDING) 
		{
			printf("WSASend() failed with error %d\n", WSAGetLastError());
			return -1;
        }
    }
	pSI->wsaBuf.buf[dwSendBytes]='\0';
	pSI->out<<"send:"<<pSI->wsaBuf.buf<<endl;

	//printf("send:%s\n",pSI->wsaBuf.buf);


	return 0;
}
//接受数据
int RecvReq( LPSOCKET_INF pSI )
{
	static DWORD dwRecvBytes = 0;	
	pSI->nStatus = WSA_RECV;	

	DWORD dwFlags = 0;
	memset(&(pSI->o), 0,sizeof(WSAOVERLAPPED));
	//pSI->o.hEvent = g_events[g_index - WSA_WAIT_EVENT_0];
	pSI->wsaBuf.len = DATA_BUFSIZE;

	if (WSARecv(pSI->s, &(pSI->wsaBuf), 1, &dwRecvBytes,&dwFlags,&(pSI->o), NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
		   printf("WSARecv() failed with error %d\n", WSAGetLastError());
		   return -1;
		}
	}
	return 0;
}

//显示欢迎消息
BOOL WelcomeInfo( SOCKET s )
{
	char* szWelcomeInfo = "220 欢迎您登录到王岳FtpServer...\r\n";
	if( send( s,szWelcomeInfo,strlen(szWelcomeInfo),0 ) == SOCKET_ERROR ) 
	{
		printf("Ftp client error:%d\n", WSAGetLastError() );
		return FALSE;
	}
	// 刚进来，还没连接，故设置初始状态为false
	g_bLoggedIn = FALSE;
	return TRUE;
}
//登录函数
int LoginIn( LPSOCKET_INF pSocketInfo  )
{
	const char* szUserOK = "331 User name okay, need password.\r\n"; 
	const char* szLoggedIn = "230 User logged in, proceed.\r\n";

	int  nRetVal = 0;
	static char szUser[MAX_NAME_LEN], szPwd[MAX_PWD_LEN];
	LPSOCKET_INF pSI = pSocketInfo;
	// 取得登录用户名
	if( strstr(strupr(pSI->buffRecv),"USER") ) 
	{		
		sprintf(szUser,"%s",pSI->buffRecv+strlen("USER")+1);
		strtok( szUser,"\r\n");		
		// 响应信息
		sprintf(pSI->buffSend,"%s",szUserOK );
		if( SendRes(pSI) == -1 ) return -1;
		return USER_OK;
	}
	if( strstr(strupr(pSI->buffRecv),"PASS") || strstr(pSI->buffRecv,"pass") ) 
	{
		sprintf(szPwd,"%s",pSI->buffRecv+strlen("PASS")+1 );
		strtok( szPwd,"\r\n");
		// 判断用户名跟口令正确性
		if( stricmp( szUser,user) || stricmp(szPwd,pass) ) 
		{
			sprintf(pSI->buffSend,"530 User %s cannot log in.\r\n",szUser );
			printf("User %s cannot log in\n",szUser );
			nRetVal = LOGIN_FAILED;
		} 
		else 
		{
			sprintf(pSI->buffSend,"%s",szLoggedIn);
			//printf("User %s logged in\n",szUser );
			pSocketInfo->out<<"User "<<szUser<<" logged in\n";
			nRetVal = LOGGED_IN;
		}
		if( SendRes( pSI ) == -1 ) 
			return -1;
	}
	return nRetVal;
}

char* ConvertCommaAddress( char* szAddress, WORD wPort )
{
	char szPort[10];
	sprintf( szPort,"%d,%d",wPort/256,wPort%256 );
	char szIpAddr[20];

	//sprintf( szIpAddr,"%s,",szAddress );
	sprintf( szIpAddr,"%s,",inet_ntoa(((sockaddr_in *)&addr)->sin_addr));

	int idx = 0;
	while( szIpAddr[idx] ) 
	{
		if( szIpAddr[idx] == '.' )
			szIpAddr[idx] = ',';
		idx ++;
	}
	sprintf( szAddress,"%s%s",szIpAddr,szPort );
	return szAddress;
}

int ConvertDotAddress( char* szAddress, LPDWORD pdwIpAddr, LPWORD pwPort ) 
{
	int  idx = 0,i = 0, iCount = 0;
	char szIpAddr[MAX_ADDR_LEN]; memset( szIpAddr,0,sizeof(szIpAddr) );
	char szPort[MAX_ADDR_LEN];   memset( szPort,0,  sizeof(szPort)   );

	*pdwIpAddr = 0; *pwPort = 0;

	while( szAddress[idx]  )
	{
		if( szAddress[idx] == ',' )
		{
			iCount ++;
			szAddress[idx] ='.';
		}
		if( iCount < 4 )
			szIpAddr[idx] = szAddress[idx];
		else
			szPort[i++] =   szAddress[idx];
		idx++;
	}

	if( iCount != 5 ) return -1;
	*pdwIpAddr = inet_addr( szIpAddr );
	if( *pdwIpAddr  == INADDR_NONE ) return -1;
	char *pToken = strtok( szPort+1,"." );
	if( pToken == NULL ) return -1;
	*pwPort = (WORD)(atoi(pToken) * 256);
	pToken = strtok(NULL,".");
	if( pToken == NULL ) return -1;
	*pwPort += (WORD)atoi(pToken);
		
	return 0;
}

UINT FileListToString( LPSOCKET_INF &pSI,char* buff, UINT nBuffSize,BOOL bDetails )
{
	FILE_INF   fi[MAX_FILE_NUM];
	int nFiles = GetFileList( pSI,fi, MAX_FILE_NUM, "*.*" );
	char szTemp[128];
	sprintf( buff,"%s","" );
	if( bDetails ) {
		for( int i=0; i<nFiles; i++) {
			if( strlen(buff)>nBuffSize-128 )   break;
			if(!strcmp(fi[i].szFileName,"."))  continue;
			if(!strcmp(fi[i].szFileName,"..")) continue;
			// 时间
			SYSTEMTIME st;
			FileTimeToSystemTime(&(fi[i].ftLastWriteTime), &st);
			char  *szNoon = "AM";
			if( st.wHour > 12 ) 
			{ 
				st.wHour -= 12;
				szNoon = "PM"; 
			}
			if( st.wYear >= 2000 )
				st.wYear -= 2000;
			else st.wYear -= 1900;
			sprintf( szTemp,"%02u-%02u-%02u  %02u:%02u%s       ",st.wMonth,st.wDay,st.wYear,st.wHour,st.wMonth,szNoon );
			strcat( buff,szTemp );
			if( fi[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				strcat(buff,"<DIR>");
				strcat(buff,"          ");
			}
			else 
			{
				strcat(buff,"     ");
				// 文件大小
				sprintf( szTemp,"% 9d ",fi[i].nFileSizeLow );
				strcat( buff,szTemp );
			}
			// 文件名
			strcat( buff,fi[i].szFileName );
			strcat( buff,"\r\n");
		}
	} 
	else
	{ 
		for( int i=0; i<nFiles; i++)
		{
			if( strlen(buff) + strlen( fi[i].szFileName ) + 2 < nBuffSize )
			{ 
				strcat( buff, fi[i].szFileName );
				strcat( buff, "\r\n");
			} 
			else
				break;
		}
	}
	return strlen( buff );
}

DWORD ReadFileToBuffer(  LPSOCKET_INF &pSI,const char* szFile, char *buff, DWORD nFileSize )
{
	DWORD  idx = 0;
	DWORD  dwBytesLeft = nFileSize;
	DWORD  dwNumOfBytesRead = 0;
	char lpFileName[MAX_PATH];

	strcpy(lpFileName,pSI->directory); //GetCurrentDirectoryA( MAX_PATH,lpFileName );
	if(lpFileName[strlen(lpFileName)-1]!='\\')	strcat( lpFileName,"\\" );
	strcat(lpFileName,szFile );
	HANDLE hFile = CreateFileA( lpFileName,
							   GENERIC_READ,
							   FILE_SHARE_READ,
							   NULL,
							   OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		while( dwBytesLeft > 0 ) 
		{
			if( !ReadFile( hFile,&buff[idx],dwBytesLeft,&dwNumOfBytesRead,NULL ) )
			{
				printf("读文件出错.\n");
				CloseHandle( hFile );
				return 0;
			}
			idx += dwNumOfBytesRead;
			dwBytesLeft -= dwNumOfBytesRead;
		}
		CloseHandle( hFile );
	}
	return idx;
}

DWORD WriteToFile(  LPSOCKET_INF &pSI,SOCKET s , const char* szFile )
{
	DWORD  idx = 0;
	DWORD  dwNumOfBytesWritten = 0;
	DWORD  nBytesLeft = DATA_BUFSIZE;
	char   buf[DATA_BUFSIZE];
	char   lpFileName[MAX_PATH];
	strcpy(lpFileName,pSI->directory);//GetCurrentDirectoryA( MAX_PATH,lpFileName );
	if(lpFileName[strlen(lpFileName)-1]!='\\')	strcat( lpFileName,"\\" );
	strcat(lpFileName,szFile );
	HANDLE hFile = CreateFileA( lpFileName,
							   GENERIC_WRITE,
							   FILE_SHARE_WRITE,
							   NULL,
							   OPEN_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL );
	if( hFile == INVALID_HANDLE_VALUE ) 
	{ 
		printf("打开文件出错.\n");
		return 0;
	}
	
	while( TRUE )
	{
		int nBytesRecv = 0;
		idx = 0; nBytesLeft = DATA_BUFSIZE;
		while( nBytesLeft > 0 )
		{
			nBytesRecv = recv( s,&buf[idx],nBytesLeft,0 );
			if( nBytesRecv == SOCKET_ERROR ) 
			{
				printf("Failed to send buffer to socket %d\n",WSAGetLastError() );
				return -1;
			}
			if( nBytesRecv == 0 ) break;
		
			idx += nBytesRecv;
			nBytesLeft -= nBytesRecv;
		}
		nBytesLeft = idx;   // 要写入文件中的字节数
		idx = 0;			// 索引清0,指向开始位置
		while( nBytesLeft > 0 ) 
		{
			// 移动文件指针到文件末尾
			if( !SetEndOfFile(hFile) ) return 0;
			if( !WriteFile( hFile,&buf[idx],nBytesLeft,&dwNumOfBytesWritten,NULL ) ) 
			{
				printf("写文件出错.\n");
				CloseHandle( hFile );
				return 0;
			}
			idx += dwNumOfBytesWritten;
			nBytesLeft -= dwNumOfBytesWritten;
		}
		// 如果没有数据可接收，退出循环
		if( nBytesRecv == 0 ) break;
	}

	CloseHandle( hFile );
	return idx;
}
int CombindFileNameSize(LPSOCKET_INF &pSI , const char* szFileName,char* szFileNS )
{
	// 假定文件的大小不超过4GB,只处理低位
	int nFileSize = -1;
	FILE_INF fi[1];
	int nFiles = GetFileList(pSI, fi,1,szFileName );
	if( nFiles != 1 ) return -1;
	sprintf( szFileNS, "%s<%d bytes>",szFileName,fi[0].nFileSizeLow );
	nFileSize = fi[0].nFileSizeLow;
	return nFileSize;
}

int	DataConn( SOCKET& s, DWORD dwIp, WORD wPort, int nMode ) 
{
	// 创建一个socket
	s = socket( AF_INET,SOCK_STREAM,0 );
	if( s == INVALID_SOCKET )
	{
		 printf("Failed to get a socket %d\n", WSAGetLastError());
		 return -1;
	}

	struct sockaddr_in inetAddr;
	inetAddr.sin_family = AF_INET;
	if( nMode == MODE_PASV )
	{
		 inetAddr.sin_port = htons( wPort );
		 inetAddr.sin_addr.s_addr = dwIp;
	}
	else
	{ 
		inetAddr.sin_port = htons( DATA_FTP_PORT );
		inetAddr.sin_addr.s_addr = inet_addr(inet_ntoa(((sockaddr_in *)&addr)->sin_addr));
	}

	BOOL optval = TRUE;
	if( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,sizeof(optval)) == SOCKET_ERROR ) 
	{
		printf("Failed to setsockopt %d.\n",WSAGetLastError() );
		closesocket(s);
		return -1;
	}

	if( bind( s,(struct sockaddr*)&inetAddr,sizeof(inetAddr)) == SOCKET_ERROR )
	{
		printf("Failed to bind a socket %d.\n",WSAGetLastError() );
		closesocket(s);
		return -1;
	}

	if( MODE_PASV == nMode )
	{
		if( listen( s,SOMAXCONN ) == SOCKET_ERROR ) 
		{
			printf("Failed to listen a socket %d.\n",WSAGetLastError() );
			closesocket(s);
			return -1;
		}
	} 
	else if( MODE_PORT == nMode ) 
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port   = htons( wPort );
		addr.sin_addr.s_addr   = dwIp;
		if( connect( s, (const sockaddr*)&addr,sizeof(addr) ) == SOCKET_ERROR ) 
		{
			printf("Failed to connect a socket %d\n",WSAGetLastError() );
			closesocket( s );
			return -1;
		}
	}
	return 0;
}

int DataSend( SOCKET s, char* buff,int nBufSize )
{
	int nBytesLeft = nBufSize;
	int idx = 0, nBytes = 0;
	while( nBytesLeft > 0 ) {
		nBytes = send( s,&buff[idx],nBytesLeft,0);
		if( nBytes == SOCKET_ERROR )
		{
			printf("Failed to send buffer to socket %d\n",WSAGetLastError() );
			closesocket( s );
			return -1;
		}
		nBytesLeft -= nBytes;
		idx += nBytes;
	}
	return idx;
}
int DataRecv( LPSOCKET_INF &pSI, SOCKET s, const char* szFileName )
{
	return WriteToFile( pSI,s, szFileName );	
}

SOCKET DataAccept( SOCKET& s )
{
	SOCKET sAccept = accept( s ,NULL,NULL );
	if( sAccept != INVALID_SOCKET ) 
	{
		closesocket( s );
	}
	return sAccept;
}
char  reName[MAX_PATH];
int DealCommand( LPSOCKET_INF pSI )
{
	int nRetVal = 0;
	/*static SOCKET sAccept = INVALID_SOCKET;
	static SOCKET s       = INVALID_SOCKET;
	static BOOL   bPasv   = FALSE;*/

	char  szCmd[MAX_REQ_LEN]; 
	char  szCurrDir[MAX_PATH];
	strcpy(szCmd, pSI->buffRecv );
	if( strtok( szCmd," \r\n") == NULL ) return -1;
	strupr(szCmd );

	const char*  szOpeningAMode = "150 Opening ASCII mode data connection for ";
	static DWORD  dwIpAddr = 0;
	static WORD   wPort    = 0;
	// PORT n1,n2,n3,n4,n5,n6
	if( strstr(szCmd,"PORT") )
	{
		if( ConvertDotAddress( pSI->buffRecv+strlen("PORT")+1,&dwIpAddr,&wPort) == -1 ) 
			return -1;
		const char*  szPortCmdOK    = "200 PORT Command successful.\r\n";
		sprintf(pSI->buffSend,"%s",szPortCmdOK );
		if( SendRes( pSI ) == -1 ) return -1;
		//bPasv = FALSE;
		pSI->bPasv=false;
		return CMD_OK;
	}
	if( strstr( szCmd,"PASV") ) 
	{
		if( DataConn( pSI->send, htonl(INADDR_ANY), PORT_BIND, MODE_PASV ) == -1 )
			return -1;
		char *szCommaAddress = ConvertCommaAddress( inet_ntoa(((sockaddr_in *)&addr)->sin_addr),PORT_BIND );
		sprintf( pSI->buffSend,"227 Entering Passive Mode (%s).\r\n",szCommaAddress );
		if( SendRes( pSI ) == -1 ) 
			return -1;
		pSI->bPasv = true;
		return PASSIVE_MODE;		
	}
	if( strstr( szCmd, "NLST") || strstr( szCmd,"LIST") ) 
	{
		if( pSI->bPasv ) pSI->sAccept = DataAccept( pSI->send );
		if( !pSI->bPasv )
			sprintf(pSI->buffSend,"%s/bin/ls.\r\n",szOpeningAMode );
		else 
			strcpy(pSI->buffSend,"125 Data connection already open; Transfer starting.\r\n");		
		
		if( SendRes( pSI ) == -1 ) 
			return -1;
		// 取得文件列表信息，并转换成字符串
		BOOL bDetails = strstr(szCmd,"LIST")?TRUE:FALSE;
		char buff[DATA_BUFSIZE];
		UINT nStrLen = FileListToString( pSI,buff,sizeof(buff),bDetails);
		if( !pSI->bPasv ) 
		{
			if( DataConn( pSI->send,dwIpAddr,wPort,MODE_PORT ) == -1 )
				return -1;
			if( DataSend( pSI->send, buff,nStrLen ) == -1 )
				return -1;
			closesocket(pSI->send);
			pSI->use=true;
		}
		else 
		{
			DataSend( pSI->sAccept,buff,nStrLen );
			closesocket( pSI->sAccept );
		}
		sprintf( pSI->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;

		return TRANS_COMPLETE;
	}
	if( strstr( szCmd, "RETR") ) 
	{
		if( pSI->bPasv ) pSI->sAccept = DataAccept(pSI->send);	
		char szFileNS[MAX_PATH];
		char *szFile = strtok( NULL," \r\n" );
		int nFileSize = CombindFileNameSize( pSI,szFile,szFileNS );
		if( nFileSize == -1  ) 
		{
			sprintf( pSI->buffSend,"550 %s: 系统找不到指定的文件.\r\n",szFile);
			if( SendRes( pSI ) == -1 )
				return -1;
			if( !pSI->bPasv ) closesocket( pSI->sAccept );
			else closesocket( pSI->send );
			
			return CANNOT_FIND; 
		}
		else 
			sprintf(pSI->buffSend,"%s%s.\r\n",szOpeningAMode,szFileNS);

		if( SendRes( pSI ) == -1 )
			return -1;
	
		char* buff = new char[nFileSize];
		if( NULL ==buff ) 
		{
			printf("Not enough memory error!\n");
			return -1;
		}
		if( ReadFileToBuffer(pSI, szFile,buff, nFileSize ) == (DWORD)nFileSize ) 
		{
			// 处理Data FTP连接
			Sleep( 10 );
			if( pSI->bPasv ) 
			{
				DataSend( pSI->sAccept,buff,nFileSize );
				closesocket( pSI->sAccept );
			} 
			else 
			{
				if( DataConn( pSI->send,dwIpAddr,wPort,MODE_PORT ) == -1 )
					return -1;
				DataSend( pSI->send, buff, nFileSize );
				closesocket( pSI->send );
			}
		}
		if( buff != NULL )
			delete[] buff;

		sprintf( pSI->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;
		
				
		return TRANS_COMPLETE;
	}
	if( strstr( szCmd, "STOR") ) 
	{
		if( pSI->bPasv ) pSI->sAccept = DataAccept(pSI->send);		
		char *szFile = strtok( NULL," \r\n" );
		if( NULL == szFile ) return -1;
		sprintf(pSI->buffSend,"%s%s.\r\n",szOpeningAMode,szFile);
		if( SendRes( pSI ) == -1 )
			return -1;

		// 处理Data FTP连接
		if( pSI->bPasv ) 
			DataRecv( pSI,pSI->sAccept,szFile );
		else 
		{
			if( DataConn( pSI->send,dwIpAddr,wPort,MODE_PORT ) == -1 )
				return -1;
			DataRecv(pSI, pSI->send, szFile );
		}
		
		sprintf( pSI->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;
				
		return TRANS_COMPLETE;
	}
	if( strstr( szCmd,"QUIT" ) )
	{
		sprintf( pSI->buffSend,"%s","221 Good bye,欢迎下次再来.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;
		
		return FTP_QUIT; 
	}
	if( strstr( szCmd,"XPWD" ) || strstr( szCmd,"PWD") )	{
		strcpy(szCurrDir,pSI->directory);   //GetCurrentDirectoryA( MAX_PATH,szCurrDir );
		sprintf( pSI->buffSend,"257 \"%s\" is current directory.\r\n",RelativeDirectory(szCurrDir) );
		if( SendRes( pSI ) == -1 ) return -1;

		return CURR_DIR;
	}
	if( strstr( szCmd,"CWD" ) || strstr(szCmd,"CDUP") ) 
	{
		char *szDir = strtok( NULL,"\r\n" );
		if( szDir == NULL ) szDir = "\\";
		char szSetDir[MAX_PATH];
		if( strstr(szCmd,"CDUP") ) 
			strcpy(szSetDir,"..");
		 else 
			strcpy(szSetDir,AbsoluteDirectory( szDir ) );
		/*if( !SetCurrentDirectoryA( szSetDir ) ) 
		{
			sprintf(szCurrDir,"\\%s",szSetDir); 
			sprintf( pSI->buffSend,"550 %s No such file or Directory.\r\n",
					RelativeDirectory(szCurrDir) );
			nRetVal = CANNOT_FIND;
		} 
		else 
		{*/
			strcpy(pSI->directory,szSetDir);
			strcpy(szCurrDir,pSI->directory);
			//GetCurrentDirectoryA( MAX_PATH,szCurrDir );
			sprintf( pSI->buffSend,"250 Directory changed to %s.\r\n",RelativeDirectory(szCurrDir) );
			nRetVal = DIR_CHANGED;
		//}
		if( SendRes( pSI ) == -1 ) return -1;

		return nRetVal;
	}
	if( strstr( szCmd,"SYST" ) ) 
	{
		sprintf( pSI->buffSend,"%s","215 Windows_NT Version 4.0\r\n");
		if( SendRes( pSI ) == -1 ) return -1;
		return OS_TYPE;
	}
	if( strstr( szCmd,"TYPE") ) 
	{
		char *szType = strtok(NULL,"\r\n");
		if( szType == NULL ) szType = "A";
		sprintf(pSI->buffSend,"200 Type set to %s.\r\n",szType );
		if( SendRes( pSI ) == -1 ) 
			return -1;
		return CMD_OK;		
	}
	if( strstr( szCmd,"REST" ) )
	{
		sprintf( pSI->buffSend,"504 Reply marker must be 0.\r\n");
		if( SendRes( pSI ) == -1 ) 
			return -1;
		return REPLY_MARKER;		
	}
	if( strstr( szCmd,"NOOP") ) 
	{
		sprintf( pSI->buffSend,"200 NOOP command successful.\r\n");
		if( SendRes( pSI ) == -1 ) 
			return -1;
		return CMD_OK;		
	}
	if( strstr( szCmd,"DELE" )) 
	{
		char *file = strtok( NULL,"\r\n" );
		strcpy(szCurrDir,pSI->directory);   //GetCurrentDirectoryA( MAX_PATH,szCurrDir );
		if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
		strcat(szCurrDir,file);
		if(DeleteFileA((LPCSTR)szCurrDir))
			sprintf( pSI->buffSend,"250 %s has deleted.\r\n",file);
		else sprintf( pSI->buffSend,"250 %s has not deleted.\r\n",file);
		if( SendRes( pSI ) == -1 ) return -1;

		return 250;
	}
	if( strstr( szCmd,"MKD" )){
		char *file = strtok( NULL,"\r\n" );
		strcpy(szCurrDir,pSI->directory);   //GetCurrentDirectoryA( MAX_PATH,szCurrDir );
		if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
		strcat(szCurrDir,file);
		if(CreateDirectoryA((LPCSTR)szCurrDir,NULL))
			sprintf( pSI->buffSend,"250 %s has created.\r\n",file);
		else sprintf( pSI->buffSend,"250 %s has not created.\r\n",file);
			
		if( SendRes( pSI ) == -1 ) return -1;

		return 250;
	}
	if( strstr( szCmd,"RMD" )) {
		char *file = strtok( NULL,"\r\n" );
		strcpy(szCurrDir,pSI->directory);
		if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
		strcat(szCurrDir,file);
	// CString strResult;
	//	int nResult = theServer.m_UserManager.GetDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_DELETE, strResult);
		BOOL fbRet=FALSE;
		fbRet= RemoveDirectoryA((LPCSTR)szCurrDir);
		if(!fbRet){
			/*if (GetLastError() == ERROR_DIR_NOT_EMPTY)
					sprintf( pSI->buffSend,"550 Directory not empty.");
			else*/
				sprintf( pSI->buffSend,"450 Internal error deleting the directory.\r\n");
		}
		else{
			sprintf(pSI->buffSend,"250 Directory deleted successfully\r\n");
		}
		if( SendRes( pSI ) == -1 ) return -1;

		return 250;		
	}

	if( strstr( szCmd,"RNFR" )||strstr(szCmd,"RNTO")){
		char *file = strtok( NULL,"\r\n" );
		if(strstr(szCmd,"RNFR")){
			strcpy(reName,pSI->directory);//GetCurrentDirectoryA( MAX_PATH,reName );
			if(reName[strlen(reName)-1]!='\\')	strcat(reName,"\\");
			strcat(reName,file);
			sprintf( pSI->buffSend,"350 %s name copyd.\r\n",file);
		}
		else{
			strcpy(szCurrDir,pSI->directory);   //GetCurrentDirectoryA( MAX_PATH,szCurrDir );
			if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
			strcat(szCurrDir,file);
			if(rename(reName,szCurrDir)==0)
				sprintf( pSI->buffSend,"250 %s has renamed.\r\n",file);
			else sprintf( pSI->buffSend,"250 %s has not renamed.\r\n",file);
		}
		
		if( SendRes( pSI ) == -1 ) return -1;
		return 250;
	}
	//其余都是无效的命令
	sprintf(pSI->buffSend,"500 '%s' command not understand.\r\n",szCmd );
	if( SendRes( pSI ) == -1 ) return -1;	
	return nRetVal;
}

///////////////////////////////////////////////////////////////////////////////////////////
//其他函数
char* GetLocalAddress(){
    struct in_addr *pinAddr;
    LPHOSTENT	lpHostEnt;
	int			nRet;
	int			nLen;
	char        szLocalAddr[80];
	memset( szLocalAddr,0,sizeof(szLocalAddr) );
	// Get our local name
    nRet = gethostname(szLocalAddr,sizeof(szLocalAddr) );
	if (nRet == SOCKET_ERROR)
	{
		return NULL;
	}
	// "Lookup" the local name
	lpHostEnt = gethostbyname(szLocalAddr);
    if (NULL == lpHostEnt)	
	{
		return NULL;
	}
    // Format first address in the list
	pinAddr = ((LPIN_ADDR)lpHostEnt->h_addr);
	nLen = strlen(inet_ntoa(*pinAddr));
	if ((DWORD)nLen > sizeof(szLocalAddr))
	{
		WSASetLastError(WSAEINVAL);
		return NULL;
	}
	return inet_ntoa(*pinAddr);
}

int GetFileList(LPSOCKET_INF &pSI, LPFILE_INF pFI, UINT nArraySize, const char* szPath  )
{
	WIN32_FIND_DATAA  wfd;
	int idx = 0;
	CHAR lpFileName[MAX_PATH];
	strcpy(lpFileName,pSI->directory);  //GetCurrentDirectoryA( MAX_PATH,lpFileName );
	if(lpFileName[strlen(lpFileName)-1]!='\\')	strcat( lpFileName,"\\" );
	strcat( lpFileName, szPath );
	HANDLE hFile = FindFirstFileA( lpFileName, &wfd );
	if ( hFile != INVALID_HANDLE_VALUE ) 
	{
		pFI[idx].dwFileAttributes = wfd.dwFileAttributes;
		lstrcpyA( pFI[idx].szFileName, wfd.cFileName );
		pFI[idx].ftCreationTime = wfd.ftCreationTime;
		pFI[idx].ftLastAccessTime = wfd.ftLastAccessTime;
		pFI[idx].ftLastWriteTime  = wfd.ftLastWriteTime;
		pFI[idx].nFileSizeHigh    = wfd.nFileSizeHigh;
		pFI[idx++].nFileSizeLow   = wfd.nFileSizeLow;
		while( FindNextFileA( hFile,&wfd ) && idx < (int)nArraySize ) 
		{
			pFI[idx].dwFileAttributes = wfd.dwFileAttributes;
			lstrcpyA( pFI[idx].szFileName, wfd.cFileName );
			pFI[idx].ftCreationTime = wfd.ftCreationTime;
			pFI[idx].ftLastAccessTime = wfd.ftLastAccessTime;
			pFI[idx].ftLastWriteTime  = wfd.ftLastWriteTime;
			pFI[idx].nFileSizeHigh    = wfd.nFileSizeHigh;
			pFI[idx++].nFileSizeLow   = wfd.nFileSizeLow;
		}
		FindClose( hFile );
	}
	return idx;
}
char* HostToNet( char* szPath ) 
{
	int idx = 0;
	if( NULL == szPath ) return NULL;
	//strlwr( szPath );
	while( szPath[idx] ) 
	{ 
		if( szPath[idx] == '\\' )
			szPath[idx] = '/';
		idx ++;
	}
	return szPath;
}

char* NetToHost( char* szPath )
{
	int idx = 0;
	if( NULL == szPath ) return NULL;
	//strlwr( szPath );
	while( szPath[idx] ) 
	{ 
		if( '/' == szPath[idx]  )
			szPath[idx] = '\\';
		idx ++;
	}
	return szPath;
}
char* RelativeDirectory( char* szDir )
{
	int nStrLen = strlen(ftproot);
	if( !strnicmp( szDir,ftproot, nStrLen )) 
		szDir += nStrLen;

	if( szDir && szDir[0] == '\0' ) szDir = "/";
	return HostToNet(szDir);
}
char* AbsoluteDirectory( char* szDir )
{
	char szTemp[MAX_PATH];
	strcpy( szTemp,ftproot);
	if( NULL == szDir ) return NULL;
	if( '/' == szDir[0] )
		strcat( szTemp, szDir );
	szDir = szTemp ;	
	return NetToHost(szDir);
}