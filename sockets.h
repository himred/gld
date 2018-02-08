/************************************************/
/*                                              */
/*           Headers TCP/IP                     */
/*                                              */
/************************************************/
/*                                              */
/* Version : 2.3  (23/06/00)			*/
/*                                              */
/************************************************/
/*                                              */
/* Author: Salim Gasmi    (salim@gasmi.net)	*/
/*						*/
/************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_NETDB_H
#include <strings.h>
#endif

#ifdef HAVE_SYS_TIME_H
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif
#endif

#define		Udp			struct sockaddr_in
#define		USHORT			ushort

#define		S_OK			 0			/* Ok ! 		*/
#define 	S_HOST_ERR		-100			/* Host unknown		*/
#define		S_SOCK_ERR		-101			/* socket error		*/
#define		S_PORT_ERR		-102			/* connect error	*/
#define		S_SLIB_ERR		-103			/* bsd lib error	*/
#define		S_WAIT_ERR		-104			/* Select error		*/
#define		S_RECV_ERR		-105			/* Recv() error		*/
#define		S_TIME_ERR		-106			/* Timemout error	*/
#define		S_SEND_ERR		-107			/* Send() error		*/
#define		S_CLOS_ERR		-108			/* close error		*/
#define		S_SHUT_ERR		-109			/* shutdown error	*/
#define		S_BIND_ERR		-110			/* local bind error	*/
#define		S_INET_ERR		-111			/* not an INET host	*/

#define		S_IMPL_ERR		-999			/* not yet implemanted	*/


#ifdef AMIGAOS

#define 	gethostbyname 		GetHostByName
#define 	gethostbyaddr 		GetHostByAddr
#define		connect			Connect
#define		socket			Socket
#define		shutdown		Shutdown
#define		send			Send
#define		recv			Recv
#define		recvfrom		RecvFrom
#define		sendto			SendTo
#define 	select(a,b,c,d,e)	WaitSelect(a,b,c,d,e,0)
#define		bind			Bind
#define		listen			Listen
#define		accept			Accept
#define		inet_ntoa		Inet_NtoA
#define		inet_addr		Inet_Addr
#define		htons			HtoNs
#define		getpeername		GetPeerName
#define		uname			Uname
#define 	SOCKLIB			"bsdsocket.library"
#define		SOCKVER			 4

int 		OpenSocketLib(void);
void 		CloseSocketLib(void);

#endif

/**************************** mes structures ***********************/

typedef struct TcpServer
	{
		int 			sd;
		struct sockaddr_in	sin;

	} TcpServer;


/******************************* prototypes **********************/

/* TCP functions */

int OpenTcpSocket(char *host, int port);
int ReadSocket(int sock, char *buff, int size,int timeout);
int WriteSocket(int sock, char *buff, int size, int timeout);
int ReadLSocket(int sock, char * buff,int maxsize,int timeout);
int WriteLSocket(int sock, char *buff,int timeout);

TcpServer OpenTcpServer(int port ,int maxcon,int loopback);
int 	  WaitTcpServer(TcpServer server);
void 	  CloseTcpServer(TcpServer server);

/* UDP Functions */

int OpenUdpSocket(char *host, int port,Udp * udp);
int SendUdpData(int sock,Udp * udp,char *buff,int size);
int ReadUdpData(int sock,char *buff,int maxsize,int timeout);

/* Generic Functions */

int CloseSocket(int sock);

/* DNS functions */

int DnsIp(char *host,char *ip);
int DnsFQDN(char *host,char *fqdn);
int DnsName(char *ip,char *fqdn);
void GetPeerIp(int sock,char *ip,char *fqdn);

/* Special Functions */

void NoZombies(int);

/* End of sockets.h */

