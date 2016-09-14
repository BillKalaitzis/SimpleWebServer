/* webserver.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cnaiapi.h>


#if defined(linux) || defined(SOLARIS) /* __ Note: Changed parameter from "LINUX" to "linux" in order to comply with gcc's standards __*/
#include <sys/time.h>
#endif

#define BUFFSIZE	256
#define SERVER_NAME	"CNAI Demo Web Server"

#define LEN_ERROR_400 106
#define LEN_ERROR_404 81    
#define LEN_HOME_PAGE 236
#define LEN_TIME_PAGE 73

int	recvln(connection, char *, int);
void	send_head(connection, int, int, int);
char    *concat(char *, char *); /*__ Concatenate two separate strings __*/
char    *getRootDirectory(); /* __ Get the default directory of the server's files __*/

/*-----------------------------------------------------------------------
 *
 * Program: webserver
 * Purpose: serve hard-coded webpages to web clients
 * Usage:   webserver <appnum>
 *
 *-----------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{

	connection	conn;
	int		n;
	char		buff[BUFFSIZE], cmd[16], path[64], vers[16];
	char		*timestr;
        
        char            rootdir[256];  /*The root directory of the server */
                        *rootdir = getRootDirectory();
                        
        char            *error400 = "error400";         /* __ The pages of the server __ */
        char            *error404 = "error404";
        char            *home = "index.html";
        char            *time = "time.asp";     
        char            *personalPage = "BillKalaitzis.jpg";
        
        char            *fp_error400 = concat(rootdir,error400); /* __ Full paths to the pages of the server __ */
        char            *fp_error404 = concat(rootdir,error404);
        char            *fp_home = concat(rootdir,home);
        char            *fp_time = concat(rootdir,time);
        char            *fp_personalPage = concat(rootdir,personalPage);
        
        char            error_400_content[LEN_ERROR_400];  /* __ The buffers of each page __*/
        char            error_404_content[LEN_ERROR_404];
        char            home_content[LEN_HOME_PAGE];
        char            time_content[LEN_TIME_PAGE];
        char            *personalPage_content;
        
        unsigned long    len; /* __ The length of the image __*/
        
        FILE            *fp;

#if defined(linux) || defined(SOLARIS) /* __ Note: Changed parameter from "LINUX" to "linux" in order to comply with gcc's standards __*/
	struct timeval	tv;

#elif defined(WIN32)
	time_t          tv;
