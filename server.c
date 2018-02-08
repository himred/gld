#include "gld.h"
#include "sockets.h"

TcpServer srv;
config conf;

int main(int argc,char **argv)
{
int s;
int cid;
int c;
char clean=0;
char query[QLEN];
int status;
struct group *grp;
struct passwd *user;

if(argc==2 && strcmp(argv[1],"-v")==0)
	{
	printf("gld %s <salim@gasmi.net> <http://www.gasmi.net>\n",VERSION);
	exit(0);
	}

if(argc==3 && strcmp(argv[1],"-c")==0) clean=1;
if(argc==3 && strcmp(argv[1],"-C")==0) clean=2;
if(argc==3 && strcmp(argv[1],"-k")==0) clean=3;
if(argc==3 && strcmp(argv[1],"-K")==0) clean=4;
if(argc==2 && strcmp(argv[1],"-i")==0) clean=5;

if(argc!=1 && clean==0 && strcmp(argv[1],"-d")!=0)
	{
	printf("Usage: gld [-c <n>|-C <n>|-k <n>|-K <n>|-h|-v]\n");
	printf(" gld -c <n> : clean the database for ALL entries not updated since <n> days\n");
	printf(" gld -C <n> : show what the -c option would do, without doing it\n");
	printf(" gld -k <n> : clean the database for entries not updated since <n> days with only one hit \n");
	printf(" gld -K <n> : show what the -k option would do, without doing it\n");
	printf(" gld -i     : show some database informations\n");
	printf(" gld -d     : enable debug mode\n");
	printf(" gld -v     : display version\n");
	printf(" gld -h     : display Usage\n");
	exit(1);
	}

if(ReadConfig(CONF,&conf)!=0)
	{
	printf("Invalid config file %s\n",CONF);
	exit(2);
	}

if(argc==2 && strcmp(argv[1],"-d")==0) conf.debug=1;

signal(SIGTERM,TheEnd);
signal(SIGHUP,Reload);
signal(SIGCHLD,NoZombies);

//
// Here we drop privileges and setuid/setgid if needed
//

if(conf.grp[0]!=0)
	{
	grp=getgrnam(conf.grp);
	if(grp==(struct group *)NULL)
		{
		printf("Group %s not found, please check the GROUP variable in gld.conf\n",conf.grp);
		exit(10);
		}

	if(setgid(grp->gr_gid)!=0)
		{
		printf("Unable to setgid to %s\n",conf.grp);
		exit(11);
		}

	if(conf.debug==1) printf("setgid to %s OK\n",conf.grp);
	}

if(conf.user[0]!=0)
	{
	user=getpwnam(conf.user);
	if(user==(struct passwd *)NULL)
		{
		printf("User %s not found, please check the USER variable in gld.conf\n",conf.user);
		exit(10);
		}

	if(setuid(user->pw_uid)!=0)
		{
		printf("Unable to setuid to %s\n",conf.user);
		exit(12);
		}

	if(conf.debug==1) printf("setuid to %s OK\n",conf.user);
	}


//
// Now we do what we have to do
//


if(clean!=0)
	{
	if(SQLConnect(conf.sqlhost,conf.sqluser,conf.sqlpasswd,conf.sqldb)<0)
		{
		printf("Unable to connect to MYSQL\n");
		exit(1);
		}

	if(clean==5)
		{
		ShowBaseInfo();
		SQLClose();
		exit(0);
		}

	if(clean==1 || clean==2)
	snprintf(query,sizeof(query)-1,"select count(last) from greylist where last < UNIX_TIMESTAMP()-86400*%d",atoi(argv[2]));

	if(clean==3 || clean==4)
	snprintf(query,sizeof(query)-1,"select count(last) from greylist where last < UNIX_TIMESTAMP()-86400*%d AND n=1",atoi(argv[2]));

        c=SQLQuery(query);

	if(clean==2 || clean==4)
		{
		printf("I would clean %d entries older than %d days\n",c,atoi(argv[2]));
		SQLClose();
		exit(0);
		}

	if(clean==1) snprintf(query,sizeof(query)-1,"delete from greylist where last < UNIX_TIMESTAMP()-86400*%d",atoi(argv[2]));
	if(clean==3) snprintf(query,sizeof(query)-1,"delete from greylist where last < UNIX_TIMESTAMP()-86400*%d and n=1",atoi(argv[2]));
        SQLQuery(query);
	SQLClose();

	printf("Cleaned %d entries older than %d days\n",c,atoi(argv[2]));
        exit(0);
	}


//
// Ok, here we start the server
//


srv=OpenTcpServer(conf.port,conf.maxcon,conf.loopback);
if(srv.sd==-1)
	{
	printf("Unable to bind to port %d\n",conf.port);
	perror("Error was: ");
	exit(1);
	}

if(conf.debug==1) printf("bind to port %d succesful\n",conf.port);

if(conf.syslog==1) ErrorLog(&conf,"gld started, up and running");
if(conf.debug==0) MyDaemon(0,0);
if(conf.debug==1) printf("Waiting for incoming connexions\n");

//
// The main loop
//
while(1==1)
	{
	s=WaitTcpServer(srv);
	if(s>=0)
		{
		cid=fork();
		if(cid < 0 && conf.syslog==1) ErrorLog(&conf,"Fork returned error code, no child");
		if(cid==0)
        		{
			c=HandleChild(s,&conf);
			if(c!=0 && conf.accept==1)
				WriteSocket(s,"action=dunno\n\n",14,TOUT);

			close(s);
			waitpid(-1, &status, WNOHANG);
			exit(0);
        		}
		close(s);
		}
	}

CloseTcpServer(srv);
exit(0);
}

