#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#define MAX_LENGTH 512


int main(int argc, char *argv[])
{
    DIR *dirp;
    struct dirent *direntp;
    struct stat stat_buf;
    char name[MAX_LENGTH];

    // number of arguments must be 2
    if (argc != 2)
    {
        fprintf( stderr, "Usage: %s dir_name\n", argv[0]);
        exit(1);
    }
    if ((dirp = opendir( argv[1])) == NULL)
    {
        perror(argv[1]);
        exit(2);
    }

    // header of the file
    FILE * file_ptr;
    file_ptr = fopen ("files.txt", "a");
    fprintf(file_ptr, "inode\ttitle\t\t\taccess\t size\tlast modification\n");

    while ((direntp = readdir( dirp)) != NULL)
    {
        sprintf(name,"%s/%s",argv[1],direntp->d_name);
        // alternativa a chdir()
        if (lstat(name, &stat_buf)==-1)
        {
            perror("lstat ERROR"); //if permission denied on that directory
            exit(3);
        }
        if (S_ISREG(stat_buf.st_mode)) {
            fprintf(file_ptr,"%-7ld %-23s ", (long) stat_buf.st_ino, direntp->d_name);
            fprintf(file_ptr, (stat_buf.st_mode & S_IRUSR) ? "r" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IWUSR) ? "w" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IXUSR) ? "x" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IRGRP) ? "r" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IWGRP) ? "w" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IXGRP) ? "x" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IROTH) ? "r" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IWOTH) ? "w" : "-");
            fprintf(file_ptr, (stat_buf.st_mode & S_IXOTH) ? "x" : "-");
            fprintf(file_ptr," %-6lld",(long long) stat_buf.st_size);
            fprintf(file_ptr," %-5s", ctime(&stat_buf.st_mtime));
        }
    }
    fclose(file_ptr);
    closedir(dirp);
    exit(0);
}