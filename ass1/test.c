#include <stdio.h>
#include <stdlib.h>

int main()
{
	char buff[4096];
		FILE * fp;
		// if( fp = fopen("test.txt","r")==NULL){
		// 	printf("jdsj\n");
		// }
		fp = fopen("test.txt","r");
		fgets(buff, 255, (FILE*)fp);
   printf("2: %s\n", buff );
}