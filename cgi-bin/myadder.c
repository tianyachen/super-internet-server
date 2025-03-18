/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

void myadder(int fd, char* cgiargs) {
    char *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    char buf1[50], buf2[200], buf3[300], buf4[200], buf5[200];
    int n1=0, n2=0;

    /* Extract the two arguments */
    p = strchr(cgiargs, '&');
    *p = '\0';
    strcpy(arg1, cgiargs);
    strcpy(arg2, p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);


    /* Make the response body */
    sprintf(buf1, "Welcome to fast_add.com: ");
    sprintf(buf2, "%sTHE Internet addition portal.\r\n<p>", buf1);
    sprintf(buf3, "%sThe answer is: %d + %d = %d\r\n<p>", 
	    buf2, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", buf3);
  
    /* Generate the HTTP response */
    sprintf(buf4, "Content-length: %d\r\n", (int)strlen(content));
    Rio_writen(fd, buf4, strlen(buf4));
    sprintf(buf5, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf5, strlen(buf5));
    Rio_writen(fd, content, strlen(content));
    return;
}
/* $end adder */