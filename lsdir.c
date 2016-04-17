#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#define MAX_PATH_LENGTH 4096
#define MAX_NAME_LENGTH 512
#define DATE_LENGTH 24

void saveFileInfo(char *dir_name, char *file_name, struct stat *stat_buf, FILE *file_ptr) {
    fprintf(file_ptr, "%-8ld ", (long) (*stat_buf).st_ino);                                                             // inode
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IRUSR) ? "r" : "-");                                                     // permissions
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IWUSR) ? "w" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IXUSR) ? "x" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IRGRP) ? "r" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IWGRP) ? "w" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IXGRP) ? "x" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IROTH) ? "r" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IWOTH) ? "w" : "-");
    fprintf(file_ptr, ((*stat_buf).st_mode & S_IXOTH) ? "x" : "-");
    char *dtime = ctime(&(*stat_buf).st_mtime);                                                                         // modification date
    dtime[DATE_LENGTH] = 0;
    fprintf(file_ptr, " %s", dtime);
    fprintf(file_ptr, " %-10lld", (long long)((*stat_buf).st_mtime));
    fprintf(file_ptr, " %-9lld", (long long) (*stat_buf).st_size);
    char cwd[MAX_PATH_LENGTH];                                                                                          // path and name
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        fprintf(file_ptr, " %s/%s %s\n", cwd, dir_name, file_name);
    else
        perror("getcwd() error");
}

int main(int argc, char *argv[]) {
    DIR *dirp;
    struct dirent *direntp;
    struct stat stat_buf;
    char name[MAX_NAME_LENGTH];
    pid_t pid;
    int status;
    if (argc != 2) {                                                                                                    // number of arguments must be 2
        fprintf(stderr, "Usage: %s dir_name\n", argv[0]);
        exit(1);
    }
    if ((dirp = opendir(argv[1])) == NULL) {                                                                            // second argument must be a valid directory path
        perror(argv[1]);
        exit(2);
    }
    FILE *file_ptr = fopen("files.txt", "a");
    while ((direntp = readdir(dirp)) != NULL) {                                                                         // alternative to chdir()
        if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0)
            continue;
        sprintf(name, "%s/%s", argv[1], direntp->d_name);
        if (lstat(name, &stat_buf) == -1) {                                                                             // if there is some lstat of error
            perror("lstat error");
            int error = errno;
            if (error == EACCES)                                                                                        // if permission denied on that directory
                fprintf(stderr, "in %s\n\n", name);
        }
        if (S_ISREG(stat_buf.st_mode)) {                                                                                // if regular file, save file info
            saveFileInfo(argv[1], direntp->d_name, &stat_buf, file_ptr);
        }
        else if (S_ISDIR(stat_buf.st_mode)) {                                                                           // if directory start new lsdir process
            if ((pid = fork()) == -1) {
                perror("Error creating fork.\n");
                exit(3);
            }
            if (pid == 0)
                execlp("./lsdir", "./lsdir", name, NULL);
            else wait(&status);
        }
    }
    fclose(file_ptr);
    closedir(dirp);
    exit(0);
}
