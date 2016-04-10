#include <stdio.h>
#include <stdlib.h>
int main()
{
   FILE * file_ptr;
   file_ptr = fopen ("files.txt", "a");
   fprintf(file_ptr, "%s %s %s %d", "We", "are", "in", 2014);
   fclose(file_ptr);
   return(0);
}
