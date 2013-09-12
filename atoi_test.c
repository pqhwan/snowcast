#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
	int
main ( int argc, char *argv[] )
{
	char buf[512];
	ssize_t bytes;
	size_t max = sizeof buf;
	int not_int;
	int i;
	while(1){
		printf("type in a number\n");
		bytes = read(0, buf, max);
		buf[bytes] = '\0';
		not_int = 0;
		for (i=0 ;i<bytes-1;i=i+1){
			if(!isdigit(buf[i])){
				printf("something not int: %c\n", buf[i]);
				not_int = 1;
			}
		}
		if(!not_int){
			printf("%d \n", atoi(buf));
		}
	}
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
