/* $begin Super_servermain */
/*
 * Super_server.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content. Speed imporved from 
 *     baseline by using dynamic linking for cgi protocol and pre-threading
 *     server design. 
 * This server is developed on top of the tiny server in CS:APP Book
 *
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "csapp.h"
#include "sbuf.h"
#define NTHREADS  50
#define SBUFSIZE  200

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of connected descriptors */
void* handle;
void (*myadder)(int, char *);
char* error;

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    // char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; 

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* Dynamically load the shared library containing addvec() */
    handle = dlopen("./cgi-bin/mylib.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    /* Get a pointer to the addvec() function we just loaded */
    myadder = dlsym(handle, "myadder");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }    


    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE); 
    for (int i = 0; i < NTHREADS; i++)  /* Create worker threads */ 
	    Pthread_create(&tid, NULL, thread, NULL);             

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        // Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
        //             port, MAXLINE, 0);
        // printf("Accepted connection from (%s, %s)\n", hostname, port);
        sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */                                        
    }


    /* Unload the shared library */
    if (dlclose(handle) < 0) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    return 0;
}
/* $end Super_servermain */


/* Thread routine */
void *thread(void *vargp) 
{  
    Pthread_detach(pthread_self()); 
    while (1) { 
        int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */ 
        doit(connfd);                /* Service client */
        Close(connfd);
    }
}


/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {                     
        clienterror(fd, method, "501", "Not Implemented",
                    "Super_server does not implement this method");
        return;
    }                                                    
    read_requesthdrs(&rio);                              

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);       
    if (stat(filename, &sbuf) < 0) {                     
        clienterror(fd, filename, "404", "Not found",
                "Super_server couldn't find this file");
        return;
    }                                                    

    if (is_static) { /* Serve static content */          
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 
            clienterror(fd, filename, "403", "Forbidden",
                "Super_server couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);        
    }
    else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
            clienterror(fd, filename, "403", "Forbidden",
                "Super_server couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);            
    }
}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {          
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */
	strcpy(cgiargs, "");                           
	strcpy(filename, ".");                           
	strcat(filename, uri);                           
	if (uri[strlen(uri)-1] == '/')                   
	    strcat(filename, "home.html");               
	return 1;
    }
    else {  /* Dynamic content */          
        ptr = index(uri, '?');                
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else 
            strcpy(cgiargs, "");                    
        strcpy(filename, ".");                         
        strcat(filename, uri);                       
        return 0;
    }
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[1000], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);    
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Super_server Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n", filesize);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));   

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0); 
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); 
    Close(srcfd);                       
    Rio_writen(fd, srcp, filesize);     
    Munmap(srcp, filesize);             
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}  
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE];

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Super_server Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    /* Run cgi program here */
    myadder(fd, cgiargs);
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Super_server Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Super_server Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */