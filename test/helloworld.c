#include "helloworld.h"

int main()
{
   printf("HELLO\n");
   FILE * fp;
   fp = fopen("helloworld.txt","w");
   fprintf(fp,"%s", hello);
   fclose(fp);
}