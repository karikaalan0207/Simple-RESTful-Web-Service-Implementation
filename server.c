
This is a very simple HTTP server. Default port is 10000 and ROOT for the server is your current working directory..

You can provide command line arguments like:- $./a.aout -p [port] -r [path]

for ex. 
$./a.out -p 50000 -r /home/
to start a server at port 50000 with root directory as "/home"

$./a.out -r /home/shadyabhi
starts the server at port 10000 with ROOT as /home/shadyabhi

*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include <mysql.h>


#define CONNMAX 1000
#define BYTES 1024

char *ROOT;
int listenfd, clients[CONNMAX];
void display_course();
void error(char *);
void startServer(char *);
void respond(int);

int main(int argc, char* argv[])
{
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;    
	
	//Default Values PATH = ~/ and PORT=10000
	char PORT[6];
	ROOT = getenv("PWD");
	strcpy(PORT,"10000");

	int slot=0;

	//Parsing the command line arguments
	while ((c = getopt (argc, argv, "p:r:")) != -1)
		switch (c)
		{
			case 'r':
				ROOT = malloc(strlen(optarg));
				strcpy(ROOT,optarg);
				break;
			case 'p':
				strcpy(PORT,optarg);
				break;
			case '?':
				fprintf(stderr,"Wrong arguments given!!!\n");
				exit(1);
			default:
				exit(1);
		}
	
	printf("Server started at port no. %s%s%s with root directory as %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");
	// Setting all elements to -1: signifies there is no client connected
	int i;
	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	startServer(PORT);

	// ACCEPT connections
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
			error ("accept() error");
		else
		{
			if ( fork()==0 )
			{
				respond(slot);
				exit(0);
			}
		}

		while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
	}

	return 0;
}
//display_course
void display_course()
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

	char *server = "localhost";
	char *user = "master";
	char *password = "password"; /* set me first */
	char *database = "assignment";

	conn = mysql_init(NULL);

	/* Connect to database */
	if (!mysql_real_connect(conn, server, user, password,database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	/* send SQL query */
	if (mysql_query(conn, "select * from students")) {
	fprintf(stderr, "%s\n", mysql_error(conn));
	exit(1);
	}
 
	res = mysql_use_result(conn);

	// making a file and writing the schema
	FILE *filepoi;

	filepoi = fopen("studentsdata/studentsdata.txt","w");
	fprintf(filepoi,"Students of CST Department\n\n");
	fprintf(filepoi,"|Rollno    |Year      |Name      |\n");
	fprintf(filepoi,"----------------------------------\n");

	/* output table name */
	printf("Students of CST Department\n\n");
	printf("----------------------------------\n");
	printf("|Rollno    |Year      |Name      |\n");
	printf("----------------------------------\n");
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		printf("|%s        |%s        |%s        \n", row[0],row[2],row[1]);
		/* saving data to text */
		fprintf(filepoi,"|%s        |%s        |%s        \n",row[0],row[2],row[1]);
	}
	 
	close(filepoi);
	 /* close connection */
	mysql_free_result(res);
	mysql_close(conn);

}

//start server
void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if ( listen (listenfd, 1000000) != 0 )
	{
		perror("listen() error");
		exit(1);
	}
}

//client connection
void respond(int n)
{
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999] , *ss[1];
	int rcvd, fd, bytes_read;

	memset( (void*)mesg, (int)'\0', 99999 );

	rcvd=recv(clients[n], mesg, 99999, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		printf("%s", mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			}
			else
			{

				/*

				This part is commented out since it corresponds to find out the file
				requested in the URL and catering it. In your case the URL corresponds 
				to some 'resource' that your web service should cater.
				For example, '/courses' may report all the courses offered by the department which
				your web service gets from a database.
				At this place, therefore, you should write the code to parse the URL and take 
				appropriate action.

				In case you decide to use database for storing and catering information, use mysql@10.2.1.40
				and use mysql C api here for accessing the database.
				*/
				if ( strncmp(reqline[1], "/\0", 2)==0 )
				{
					reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHe..
					strcpy(path, ROOT);
					strcpy(&path[strlen(ROOT)], reqline[1]);
					printf("file: %s\n", path);

					if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
					{
						send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
						while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
						write (clients[n], data_to_send, bytes_read);
					}
					else    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
				
				}
				else{
					char  *new=reqline[1];
					printf("%s\n",new);
				 	if (strcmp(new,"/studentsdata")==0)
					{
						display_course();
						reqline[1] = "/studentsconf.html";        
						strcpy(path, ROOT);
						strcpy(&path[strlen(ROOT)], reqline[1]);
						printf("file: %s\n", path);

						if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
						{
							send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
							// printf("%d",read(fd, data_to_send, BYTES));
							while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
							{
								write (clients[n], data_to_send, bytes_read);
							}
						}
						return;
					}
				}

			}
		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
}