int HandleChild(int s,config *cnf)
{
char buff[BLEN];
char request[BLEN];
char sender[BLEN];
char recipient[BLEN];
char ip[BLEN];
int n;
long ts;
int pid;

pid=getpid();

if(SQLConnect(cnf->sqlhost,cnf->sqluser,cnf->sqlpasswd,cnf->sqldb)<0)
	{
	if(cnf->debug==1) printf("%d: Unable to connect to MYSQL\n",pid);
	if(cnf->syslog==1)
		{
		snprintf(buff,sizeof(buff)-1,"Unable to connect to MYSQL\n");
		ErrorLog(cnf,buff);
		}
	return(-1);
	}

GetPeerIp(s,ip,buff);

//
// We check if this IP is authorized to connect to us
//

if(CheckIP(cnf,ip)!=1)
	{
	if(cnf->debug==1) printf("%d: Rejected New incoming connexion from %s (%s)\n",pid,buff,ip);
	if(cnf->syslog==1)
		{
		snprintf(buff,sizeof(buff)-1,"Rejected New incoming connexion from %s (%s)\n",buff,ip);
		ErrorLog(cnf,buff);
		}

	SQLClose();
	return(0);
	}

//
// Ok, The IP is accepted
//

if(cnf->debug==1) printf("%d: New incoming connexion from %s (%s)\n",pid,buff,ip);

ts=time(0);

request[0]=sender[0]=recipient[0]=ip[0]=0;

CloseTcpServer(srv);

while(1==1)
	{
	//
	// This functions does not read more than BLEN-1 bytes
	// from the network and thus no buffer overflow is possible
	//
	if(ReadLSocket(s,buff,BLEN-1,TOUT)<0)
		{
		if(cnf->syslog==1) ErrorLog(cnf,"Read Network error");
		if(cnf->debug==1) printf("%d: Read Network error\n",pid);
		SQLClose();
		return(-1);
		}

	//
	// To be sure that our buffer is null terminated to avoid
	// a buffer overflow, we manually set a null to the end of the buffer.
	//
	buff[BLEN-1]=0;

	//
	// Now, we are sure our buffer string length is no more than BLEN
	// as all parameters are defined also as buffers with a BLEN size
	// no buffer overflow is possible using strcpy .
	//

	if(strcmp(buff,"")==0) break;

	if(strncmp(buff,"request=",8)==0)
		strcpy(request,buff+8);

	if(strncmp(buff,"sender=",7)==0)
		strcpy(sender,buff+7);

	if(strncmp(buff,"recipient=",10)==0)
		strcpy(recipient,buff+10);

	if(strncmp(buff,"client_address=",15)==0)
		strcpy(ip,buff+15);

	}

//
// To be sure that our parameters are null terminated to avoid
// a buffer overflow, we manually set a null to the end of the parameters.
//

ip[BLEN-1]=0;
recipient[BLEN-1]=0;
sender[BLEN-1]=0;

//
// Then we remove nasty chars to avoid a possible SQL injection
//

Quote(ip);
Quote(recipient);
Quote(sender);

//
// Now, we can safely use, str** functions
//

if(sender[0]==0) strcpy(sender,"void@void");

if(strcmp(request,REQ)!=0 || recipient[0]==0 || ip[0]==0)
	{
	snprintf(buff,sizeof(buff)-1,"Received invalid data req=(%s) sender=(%s) recipient=(%s) ip=(%s)",request,sender,recipient,ip);
	if(cnf->syslog==1) ErrorLog(cnf,buff);
	if(cnf->debug==1) printf("%d: %s\n",pid,buff);
	SQLClose();
	return(-2);
	}

if(cnf->debug==1)
	printf("%d: Got the following valid data req=(%s) sender=(%s) recipient=(%s) ip=(%s)\n",pid,request,sender,recipient,ip);

n=GreyList(ip,sender,recipient,cnf);
if(cnf->debug==1) printf("%d: End of the greylist algo\n",pid);


if(n<0)
	{
	if(cnf->syslog==1) ErrorLog(cnf,"MySQL error");
	if(cnf->debug==1) printf("%d: MySQL error\n",pid);
	SQLClose();
	return(-3);
	}

if(n==0)
	{
        if(cnf->syslog==1) Log(cnf,recipient,sender,ip,MSGGREYLIST);
	if(cnf->debug==1) printf("%d: Decision is to greylist\n",pid);

	if(cnf->training==0)
		{
		if(atoi(cnf->message)<400 || atoi(cnf->message)>499)
		snprintf(buff,sizeof(buff)-1,"action=defer_if_permit %s\n\n",cnf->message);
		else snprintf(buff,sizeof(buff)-1,"action=%s\n\n",cnf->message);
		}
	else
		{
		if(cnf->debug==1) printf("%d: Training mode, sending dunno\n",pid);
		strcpy(buff,"action=dunno\n\n");
		}

        WriteSocket(s,buff,strlen(buff),TOUT);
        }
else
	{
	WriteSocket(s,"action=dunno\n\n",14,TOUT);
	if(cnf->debug==1) printf("%d: Decision is to not greylist\n",pid);
	}

SQLClose();
return(0);
}

void TheEnd(int s)
{
	int status;

	while(wait(&status) > 0);
	shutdown(srv.sd,2);
	CloseTcpServer(srv);
	exit(0);
}

void Reload(int s)
{
ReadConfig(CONF,&conf);
}

int MyDaemon(int nochdir, int noclose)
{
int fd;

switch (fork()) {
        case -1:
                return(-1);
        case 0:
                break;
        default:
                _exit(0);
}

if(setsid() == -1) return(-1);
if(!nochdir) (void)chdir("/");

if(!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1)
        {
        (void)dup2(fd, STDIN_FILENO);
        (void)dup2(fd, STDOUT_FILENO);
        (void)dup2(fd, STDERR_FILENO);
        if(fd>2) (void)close(fd);
        }
return(0);
}
