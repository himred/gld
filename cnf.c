#include "gld.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#define FACSIZE 10

typedef struct facs {
    char *name;
    int fac;
}facs;

facs facys[] =
  {
    { "daemon", LOG_DAEMON },
    { "mail", LOG_MAIL },
    { "local0", LOG_LOCAL0 },
    { "local1", LOG_LOCAL1 },
    { "local2", LOG_LOCAL2 },
    { "local3", LOG_LOCAL3 },
    { "local4", LOG_LOCAL4 },
    { "local5", LOG_LOCAL5 },
    { "local6", LOG_LOCAL6 },
    { "local7", LOG_LOCAL7 }
  };
#endif

int ReadConfig(char *file,config *conf)
{
char *p;
char buffer[1024];
FILE *fic;
int i;

fic=fopen(file,"r");
if(fic==(FILE *)NULL) return(-1);

// We set the default values

strcpy(conf->sqlhost,"localhost");
strcpy(conf->sqluser,"myuser");
strcpy(conf->sqldb,"mydb");
strcpy(conf->sqlpasswd,"mypasswd");
strcpy(conf->message,"Greylisted");
conf->training=0;
conf->port=2525;
conf->maxcon=100;
conf->mini=60;
conf->syslog=1;
conf->accept=1;
conf->whitelist=1;
conf->light=0;
conf->dnswl[0]=0;
conf->loopback=1;
conf->debug=0;
conf->user[0]=0;
conf->grp[0]=0;
conf->nbnet=0;
conf->mxgrey=0;

#ifdef HAVE_SYSLOG_H
conf->facility=LOG_MAIL;
#endif

// Now we read

while(fgets(buffer,1024,fic)!=NULL)
        {
        p=(char *)strstr(buffer,"=");
        if(p!=NULL)
                {
		buffer[strlen(buffer)-1]=0;
                *p=0;
                if(strcmp(buffer,"CLIENTS")==0) ReadClients(conf,p+1);
                if(strcmp(buffer,"USER")==0) strcpy(conf->user,p+1);
                if(strcmp(buffer,"GROUP")==0) strcpy(conf->grp,p+1);
                if(strcmp(buffer,"DNSWL")==0) strcpy(conf->dnswl,p+1);
                if(strcmp(buffer,"SQLHOST")==0) strcpy(conf->sqlhost,p+1);
                if(strcmp(buffer,"SQLUSER")==0) strcpy(conf->sqluser,p+1);
                if(strcmp(buffer,"SQLDB")==0) strcpy(conf->sqldb,p+1);
                if(strcmp(buffer,"SQLPASSWD")==0) strcpy(conf->sqlpasswd,p+1);
                if(strcmp(buffer,"MESSAGE")==0) strcpy(conf->message,p+1);
                if(strcmp(buffer,"PORT")==0) conf->port=atoi(p+1);
                if(strcmp(buffer,"MAXCON")==0) conf->maxcon=atoi(p+1);
                if(strcmp(buffer,"TRAINING")==0) conf->training=atoi(p+1);
                if(strcmp(buffer,"MINTIME")==0) conf->mini=atol(p+1);
                if(strcmp(buffer,"MXGREY")==0) conf->mxgrey=atoi(p+1);
                if(strcmp(buffer,"SYSLOG")==0) conf->syslog=atoi(p+1);
                if(strcmp(buffer,"ERRACCEPT")==0) conf->accept=atoi(p+1);
                if(strcmp(buffer,"WHITELIST")==0) conf->whitelist=atoi(p+1);
                if(strcmp(buffer,"LIGHTGREY")==0) conf->light=atoi(p+1);
                if(strcmp(buffer,"LOOPBACKONLY")==0) conf->loopback=atoi(p+1);
		#ifdef HAVE_SYSLOG_H
                if(strcmp(buffer,"FACILITY")==0)
			for(i=0;i<FACSIZE;i++)
				if(strcmp(p+1,facys[i].name)==0)
					conf->facility=facys[i].fac;
		#endif
                }
        }

fclose(fic);

return(0);
}

void Log(config *conf,char *recipient,char *sender,char *ip,int white)
{
#ifdef HAVE_SYSLOG_H
openlog("gld",0,conf->facility);
if(white==MSGGREYLIST) syslog(LOG_NOTICE,"Greylist activated for recipient=<%s> sender=<%s> ip=<%s>",recipient,sender,ip);
if(white==MSGLOCALWL) syslog(LOG_NOTICE,"Local whitelist hit for recipient=<%s> sender=<%s> ip=<%s>",recipient,sender,ip);
if(white==MSGDNSWL) syslog(LOG_NOTICE,"DNS whitelist hit for recipient=<%s> sender=<%s> ip=<%s>",recipient,sender,ip);
closelog();
#endif
}

void ErrorLog(config *conf,char *msg)
{
#ifdef HAVE_SYSLOG_H
openlog("gld",0,conf->facility);
syslog(LOG_ALERT,"%s",msg);
closelog();
#endif
}

int ReadClients(config *conf,char *str)
{
char *ptr,*x,*y;

ptr=str;
conf->nbnet=0;

while(*ptr!=0)
        {
        x=strstr(ptr," ");
        if(x!=NULL) *x=0;
        y=strstr(ptr,"/");
        if(y!=NULL)
                {
                *y=0;
                conf->nets[conf->nbnet].netw=Hash(ptr);
                conf->nets[conf->nbnet].mask=CidrMsk(atoi(y+1));
                conf->nbnet++;
                if(conf->nbnet==NLEN || x==NULL) break;
                }
        ptr=x+1;
        }
return(0);
}

unsigned long Hash(char *ip)
{
struct in_addr x;
int r;

r=inet_aton(ip,&x);
if(r==0) return(0);

return(x.s_addr);
}

int CheckIP(config *conf,char *ip)
{
unsigned long h,netw,mask;
int i;

if(conf->nbnet==0) return(1);

h=Hash(ip);

for(i=0;i<conf->nbnet;i++)
        {
        netw=conf->nets[i].netw;
        mask=conf->nets[i].mask;
        if((h&mask)==(netw&mask)) return(1);
        }

return(0);
}

unsigned long CidrMsk(int msk)
{
int i;
unsigned long x=0;

for(i=0;i<msk;i++)
        {
        x=x>>1;
        x+=0x80000000;
        }

x=htonl(x);
return(x);
}
