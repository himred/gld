# What is gld ?

gld stands for GreyList Daemon.
gld is a standalone policy delegation server for postfix that implements
the greylist algorithm as defined at http://www.greylisting.org
gld is written in C and use mysql as database.

# What are the requirements ?

gld needs the following software:
- a postfix 2.1 or higher server
- a mysql server or a pgsql server

# How to build gld ?

This should be easy, just run `./configure` and then make and wait ...
Of course you need to have include files and libraries for MySQL support.

If the compilation fails with mysql related errors, try to run `./configure --with-mysql=DIR` .


# How to install gld ?

Just run `make install`

The install copy only 2 files into your system.
The files are installed as follow:

| file path | file role |
|--|--|
| /etc/gld.conf.sample | the sample config file |
| /usr/local/bin/gld | the gld executable file |



Afterward, edit /etc/gld.conf.sample file and modify it to suit your needs.
Then rename it to gld.conf .

Please, **PLEASE** read the **README-SECURITY** file before choosing your options.

Then, create the tables whitelist and greylist on your mysql server
I have provided the script tables.sql that helps you to do this task.

Then start gld and see if it run or if an error message is displayed

If you are here, you are done, now just configure postfix to use your brand new greylist server .

To configure postfix just add the following line:

    check_policy_service inet:127.0.0.1:2525

to the end of your smtpd_recipient_restrictions statement of your main.cf file.
You don't have to edit or modify your master.cf file .


# How to start gld ?

just run gld without any parameters.


# How to stop gld ?

Just kill the process with the TERM signal
(SIGTERM is the default signal sent by the kill command)


# How to reload the configuration of the server ?

Just send the HUP signal to the server
ie: `kill -HUP <pid> or killall -HUP gld`

# How to know the version of gld ?

Just try: `gld -v`
	

# What happens if the mysql server goes down ?

Depending on the configuration of gld, the server can refuse to answer
or send a 'dunno' response, which will likely accept the mails.
Please read gld.conf for more informations.


# What happens if gld goes down ?

Well, gld *MUST* be running while postfix is running.
make sure you start gld before you start postfix .

If gld should die (this has never occured here)
postfix will not be able to connect to the policy server
and will return a '450 server configuration' .
Thus, you won't be able to receive any greylisted mail .....


# What kind of replies this server send to postfix ?

gld sends only 2 replies to postfix.

if the email is to be refused, then gld reply: action=defer_if_permit MESSAGE
otherwise gld reply: action=dunno


# I want to greylist only some emails and only some domains, How to do ?

postfix has a cool feature for that named policy maps.
Let say you want to greylist only the email grey@foo.bar and the whole domain bar.com
here follow hos to set up postfix for that.

First, define a policy map named greylist_policy in main.cf
to do this just add the following lines in main.cf :

    smtpd_restriction_classes = greylist_policy
    greylist_policy = check_policy_service inet:127.0.0.1:2525

Then in the end of  smtpd_recipient_restrictions
add the following line: `check_recipient_access hash:/etc/postfix/A_FILE_OF_YOUR_CHOICE`
instead of the standard line: `check_policy_service inet:127.0.0.1:2525`

Finally create a text file named /etc/postfix/A_FILE_OF_YOUR_CHOICE
which contains domain and emails to be greylisted, the format is:
		

    email greylist_policy
    domain greylist_policy

thus in our example, the file would contain:
		
    grey@foo.bar greylist_policy
    bar.com greylist_policy

Finally make this text file a hash database with the command: 

    postmap /etc/postfix/A_FILE_OF_YOUR_CHOICE
and you are done ! .

# Where do I report bugs,suggestions,insults ?

Just send a mail to salim@gasmi.net
(of course this email is greylisted ..)
But please, include the version of gld you use and the OS you are running.


# Note from the author:

I want to thanks all people who mailed me with suggestions, patches and especially the following guys:
- Lefteris Tsintjelis for his help and support for testing 1.6.
- Dietmar Braun for his idea of the training mode.
- Wayne Smith for his support and suggestions.
- Santiago Vila for maintaining the Debian package.
- Blaz Zupan for maintaining the FreeBSD port.
- Volker Tanger for the nice howto.
- Brian Truelsen for his help and suggestions on MXGREY.
- Wietse Venema for postfix.

