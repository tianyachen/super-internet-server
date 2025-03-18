/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    char buf1[50], buf2[200], buf3[300];
    int n1=0, n2=0;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p+1);
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    /* Make the response body */
    sprintf(buf1, "Welcome to add.com: ");
    sprintf(buf2, "%sTHE Internet addition portal.\r\n<p>", buf1);
    sprintf(buf3, "%sThe answer is: %d + %d = %d\r\n<p>", 
	    buf2, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", buf3);
  
    /* Generate the HTTP response */
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);
}
/* $end adder */