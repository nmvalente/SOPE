#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/wait.h>

#define MAX_LENGTH 512

struct FileInfo {
    char* name;
    char* path;
    time_t mtime;
    int inode;
    int size;
    char permissions[10];
};


int runlsdir(char* directory) {
    pid_t pid;
    int status;
    FILE* file_ptr;                                                                                                     // header of the file
    file_ptr = fopen("files.txt", "w");
    if (file_ptr == NULL) {
        perror("Error opening files.txt.\n");
        return 1;
    }
    fclose(file_ptr);
    if ((pid = fork()) == -1) {
        perror("Error creating fork.\n");
        return -1;
    }
    if (pid == 0)
        execlp("./lsdir", "./lsdir", directory, NULL);
    else wait(&status);
    pid = fork();
    if (pid == 0)
        execlp("sort", "sort", "files.txt", "-k", "11,11", "-k", "8,8", "-o", "files_sorted.txt", NULL);
    else wait(&status);
    return 0;
}

int getNumberFiles(FILE* file_ptr) {
    char* line = NULL;
    size_t line_length = 0;
    int numberFiles = 0;
    while (getline(&line, &line_length, file_ptr) != -1) numberFiles++;
    rewind(file_ptr);
    return numberFiles;
}

int getFileInfos(struct FileInfo* fileInfos, FILE* file_ptr) {                                                          // need to add error checking

    char inode_str[9];
    char time_str[11];
    char size_str[10];
    char* line = NULL;
    size_t line_length = 0;
    ssize_t real_lentgh;
    int file_index = 0;
    int path_index;

    while ((real_lentgh = getline(&line, &line_length, file_ptr)) != -1) {
        line[real_lentgh - 1] = '\0';
        struct FileInfo fileInfo;
        memcpy(inode_str, &line[0], 8);
        inode_str[8] = '\0';
        fileInfo.inode = atoi(inode_str);
        memcpy(fileInfo.permissions, &line[9], 9);
        fileInfo.permissions[9] = '\0';
        memcpy(time_str, &line[44], 10);
        time_str[10] = '\0';
        fileInfo.mtime = (time_t)(atoll(time_str));
        memcpy(size_str, &line[55], 9);
        size_str[9] = '\0';
        fileInfo.size = atoi(size_str);
        fileInfo.path = malloc((int)(real_lentgh - 66) * sizeof(char));
        memcpy(fileInfo.path, &line[65], real_lentgh - 66);
        fileInfo.path[real_lentgh - 66] = '\0';
        path_index = (int)(real_lentgh - 67);
        while(fileInfo.path[path_index]) {
            if (fileInfo.path[path_index] == ' ') {
                fileInfo.path[path_index] = '/';
                break;
            }
            path_index--;
        }
        fileInfo.name = basename(fileInfo.path);
        fileInfos[file_index] = fileInfo;
        file_index++;
        printf("|%s|\n", line);
        printf("|%d|\n", fileInfo.inode);
        printf("|%s|\n", fileInfo.permissions);
        printf("|%-10lld|\n", (long long) fileInfo.mtime);
        printf("|%d|\n", fileInfo.size);
        printf("|%s|\n", fileInfo.path);
        printf("|%s|\n", fileInfo.name);
    }

    free(line);
    fclose(file_ptr);
    return 0;

}

int compareFileContents(char* path1, char* path2) {
    printf("Comparing file contents: %s, %s\n", path1, path2);
    pid_t pid;
    int status;
    int link[2];
    char whatever[4096];
    if (pipe(link) == -1) {
        perror("Error opening pipe.\n");
        return -1;
    }
    if ((pid = fork()) == -1) {
        perror("Error creating fork.\n");
        return -1;
    }
    if (pid == 0) {
        dup2(link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        execlp("diff", "diff", path1, path2, NULL);
    } else wait(&status);

    close(link[1]);
    int nbytes = (int) read(link[0], whatever, sizeof(whatever));
    if (nbytes == 0) {
        printf("Same file contents.\n");
        return 0;
    }
    printf("Different file contents.\n");
    return 1;
}

int compareFiles(struct FileInfo file1, struct FileInfo file2) {
    if (strcmp(file1.name, file2.name) != 0) {                                                                          // different file names return 2;
        printf("Different file names: %s, %s.\n", file1.path, file2.path);
        return 2;
    }
    if (strcmp(file1.permissions, file2.permissions) != 0 || file1.size != file2.size) {                                // same file name, but different permissions or sizes, return 1
        printf("Different permissions or sizes: %s, %s.\n", file1.path, file2.path);
        return 1;
    }
    return compareFileContents(file1.path, file2.path);                                                                 // same contents return 0, different contents return 1, error return -1
}

int createHardLink(struct FileInfo file1, struct FileInfo file2) {
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {                                                                                                    // number of arguments must be 2
        fprintf(stderr, "Usage: %s dir_name\n", argv[0]);
        exit(1);
    }

    if (runlsdir(argv[1])) exit(EXIT_FAILURE);                                                                          // run lsdir and sort files.txt

    FILE* file_ptr = fopen("files_sorted.txt", "r");
    if (file_ptr == NULL) {
        perror("Error opening files_sorted.txt.\n");
        exit(EXIT_FAILURE);
    }
    int numberFiles = getNumberFiles(file_ptr);                                                                         // determine number of files
    struct FileInfo* fileInfos = malloc(numberFiles * sizeof(struct FileInfo));
    if (getFileInfos(fileInfos, file_ptr)) exit(EXIT_FAILURE);                                                          // parse sorted file informations into array of structs

    struct FileInfo currentFile = fileInfos[0];
    struct FileInfo otherFile;
    int i = 0;
    int j = 1;
    while (i < numberFiles - 1) {
        while (j < numberFiles -1) {
            otherFile = fileInfos[j];
            int compare = compareFiles(currentFile, otherFile);
            switch (compare) {
                case 2:
                    j = numberFiles - 1;
                    break;
                case 1:
                    j++;
                    break;
                case 0:
                    createHardLink(currentFile, otherFile);
                    j++;
                    break;
                default:
                    j++;
                    break;
            }
        }
        currentFile = fileInfos[++i];
        j = i + 1;
    }
    exit(0);
}

