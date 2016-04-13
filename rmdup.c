#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>

#define MAX_LENGTH 512

struct FileInfo {
    char* name;
    char* path;
    time_t mtime;
    int inode;
    int size;
    char permissions[10];
};

int main(int argc, char *argv[]) {

    pid_t pid;
    int status;
    char* line;
    char inode_str[9];
    char time_str[11];
    char size_str[10];
    size_t line_length;
    struct FileInfo* fileInfos;
    int numberFiles = 0;

    if (argc != 2) {                                                                                                    // number of arguments must be 2
        fprintf(stderr, "Usage: %s dir_name\n", argv[0]);
        exit(1);
    }

    pid = fork();
    if (pid == 0)
        execlp("./lsdir", "./lsdir", argv[1], NULL);
    else wait(&status);

    FILE* file_ptr;                                                                                                     // header of the file
    file_ptr = fopen("files.txt", "r");

    if (file_ptr == NULL) {
        perror("Error opening files.txt.\n");
        exit(EXIT_FAILURE);
    }

    while (getline(&line, &line_length, file_ptr) != -1) numberFiles++;

    fileInfos = malloc(numberFiles * sizeof(struct FileInfo));
    rewind(file_ptr);

    int i = 0;
    ssize_t real_lentgh;
    while ((real_lentgh = getline(&line, &line_length, file_ptr)) != -1) {
        line[real_lentgh - 1] = '\0';
        printf("|%s|\n", line);
        struct FileInfo fileInfo;
        memcpy(inode_str, &line[0], 8);
        inode_str[8] = '\0';
        fileInfo.inode = atoi(inode_str);
        printf("|%d|\n", fileInfo.inode);
        memcpy(fileInfo.permissions, &line[9], 9);
        fileInfo.permissions[9] = '\0';
        printf("|%s|\n", fileInfo.permissions);
        memcpy(time_str, &line[44], 10);
        time_str[10] = '\0';
        fileInfo.mtime = (time_t)(atoll(time_str));
        printf("|%-10lld|\n", (long long) fileInfo.mtime);
        memcpy(size_str, &line[55], 9);
        size_str[9] = '\0';
        fileInfo.size = atoi(size_str);
        printf("|%d|\n", fileInfo.size);
        fileInfo.path = malloc((int)(real_lentgh - 66) * sizeof(char));
        memcpy(fileInfo.path, &line[65], real_lentgh - 66);
        fileInfo.path[real_lentgh - 66] = '\0';
        printf("|%s|\n", fileInfo.path);
        fileInfo.name = basename(fileInfo.path);
        printf("|%s|\n", fileInfo.name);
        fileInfos[i] = fileInfo;
        i++;
    }
    
    fclose(file_ptr);
    exit(0);
}


// context, name, permissions and regulars
bool compareContextFiles(char *fname1, char *fname2) {
    if (fname1 == fname2) return true;

    int ch1, ch2;
    FILE *fp1, *fp2;
    fp1 = fopen(fname1, "r");
    fp2 = fopen(fname2, "r");

    if (fp1 == NULL) {
        printf("Cannot open %s for reading ", fname1);
        exit(1);
    }
    else if (fp2 == NULL) {
        printf("Cannot open %s for reading ", fname2);
        exit(1);
    }
    else {
        ch1 = getc(fp1);
        ch2 = getc(fp2);

        while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2)) {
            ch1 = getc(fp1);
            ch2 = getc(fp2);
        }
        fclose(fp1);
        fclose(fp2);
    }
    if (ch1 == ch2)
        return true;
    return false;
}