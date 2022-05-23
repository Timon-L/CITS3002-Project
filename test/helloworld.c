#include "helloworld.h"
int main()
{
   FILE * fp;
   fp = fopen("helloworld.txt","w");
   fprintf(fp,"%s", hello);
   fclose(fp);
}