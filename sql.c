#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _DB_MYSQL_
#include <mysql/mysql.h>
#endif
#ifdef _DB_PGSQL_
#include "libpq-fe.h"
#endif
#include "gld.h"

#ifdef _DB_MYSQL_
static int MySQLConnect(char *host,char *user,char *passwd,char *db);
static void MySQLClose(void);
static long MySQLQuery(char *q);
#endif
#ifdef _DB_PGSQL_
static int PgSQLConnect(char *host,char *user,char *passwd,char *db);
static void PgSQLClose(void);
static long PgSQLQuery(char *q);
#endif

int SQLConnect(char *host,char *user,char *passwd,char *db)
{
#ifdef _DB_MYSQL_
return(MySQLConnect(host, user, passwd, db));
#endif
#ifdef _DB_PGSQL_
return(PgSQLConnect(host, user, passwd, db));
#endif
}

void SQLClose(void)
{
#ifdef _DB_MYSQL_
return(MySQLClose());
#endif
#ifdef _DB_PGSQL_
return(PgSQLClose());
#endif
}

long SQLQuery(char *q)
{
#ifdef _DB_MYSQL_
return(MySQLQuery(q));
#endif
#ifdef _DB_PGSQL_
return(PgSQLQuery(q));
#endif
}

void ShowBaseInfo(void)
{
char query[QLEN];
int c;
long now;

now=time(0);

snprintf(query,sizeof(query)-1,"select count(*) from greylist");
c=SQLQuery(query);
printf("# of entries in the database         : %d\n",c);

if(c!=0)
	{
	snprintf(query,sizeof(query)-1,"select count(*) from greylist where n=1");
	c=SQLQuery(query);
	printf("# of one hit entries in the database : %d\n",c);

	snprintf(query,sizeof(query)-1,"select min(first) from greylist");
	c=SQLQuery(query);
	printf("Oldest entry in database             : %ld days ago\n",(now-c)/86400);
	}
}

void Quote(char *str)
{
int i,l;

l=strlen(str);
for(i=0;i<l;i++)
	if(str[i]=='"' || str[i]=='\'' || str[i]==';') str[i]=' ';

}

#ifdef _DB_MYSQL_

MYSQL *sql;

int MySQLConnect(char *host,char *user,char *passwd,char *db)
{

MYSQL *sqlh;

sqlh=mysql_init(NULL);
if(sqlh==(MYSQL *)NULL) return(-1);

sql=mysql_real_connect(sqlh,host,user,passwd,db,0,NULL,0);
if(sql==(MYSQL *)NULL) return(-2);

return(0);
}

void MySQLClose(void)
{
if(sql!=(MYSQL *)NULL) mysql_close(sql);
}

long MySQLQuery(char *q)
{
int r;
MYSQL_RES *result;
MYSQL_ROW row;
int n;
long ret;

if(sql!=(MYSQL *)NULL) r=mysql_query(sql,q); else r=-1;
if(r!=0) return(-1);

result=mysql_store_result(sql);
if(result==(MYSQL_RES *)NULL) return(0);

n=mysql_num_rows(result);
if(n==0) return(0);

row=mysql_fetch_row(result);
ret=atol(row[0]);
mysql_free_result(result) ;

return(ret);
}

#endif
#ifdef _DB_PGSQL_

PGconn *sql;

int PgSQLConnect(char *host,char *user,char *passwd,char *db) {
	char conninfo[1024];

	snprintf(conninfo,sizeof(conninfo)-1,"host = %s user = %s password = %s dbname = %s", host, user, passwd, db);
	sql = PQconnectdb(conninfo);

	if (PQstatus(sql) == CONNECTION_BAD) {
		fprintf(stderr, "Connection to database \"%s\" failed.\n", db);
		fprintf(stderr, "%s", PQerrorMessage(sql));
		PQfinish(sql);
		return(-2);
	}

	return(0);
}

void PgSQLClose(void) {
	PQfinish(sql);
	return;
}

long PgSQLQuery(char *q) {
	PGresult *result;
	long ip;

	if (sql != NULL) {
		result = PQexec(sql, q);
	} else {
		return(-1);
	}

	if (PQntuples(result) == 0) {
		return(0);
	}

	ip = atol(PQgetvalue(result, 0, 0));
	PQclear(result);
	return(ip);
}

#endif
