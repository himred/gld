#include "gld.h"
#include "sockets.h"

int GreyList(char *ip,char *sender,char *recipient,config *conf)
{

char query[QLEN];
long n,x;
int ts;
char *domain;
char netw[32];
int i,l;
char oip[32];
int a,b,c,d;
int pid;
char osender[BLEN];
char orecipient[BLEN];

pid=getpid();

ts=time(0);
strncpy(oip,ip,sizeof(oip)-1);
strncpy(osender,sender,sizeof(osender)-1);
strncpy(orecipient,recipient,sizeof(orecipient)-1);

if(conf->debug==1) printf("%d: Starting the greylist algo\n",pid);

//
// If we do lightgreylisting, then we just keep the network part of ip
//
if(conf->light==1)
{
	if(conf->debug==1) printf("%d: lightgrey is on, let's remove the last octet of ip\n",pid);
	l=strlen(ip);
	for(i=l-1;i>=0;i--)
	{
		if(ip[i]=='.')
		{
			ip[i+1]='0';
			ip[i+2]=0;
			break;
		}
	}
}

//
// Do we have this entry in our database?
//
snprintf(query,sizeof(query)-1,"select first from greylist where ip='%s' and sender='%s' and recipient='%s'",ip,sender,recipient);
n=SQLQuery(query);
if(conf->debug==1) printf("%d: Query=(%s) result=%ld\n",pid,query,n);

//
// If request failed, return the error
//
if(n<0)
{
	return(-1);
}

//
// If the triplet is in our db
//
if(n>0)
{
	// and mintime+, always update last timestamp (cleanup needs this) and accept it
	if(ts-n>conf->mini)
	{
		snprintf(query,sizeof(query)-1,"update greylist set last=%d,n=n+1 where ip='%s' and sender='%s' and recipient='%s'",ts,ip,sender,recipient);
		SQLQuery(query);
		if(conf->debug==1) printf("%d: Query=(%s)\n",pid,query);
		return(1);
	}
	// any other case (mintime-), refuse it
	else
	{
		if(conf->debug==1) printf("%d: MINTIME has not been reached yet\n",pid);
		return(0);
	}
}

// #########################################################
// From this point to the end, the triplet WAS NOT in the db
// #########################################################

//
// Now we do some whitelist checks before inserting it
//

//
// First we check our local whitelist
//
if(conf->whitelist==1)
{
	if(conf->debug==1) printf("%d: whitelist is on\n",pid);
	domain=(char *)strstr(osender,"@");
	if(domain==NULL) domain=osender;

	strncpy(netw,oip,sizeof(netw)-1);
	l=strlen(netw);
	for(i=l-1;i>=0;i--)
	{
		if(netw[i]=='.')
		{
			netw[i]=0;
			break;
		}
	}

	snprintf(query,sizeof(query)-1,"select count(mail) from whitelist where mail in ('%s','%s','%s','%s')",osender,domain,oip,netw);
	n=SQLQuery(query);
	if(conf->debug==1) printf("%d: Query=(%s) result=%ld\n",pid,query,n);
	if(n>0)
	{
		if(conf->syslog==1) Log(conf,orecipient,osender,oip,MSGLOCALWL);
		return(1);
	}
}

//
// then we check the DNS whitelist
//
if(conf->dnswl[0]!=0)
{
	if(conf->debug==1) printf("%d: DNS whitelist is on\n",pid);
	x=sscanf(oip,"%d.%d.%d.%d",&a,&b,&c,&d);
	if(x==4)
	{
		snprintf(query,sizeof(query)-1,"%d.%d.%d.%d.%s",d,c,b,a,conf->dnswl);
		n=DnsIp(query,NULL);
		if(conf->debug==1) printf("%d: DNSQuery=(%s) result=%ld\n",pid,query,n);
		if(n==0)
		{
			if(conf->syslog==1) Log(conf,orecipient,osender,oip,MSGDNSWL);
			return(1);
		}
	}
}

//
// If we are here, The mail was not in our database
// was not whitelisted and thus we have to insert it
//
snprintf(query,sizeof(query)-1,"insert into greylist values('%s','%s','%s',%d,%d,1)",ip,sender,recipient,ts,ts);
SQLQuery(query);
if(conf->debug==1) printf("%d: Query=(%s)\n",pid,query);

//
// If we have activated the mxgrey
// Let's accept the mail if this ip already succeded the required number of greylists
//
if(conf->mxgrey>0)
{
	// check for unique triplets already graylisted from the IP
	snprintf(query,sizeof(query)-1,"select count(first) from greylist where ip='%s' and n>1",ip);
	n=SQLQuery(query);
	if(conf->debug==1) printf("%d: Mxgrey Query=(%s) result=%ld (minimum needed is %d)\n",pid,query,n,conf->mxgrey);
	// if found, accept it
	if(n>=conf->mxgrey)
	{
		return(1);
	}
}

return(0);

}

