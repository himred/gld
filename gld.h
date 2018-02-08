#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <pwd.h>
#include <grp.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif


#ifdef HAVE_SYS_TIME_H
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif
#endif

#define TOUT    10			// Network timeout in seconds
#define BLEN    2048			// Our buffer size
#define QLEN    20480			// Our Query Size must be > than 6*BLEN
#define NLEN	32			// Max # of networks in our table.
#define REQ     "smtpd_access_policy"	// The string to be matched
#define CONF	"/etc/gld.conf"		// The default config file

#define MSGGREYLIST	0
#define MSGLOCALWL	1
#define MSGDNSWL	2

#define VERSION "1.7"

typedef struct network
{
	unsigned long netw;
	unsigned long mask;
}network;

typedef struct config
{
	int port;			// Port to listen to
	int maxcon;			// max # of connections
	long mini;			// Minimum time for greylist
	int syslog;			// Shall we write to the syslog
	int accept;			// Shall we return OK in case of error
	int whitelist;			// Shall we lookup the whitelist table
	int light;			// Shall we use light greylisting ?
	int facility;			// Syslog facility to use
	int loopback;			// Shall we bind only to loopback
	int mxgrey;			// Shall we use the mexgrey algorithm ?
	int debug;			// Shall we display debug informations
	int training;			// Shall we activate training mode
	char dnswl[512];		// The domain to use if we do DNSWL
	char message[512];		// The text we display
	char sqlhost[128];		// SQL server
	char sqluser[128];		// SQL User
	char sqlpasswd[128];		// SQL password
	char sqldb[128];		// SQL Database name
	char user[128];			// The user we setuid to
	char grp[128];			// The group we setgid to
	network nets[NLEN];		// The networks allowed to connect
	int nbnet;			// # of networks in nets
}config;

//
// Prototypes
//

int ReadConfig(char *file,config *conf);
int HandleChild(int s,config *cnf);
void TheEnd(int s);
int SQLConnect(char *host,char *user,char *passwd,char *db);
void SQLClose(void);
long SQLQuery(char *q);
int GreyList(char *ip,char *sender,char *recipient,config *conf);
void Log(config *conf,char * recipient,char *sender,char *ip,int white);
void ErrorLog(config *conf,char *msg);
void Reload(int s);
void ShowBaseInfo(void);
void Quote(char *str);
int ReadClients(config *conf,char *str);
unsigned long Hash(char *ip);
int CheckIP(config *conf,char *ip);
unsigned long CidrMsk(int msk);
int MyDaemon(int nochdir, int noclose);
