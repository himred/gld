/************************************************/
/*                                              */
/*             TCP/UDP sockets                  */
/*                                              */
/************************************************/
/*                                              */
/* Version : 2.4  (02/01/01)                    */
/*                                              */
/************************************************/
/*                                              */
/* Author: Salim Gasmi    (salim@gasmi.net)     */
/*                                              */
/************************************************/
/*                                              */
/* Suppported OS :                              */
/*                                              */
/* Standard Unix : -D UNIX (Linux,HPUX,OSF...)  */
/* AIX Unix      : -D AIX                       */
/* SunOs/Solaris : -D SUN (link -lnsl -lsockets)*/
/* BeOs          : -D BEOS                      */
/* AmigaOs       : -D AMIGAOS                   */
/*                                              */
/************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sockets.h"

#ifdef AMIGAOS

/************************************************/
/*                                              */
/* Open & CloseSocketLib (Amiga Only)		*/ 
/*                                              */
/************************************************/
/*                                              */
/*  0          : Okay	           		*/   
/*  S_LIB_ERR  : Impossible d'ouvrir la lib     */
/*                                              */
/************************************************/

struct Library * SocketBase;
 
int OpenSocketLib()
{
if ((SocketBase=(struct Library *)OpenLibrary(SOCKLIB,SOCKVER))==NULL)
                return(S_SLIB_ERR);
 
else return(0);
 
}
 
void CloseSocketLib()
{
CloseLibrary(SocketBase);
}

#endif

/************************************************/
/*                                              */
/* OpenTcpSocket : Se connecte sur un port TCP  */
/*                                              */
/************************************************/
/*                                              */
/* host   : Host name du Server                 */
/* port   : Port TCP a se connecter             */
/*                                              */
/************************************************/
/*                                              */
/*  >=0        : Socket de connexion      	*/
/*  S_HOST_ERR : le serveur n'existe pas      	*/
/*  S_PORT_ERR : le port ne reponds pas       	*/
/*  S_SOCK_ERR : Erreur creation socket       	*/
/*                                              */
/************************************************/

