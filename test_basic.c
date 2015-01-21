#include <stdio.h>
#include <stdlib.h> //for exit

#include "dmm.h"

int main(int argc, char *argv[])
{
    
    /*
     1024 - 32 = 992
     malloc(10)
     
     992 - 32 - 8 - 16 = 936
     
     malloc(940)
     
     
     free(940)
     
     malloc(945)
     
     */
    
    
	char *array1, *array2, *array3;
	int i;

	printf("calling malloc(10)\n");	
	array1 = (char*)dmalloc(10);
    
	printf("array1 : %p ,   array2 : %p ,    array3 : %p \n", array1, array2, array3);
    
    if(array1 == NULL)
	{
		fprintf(stderr,"call to dmalloc() failed\n");
		fflush(stderr);
		exit(1);
	}

	for(i=0; i < 9; i++)
	{
		array1[i] = 'a';
	}
	array1[9] = '\0';

	printf("String: %s\n",array1);

	printf("calling malloc(940)\n");	
	array2 = (char*)dmalloc(940);
    
    printf("array1 : %p ,   array2 : %p ,    array3 : %p \n", array1, array2, array3);
    
	if(array2 == NULL)
	{
		fprintf(stderr,"call to dmalloc() failed\n");
		fflush(stderr);
		exit(1);
	}
	for(i=0; i < 99; i++)
	{
		array2[i] = 'b';
	}
	array2[99] = '\0';

	printf("String : %s, %s\n",array1, array2);

	printf("calling free(940)\n");	
	dfree(array2);
    
    printf("After free: array1 : %p ,   array2 : %p ,    array3 : %p \n", array1, array2, array3);
    
	printf("calling malloc(945)\n");	
	array3 = (char*)dmalloc(945);
    
    printf("array1 : %p ,   array2 : %p ,    array3 : %p \n", array1, array2, array3);

	if(array3 == NULL)
	{
		fprintf(stderr,"call to dmalloc() failed\n");
		fflush(stderr);
		exit(1);
	}
	for(i=0; i < 945; i++)
	{
		array3[i] = 'c';
	}
	array3[945] = '\0';

	printf("String: %s, %s, %s\n",array1, array2, array3);

	printf("calling free(945)\n");	
	dfree(array3);

	printf("Basic testcases passed!\n");

	return(0);
}