#endif
        
	if (argc != 2) {
		(void) fprintf(stderr, "usage: %s <appnum>\n", argv[0]);
		exit(1);
	}
        
	while(1) {

		/* wait for contact from a client on specified appnum */

		conn = await_contact((appnum) atoi(argv[1]));
		if (conn < 0)
			exit(1);

		/* read and parse the request line */

		n = recvln(conn, buff, BUFFSIZE);
		sscanf(buff, "%s %s %s", cmd, path, vers);

		/* skip all headers - read until we get \r\n alone */

		while((n = recvln(conn, buff, BUFFSIZE)) > 0) {
			if (n == 2 && buff[0] == '\r' && buff[1] == '\n')
				break;
		}

		/* check for unexpected end of file */

		if (n < 1) {
			(void) send_eof(conn);
			continue;
		}
		
		/* check for a request that we cannot understand */

		if (strcmp(cmd, "GET") || (strcmp(vers, "HTTP/1.0") &&
					   strcmp(vers, "HTTP/1.1"))) {
                        fp = fopen(fp_error400,"r");
                        fscanf(fp,"%103c",error_400_content);
                        fclose(fp);
                                          
			send_head(conn, 400, strlen(error_400_content),1);
			(void) send(conn, error_400_content, strlen(error_400_content),0);
			(void) send_eof(conn);
			continue;
		}

		/* send the requested web page or a "not found" error */

		if (strcmp(path, "/") == 0) {
                        fp = fopen(fp_home,"r");
                        fscanf(fp,"%235c",home_content);
                        fclose(fp);
                        
			send_head(conn, 200, strlen(home_content),1);
			(void) send(conn, home_content, strlen(home_content),0);
		} else if (strcmp(path, "/time") == 0) {
#if defined(linux) || defined(SOLARIS) /* __ Note: Changed parameter from "LINUX" to "linux" in order to comply with gcc's standards __*/
			gettimeofday(&tv, NULL);
			timestr = ctime(&tv.tv_sec);
#elif defined(WIN32)
			time(&tv);
			timestr = ctime(&tv);
#endif
                        fp = fopen(fp_time,"r");
                        fscanf(fp,"%71c",time_content);
                        fclose(fp);
                        
			(void) sprintf(buff, time_content, timestr);
			send_head(conn, 200, strlen(buff),1);
			(void) send(conn, buff, strlen(buff), 0);
                        
		} else if(strcmp(path,"/BillKalaitzis") == 0){ /* __ My own page __ */
                        fp = fopen(fp_personalPage,"rb");
                        
                        fseek(fp, 0, SEEK_END);
                        len=ftell(fp);
                        fseek(fp, 0, SEEK_SET);
                        personalPage_content=(char *)malloc(len);
                        fread(personalPage_content, len, 1, fp);
                        fclose(fp);
                        
                        send_head(conn, 200, len,2);
			(void) send(conn, personalPage_content, len,0);
                        
                }else { /* not found */
                    
                        fp = fopen(fp_error404,"r");
                        fscanf(fp,"%79c",error_404_content);
                        fclose(fp);
                        
			send_head(conn, 404, strlen(error_404_content),1);
			(void) send(conn, error_404_content, strlen(error_404_content),0);
		}
		(void) send_eof(conn);
	}
}

/*-----------------------------------------------------------------------
 * send_head - send an HTTP 1.0 header with given status and content-len
 *-----------------------------------------------------------------------
 */
void
send_head(connection conn, int stat, int len,int contentType)
{
	char	*statstr, buff[BUFFSIZE];

	/* convert the status code to a string */

	switch(stat) {
	case 200:
		statstr = "OK";
		break;
	case 400:
		statstr = "Bad Request";
		break;
	case 404:
		statstr = "Not Found";
		break;
	default:
		statstr = "Unknown";
		break;
	}
	
	/*
	 * send an HTTP/1.0 response with Server, Content-Length,
	 * and Content-Type headers.
	 */

	(void) sprintf(buff, "HTTP/1.0 %d %s\r\n", stat, statstr);
	(void) send(conn, buff, strlen(buff), 0);

	(void) sprintf(buff, "Server: %s\r\n", SERVER_NAME);
	(void) send(conn, buff, strlen(buff), 0);

	(void) sprintf(buff, "Content-Length: %d\r\n", len);
	(void) send(conn, buff, strlen(buff), 0);
        
        if(contentType == 1){ /* __ Client has requested an html page __ */
            (void) sprintf(buff, "Content-Type: text/html\r\n");
            (void) send(conn, buff, strlen(buff), 0);
        }
        else if(contentType == 2){ /* __ Client has requested an image __ */
            (void) sprintf(buff, "Content-Type: image/jpg\r\n");
            (void) send(conn, buff, strlen(buff), 0);
        }

	(void) sprintf(buff, "\r\n");
	(void) send(conn, buff, strlen(buff), 0);
}

char *concat(char *c1, char *c2){
	char *str = malloc(strlen(c1) + strlen(c2) + 1);
        
	strcpy(str, c1);
	strcat(str, c2);
	return str;
}

char *getRootDirectory(){
    
    char returnChar[256];
    FILE *fp = fopen("./webserver.conf","r");
    
    if(fp == NULL){
        printf("No webserver.conf found.\nRead to the manual for more details.\n");
        exit(1);
    }
    else{
        fscanf(fp,"%s",returnChar);
        printf("%s",returnChar);
        return returnChar;
    }
    return NULL;
    
}