int OpenTcpSocket(char *host, int port)
{
struct	sockaddr_in	sock_addr;
struct	hostent		*host_struct;
int	s;

#ifdef AMIGAOS
if(SocketBase==NULL) OpenSocketLib();
#endif

if ((host_struct=(struct hostent *)gethostbyname(host))==NULL)
		return(S_HOST_ERR);

if ((s=socket(AF_INET, SOCK_STREAM, 0)) < 0) return(S_SOCK_ERR);

bzero(&sock_addr, sizeof(sock_addr));
bcopy(host_struct->h_addr, (char *)&sock_addr.sin_addr, host_struct->h_length);
sock_addr.sin_port 	= 	htons((u_short)port);
sock_addr.sin_family	=	host_struct->h_addrtype;

if (connect(s, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
		return(S_PORT_ERR);


return(s);
}


/************************************************/
/*                                              */
/* ReadSocket : lit n octets sur un socket TCP 	*/  
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*  buffer     : le buffer a remplir            */
/*  bufsize    : le nb d'octets a lire          */
/*  tout       : le timeout reseau en s         */
/*                                              */
/************************************************/
/*                                              */
/*  >=0        : le nb d'octets lus             */   
/*  S_WAIT_ERR : Erreur de Wait                 */
/*  S_RECV_ERR : Erreur de recv        		*/
/*  S_TIME_ERR : Erreur de timeout        	*/
/*  S_SOCK_ERR : Socket passe en arg invalide	*/
/*                                              */
/************************************************/

int ReadSocket(int s, char *buffer, int bufsize,int tout)
{
int 	nbytes=0, 
	nfds=0;

fd_set readfds;
fd_set excepfds;

struct timeval timeout;

if(s<0) return(S_SOCK_ERR);

	timeout.tv_sec=tout;
	timeout.tv_usec=0;

	FD_ZERO( &readfds );
	FD_SET(s, &readfds );

	FD_ZERO( &excepfds );

	if( (nfds = select(s + 1, &readfds, NULL, NULL, &timeout )) < 0)
       		return(S_WAIT_ERR);

	if (nfds > 0)
	{
		if ((nbytes=recv(s,buffer,bufsize,0)) < 0) return(S_RECV_ERR);
		else return(nbytes);
	}
	else return(S_TIME_ERR);
}


/************************************************/
/*                                              */
/* WriteSocket:ecrit n octets sur un socket TCP */
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*  buffer     : le buffer a utiliser           */
/*  bufsize    : le nb d'octets a ecrire        */
/*  tout       : le timeout reseau en s         */
/*                                              */
/************************************************/
/*                                              */
/*  0          : OK                             */   
/*  S_WAIT_ERR : Erreur de Wait                 */
/*  S_SEND_ERR : Erreur de send                 */
/*  S_TIME_ERR : Erreur de timeout              */
/*  S_SOCK_ERR : Socket passe en arg invalide   */
/*                                              */
/************************************************/

int WriteSocket(int s, char *buffer, int bufsize, int tout)
{
int 	nbytes=0, 
	nfds=0;

fd_set writefds;


struct timeval timeout ;

if(s<0) return(S_SOCK_ERR);

timeout.tv_sec=tout;
timeout.tv_usec=0;

	while (1)	
	{
		FD_ZERO( &writefds );
		FD_SET(s, &writefds );

		if( (nfds = select(s + 1, NULL, &writefds, NULL, &timeout )) < 0) return(S_WAIT_ERR);

		if (nfds > 0)
		{
			if ((nbytes=send(s,buffer,bufsize,0)) < 0) return(S_SEND_ERR);
			buffer+=nbytes;
			bufsize-=nbytes;
			if (bufsize==0)
				return(0);
		}
		else return(S_WAIT_ERR);
	}
}


/************************************************/
/*                                              */
/* ReadLSocket: lit une ligne sur un socket TCP */
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*  buf        : le buffer a remplir            */
/*  len        : le nb d'octets Maxi a lire     */
/*  tout       : le timeout reseau en s         */
/*                                              */
/************************************************/
/*                                              */
/*  0          : Ligne lue OK                   */
/*  S_WAIT_ERR : Erreur de Wait                 */
/*  S_RECV_ERR : Erreur de recv                 */
/*  S_TIME_ERR : Erreur de timeout              */
/*  S_SOCK_ERR : Socket passe en arg invalide   */
/*                                              */
/************************************************/


int ReadLSocket(int s, char *buf,int len,int tout)
{
int nfds=0;
fd_set readfds;
int ret=0;

struct timeval timeout ;

if(s<0) return(S_SOCK_ERR);

timeout.tv_sec=tout;
timeout.tv_usec=0;

    while (--len)
    {

	FD_ZERO( &readfds );
        FD_SET(s, &readfds );

	ret=0;
       	if((nfds = select(s + 1, &readfds, NULL, NULL, &timeout )) < 0) 
		{
		ret=S_WAIT_ERR;
		break;
		}

	if(nfds==0) 
		{
		ret=S_TIME_ERR;
		break;
		}

        if (recv(s, buf, 1,0) != 1) 
		{
		ret=S_RECV_ERR;
		break;
		}

        if (*buf == '\n') 
		{
		ret=0;
		break;
		}

        if (*buf != '\r' && *buf!=0) buf++;
    }
    *buf = 0;
    return(ret);
}

/************************************************/
/*                                              */
/* WriteLSocket: ecrit une ligne sur socket TCP */
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*  lin        : la ligne a ecrire              */
/*  tout       : le timeout reseau en s         */
/*                                              */
/************************************************/
/*                                              */
/*  >=0        : le nb d'octets ecrits          */
/*  S_WAIT_ERR : Erreur de Wait                 */
/*  S_SEND_ERR : Erreur de recv                 */
/*  S_TIME_ERR : Erreur de timeout              */
/*  S_SOCK_ERR : Socket passe en arg invalide   */
/*                                              */
/************************************************/


int WriteLSocket(int s, char *line,int tout)
{
int 	nbytes=0, 
	ntot=0,
	ntottemp=0,
	nfds=0;

fd_set writefds;

struct timeval timeout ;

if(s<0) return(S_SOCK_ERR);

timeout.tv_sec=tout;
timeout.tv_usec=0;

if ((ntot=strlen(line))==0) return(-4);

ntottemp=ntot;

	while (1)	
	{
		FD_ZERO( &writefds );
		FD_SET(s, &writefds );

		if( (nfds = select(s + 1, NULL, &writefds, NULL, &timeout )) < 0) 
				return(S_WAIT_ERR);

		if (nfds > 0)
		{
			if ((nbytes=send(s,line,strlen(line),0)) < 0) return(S_SEND_ERR);
			line+=nbytes;
			ntot-=nbytes;
			if (ntot==0)
				{
				if(send(s,"\r\n",2,0) !=2) return(S_SEND_ERR);
				return(ntottemp);
				}
		}
		else return(S_TIME_ERR);
	}
}

/************************************************/
/*                                              */
/* OpenUdpSocket : Se connecte sur un port UDP  */
/*                                              */
/************************************************/
/*                                              */
/* host   : Host name du Server                 */
/* port   : Port UDP a se connecter             */
/* addr   : struct Udp a conserver pour ecrire	*/
/*                                              */
/************************************************/
/*                                              */
/*  >=0        : Socket de connexion      	*/
/*  S_HOST_ERR : le serveur n'existe pas      	*/
/*  S_PORT_ERR : le port ne reponds pas       	*/
/*  S_SOCK_ERR : Erreur creation socket       	*/
/*  S_BIND_ERR : Erreur bind local       	*/
/*                                              */
/************************************************/

int OpenUdpSocket(char *host, int port,Udp *addr)
{
struct	sockaddr_in	local_addr;
struct	hostent		*host_struct;
int	s;

#ifdef AMIGAOS
if(SocketBase==NULL) OpenSocketLib();
#endif

if ((host_struct=(struct hostent *)gethostbyname(host))==NULL)
		return(S_HOST_ERR);

if ((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0) return(S_SOCK_ERR);

bzero(addr, sizeof(Udp));
bcopy(host_struct->h_addr, (char *)&addr->sin_addr, host_struct->h_length);
addr->sin_port 	= 	htons((ushort)port);
addr->sin_family	=	host_struct->h_addrtype;

/* Now we bind a local port since it is a connectionless protocol */
/* bind is done on localhost + dynamic free port for response port */
/* Some systems does this by default and binding is not needed */
/* Anyway I *ALWAYS* do it, in case of an old system ....  */

bzero(&local_addr,sizeof(local_addr));
local_addr.sin_family	=	AF_INET;
local_addr.sin_addr.s_addr=	htonl(INADDR_ANY);
local_addr.sin_port 	= 	htons(0);

if(bind(s,(struct sockaddr *)&local_addr,sizeof(local_addr))<0)
	{
	close(s);
	return(S_BIND_ERR);
	}

return(s);
}

/************************************************/
/*                                              */
/* SendUdpData:ecrit n octets sur un socket UDP */
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*  addr       : structure Udp destination	*/
/*  buffer     : le buffer a remplir            */
/*  size       : le nb d'octets a ecrire        */
/*                                              */
/************************************************/
/*                                              */
/*  0          : OK                             */
/*  S_SEND_ERR : Erreur de sendto               */
/*  S_SOCK_ERR : Socket passe en arg invalide   */
/*                                              */
/************************************************/

int SendUdpData(int s,Udp *addr,char *buffer,int size)
{

if(s<0) return(S_SOCK_ERR);

#ifdef SUN
if(sendto(s,buffer,size,0,(struct sockaddr *)addr,sizeof(struct sockaddr_in))!=size)
#else
if(sendto(s,buffer,size,0,(const struct sockaddr *)addr,sizeof(struct sockaddr_in))!=size)
#endif
	return(S_SEND_ERR);

return(0);
}

/************************************************/
/*                                              */
/* ReadUdpData: lit n octets sur un socket UDP  */
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*  buffer     : le buffer a remplir            */
/*  maxsize    : le nb d'octets max a lire      */
/*  tout       : timeout en secondes 		*/
/*                                              */
/************************************************/
/*                                              */
/*  >=0        : le nb d'octets lus             */
/*  S_RECV_ERR : Erreur de recv                 */
/*  S_SOCK_ERR : Socket passe en arg invalide   */
/*  S_WAIT_ERR : Erreur de wait			*/
/*  S_TIME_ERR : Erreur de timeout		*/
/*                                              */
/************************************************/

int ReadUdpData(int s,char *buffer,int maxsize,int tout)
{
int     nbytes=0,
        nfds=0;

fd_set readfds;
fd_set excepfds;

struct timeval timeout;

if(s<0) return(S_SOCK_ERR);

timeout.tv_sec=tout;
timeout.tv_usec=0;

FD_ZERO( &readfds );
FD_SET(s, &readfds );

FD_ZERO( &excepfds );

if( (nfds = select(s + 1, &readfds, NULL, NULL, &timeout )) < 0)
            return(S_WAIT_ERR);

if (nfds > 0)
    {
           nbytes=recvfrom(s,buffer,maxsize,0,(struct sockaddr *)0,(int *)0);
           if(nbytes<0) return(S_RECV_ERR); else return(nbytes);
    }
    else return(S_TIME_ERR);
}

/************************************************/
/*                                              */
/* CloseSocket : ferme un socket                */
/*                                              */
/************************************************/
/*                                              */
/*  s          : le socket                      */
/*                                              */
/************************************************/
/*                                              */
/*  0          : OK	             		*/
/*  S_CLOS_ERR : Erreur de close                */
/*  S_SHUT_ERR : Erreur de shutdown             */
/*                                              */
/************************************************/

int CloseSocket(int s)
{
	if ( (s && close(s)) < 0 ) return(S_CLOS_ERR);

/*	if ((s && shutdown(s,2)) < 0) return(S_SHUT_ERR); */

	return(0);
}


/************************************************/
/*                                              */
/* DnsIp : recupere la 1ere IP d'un hostname    */
/*                                              */
/************************************************/
/*                                              */
/*  host       : le nom du host                 */
/*  ip         : le buffer a remplir avec l'IP  */
/*                                              */
/************************************************/
/*                                              */
/*  0          : OK             		*/
/*  S_HOST_ERR : Le host n'existe pas           */
/*  S_INET_ERR : Host non de la famille INET    */
/*                                              */
/************************************************/

int DnsIp(char *host,char *ip)
{
struct hostent *hostptr;
struct in_addr *ptr;


if((hostptr=(struct hostent *)gethostbyname(host))==NULL) return(S_HOST_ERR);

if(hostptr->h_addrtype != AF_INET) return(S_INET_ERR);

ptr=(struct in_addr *) *hostptr->h_addr_list;

if(ip!=NULL) strcpy(ip,(char *)inet_ntoa(*ptr));
return(0);
}

/************************************************/
/*                                              */
/* DnsFQDN : recupere le FQDN d'un hostname     */
/*                                              */
/************************************************/
/*                                              */
/*  host       : le nom du host                 */
/*  fqdn       : le buffer a remplir avec FQDN  */
/*                                              */
/************************************************/
/*                                              */
/*  0          : OK                             */
/*  S_HOST_ERR : Le host n'existe pas           */
/*                                              */
/************************************************/

int DnsFQDN(char *host,char *fqdn)
{
struct hostent *hostptr;

if((hostptr=(struct hostent *)gethostbyname(host))==NULL) return(S_HOST_ERR);

strcpy(fqdn,hostptr->h_name);
return(0);

}

/************************************************/
/*                                              */
/* DnsName : recupere le FQDN d'une IP dotted   */
/*                                              */
/************************************************/
/*                                              */
/*  host       : dotted ip                      */
/*  fqdn       : le buffer a remplir avec FQDN  */
/*                                              */
/************************************************/
/*                                              */
/*  0          : OK                             */
/*  S_HOST_ERR : L'IP n'existe pas              */
/*                                              */
/************************************************/

int DnsName(char *ip,char *fqdn)
{

struct hostent *hostptr;
struct in_addr addr;

addr.s_addr=inet_addr(ip);

if((hostptr=(struct hostent *)gethostbyaddr((char *)&addr,sizeof(struct in_addr),AF_INET))==NULL) return(S_HOST_ERR);

strcpy(fqdn,hostptr->h_name);

return(0);
}

/************************************************/
/*                                              */
/* GetPeerIp : recupere le FQDN+Dotted IP  	*/
/* du peer connecte au socket sock		*/
/*                                              */
/************************************************/
/*                                              */
/*  sock       : socket                         */
/*  ipfrom     : le buffer a remplir avec l'IP  */
/*  hostfrom   : le buffer a remplir avec FQDN  */
/*                                              */
/************************************************/
/*                                              */
/*                 AUCUNE                       */
/*                                              */
/************************************************/

void GetPeerIp(int sock,char *ipfrom,char *hostfrom)
{
struct sockaddr_in from;
size_t foo=sizeof(struct sockaddr_in);
struct hostent *hostptr;

strcpy(ipfrom,"???.???.???.???");
strcpy(hostfrom,"?????");

if (getpeername(sock,(struct sockaddr *)&from, &foo) == 0)
     {
     strcpy(ipfrom,(char *)inet_ntoa(from.sin_addr));
     hostptr=(struct hostent *)gethostbyaddr((char *)&from.sin_addr,sizeof(struct in_addr),AF_INET);
     if(hostptr!=NULL) strcpy(hostfrom,hostptr->h_name);
     }
}

/************************************************/
/*                                              */
/* TcpServer: Cree un serveur Tcp       	*/
/*                                              */
/************************************************/
/*                                              */
/*  port       : port TCP a ecouter		*/
/*  mxcon      : nb maxi de connections		*/
/*  loopback   : 0 ou 1 (1=loopback bind)	*/
/*                                              */
/************************************************/
/*                                              */
/* serv.sd >=0 : descripteur du serveur		*/
/* serv.sd= -1 : Impossible de creer le serveur	*/  
/*                                              */
/************************************************/

TcpServer OpenTcpServer(int port,int mxcon,int loopback)
{
TcpServer serv;
int one=1;

serv.sd=-1;

bzero(&(serv.sin), sizeof(serv.sin));
serv.sin.sin_family = AF_INET;
serv.sin.sin_port = htons((u_short)port);
if(loopback==1) serv.sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
else serv.sin.sin_addr.s_addr = htonl(INADDR_ANY);

if ((serv.sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return(serv);

setsockopt(serv.sd,SOL_SOCKET,SO_REUSEADDR,(void *)&one,sizeof(int));


/* we bind it to the host */

if (bind(serv.sd, (struct sockaddr *)&(serv.sin),sizeof(serv.sin)) < 0) 
{
	close(serv.sd);
	serv.sd=-1;
	return(serv);
}

/* our socket can handle mxcon connections at a time */

if(listen(serv.sd,mxcon)<0)
	{
	close(serv.sd);
	serv.sd=-1;
	return(serv);
	}

return(serv);
}

/************************************************/
/*                                              */
/* WaitTcpServer : attend connexion sur serveur */
/*                                              */
/************************************************/
/*                                              */
/*  serv         : Descripteur de serveur       */
/*                                              */
/************************************************/
/*                                              */
/* >=0 : descripteur du nouveau socket		*/
/*  -1 : Erreur					*/
/*                                              */
/************************************************/

int WaitTcpServer(TcpServer serv)
{

size_t foo=sizeof(serv.sin);
return(accept(serv.sd,(struct sockaddr *)&(serv.sin),&foo));
}

/************************************************/
/*                                              */
/* CloseTcpServer : arrete le  serveur 		*/
/*                                              */
/************************************************/
/*                                              */
/*  serv         : Descripteur de serveur       */
/*                                              */
/************************************************/
/*                                              */
/*  	RIEN                                 	*/
/*                                              */
/************************************************/

void CloseTcpServer(TcpServer serv)
{
close(serv.sd);
}

/************************************************/
/*                                              */
/* NoZombies: Empeche la creation de zombies    */
/* Quand on forke en System V                   */
/*                                              */
/************************************************/
/*                                              */
/* 		RIEN                            */
/*                                              */
/************************************************/
/*                                              */
/*      RIEN                                    */
/*                                              */
/************************************************/

void NoZombies(int sig)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}
