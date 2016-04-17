#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/wait.h>

#define PERMISSIONS_LENGTH 10
#define INODE_LENGTH 9
#define TIME_LENGTH 11
#define SIZE_LENGTH 10
#define BEFORE_PATH_LENGTH 65
#define BEFORE_TIME_LENGTH 44

#define LOG 1

struct FileInfo {
    int number;
    int inode;
    char permissions[PERMISSIONS_LENGTH];
    time_t mtime;
    int size;
    char *path;
    char *name;
};

int runlsdir(char *directory) {
    pid_t pid;
    int status;
    FILE *file_ptr = fopen("files.txt", "w");
    if (file_ptr == NULL) {
        perror("Error opening files.txt.\n");
        return -1;
    }
    fclose(file_ptr);
    if ((pid = fork()) == -1) {
        perror("Error creating fork.\n");
        return -1;
    }
    if (pid == 0)
        execlp("./lsdir", "./lsdir", directory, NULL);
    else wait(&status);
    if ((pid = fork()) == -1) {
        perror("Error creating fork.\n");
        return -1;
    }
    if (pid == 0)
        execlp("sort", "sort", "files.txt", "-k", "11,11", "-k", "8,8", "-o", "files_sorted.txt", NULL);
    else wait(&status);
    return 0;
}

int getNumberFiles(FILE *file_ptr) {
    char *line = NULL;
    size_t line_length = 0;
    int numberFiles = 0;
    while (getline(&line, &line_length, file_ptr) != -1) numberFiles++;
    rewind(file_ptr);
    return numberFiles;
}

int getFileInfos(struct FileInfo **fileInfos, FILE *file_ptr) {
    char inode_str[INODE_LENGTH];
    char time_str[TIME_LENGTH];
    char size_str[SIZE_LENGTH];
    char *line = NULL;
    char *name;
    int name_length;
    size_t line_length = 0;
    ssize_t real_lentgh;
    int file_index = 0;
    int path_index;
    while ((real_lentgh = getline(&line, &line_length, file_ptr)) != -1) {
        if (real_lentgh < BEFORE_PATH_LENGTH + 2) {
            printf("Error parsing file info.\n");
            return -1;
        }
        line[real_lentgh - 1] = '\0';
        struct FileInfo *fileInfo = malloc(sizeof(struct FileInfo));
        fileInfo->number = file_index;
        memcpy(inode_str, &line[0], INODE_LENGTH - 1);
        inode_str[INODE_LENGTH - 1] = '\0';
        fileInfo->inode = atoi(inode_str);
        memcpy(fileInfo->permissions, &line[INODE_LENGTH], PERMISSIONS_LENGTH - 1);
        fileInfo->permissions[PERMISSIONS_LENGTH - 1] = '\0';
        memcpy(time_str, &line[BEFORE_TIME_LENGTH], TIME_LENGTH - 1);
        time_str[TIME_LENGTH - 1] = '\0';
        fileInfo->mtime = (time_t) (atoll(time_str));
        memcpy(size_str, &line[BEFORE_TIME_LENGTH + TIME_LENGTH], SIZE_LENGTH - 1);
        size_str[SIZE_LENGTH - 1] = '\0';
        fileInfo->size = atoi(size_str);
        fileInfo->path = malloc((int) (real_lentgh - BEFORE_PATH_LENGTH) * sizeof(char));
        memcpy(fileInfo->path, &line[BEFORE_PATH_LENGTH], real_lentgh - BEFORE_PATH_LENGTH - 1);
        fileInfo->path[real_lentgh - BEFORE_PATH_LENGTH - 1] = '\0';
        path_index = (int) (real_lentgh - BEFORE_PATH_LENGTH - 2);
        while (fileInfo->path[path_index]) {
            if (fileInfo->path[path_index] == ' ') {
                fileInfo->path[path_index] = '/';
                break;
            }
            path_index--;
        }
        name = basename(fileInfo->path);
        name_length = (int) strlen(name);
        fileInfo->name = malloc(name_length * sizeof(char));
        memcpy(fileInfo->name, &name[0], name_length);
        fileInfo->name[name_length] = '\0';
        fileInfos[file_index] = fileInfo;
        if (LOG) {
            printf("----------------------------------------------------------------------------------------------------\n");
            printf("file: %d\n", fileInfo->number);
            printf("inode: %d\n", fileInfo->inode);
            printf("permissions: %s\n", fileInfo->permissions);
            printf("date: %-10lld\n", (long long) fileInfo->mtime);
            printf("size: %d\n", fileInfo->size);
            printf("path: %s\n", fileInfo->path);
            printf("name: %s\n", fileInfo->name);
        }
        file_index++;
    }
    if (LOG)
        printf("----------------------------------------------------------------------------------------------------\n");
    free(line);
    fclose(file_ptr);
    return 0;
}

int compareFileContents(struct FileInfo *file1, struct FileInfo *file2) {
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
        execlp("diff", "diff", file1->path, file2->path, NULL);
    } else wait(&status);
    close(link[1]);
    int nbytes = (int) read(link[0], whatever, sizeof(whatever));
    if (nbytes == 0) {
        if (LOG) printf("Same file contents: %d, %d\n", file1->number, file2->number);
        return 0;
    }
    if (LOG) printf("Different file contents: %d, %d\n", file1->number, file2->number);
    return 1;
}

int compareFiles(struct FileInfo *file1, struct FileInfo *file2) {
    if (file1->inode == file2->inode) {
        if (LOG) printf("Same inodes: %d, %d\n", file1->number, file2->number);
        return 3;                                                                                                       // same inodes return 3;
    }
    if (strcmp(file1->name, file2->name)) {                                                                             // different file names return 2;
        if (LOG) printf("Different file names: %d, %d.\n", file1->number, file2->number);
        return 2;
    }
    if (strcmp(file1->permissions, file2->permissions)) {                                                               // same file name, but different permissions, return 1
        if (LOG) printf("Different permissions: %d, %d.\n", file1->number, file2->number);
        return 1;
    }
    if (file1->size != file2->size) {                                                                                   // same file name, same permissions, but different sizes, return 1
        if (LOG) printf("Different sizes: %d, %d.\n", file1->number, file2->number);
        return 1;
    }
    return compareFileContents(file1, file2);                                                                           // same contents return 0, different contents return 1, error return -1
}

int createHardLink(struct FileInfo *file1, struct FileInfo *file2) {
    int unlink_res = unlink(file2->path);
    if (unlink_res) {
        perror("Error unlinking file.\n");
        return unlink_res;
    }
    int link_res = link(file1->path, file2->path);
    if (link_res) perror("Error linking files.\n");
    return link_res;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {                                                                                                    // number of arguments must be 2
        fprintf(stderr, "Usage: %s dir_name\n", argv[0]);
        exit(1);
    }
    if (runlsdir(argv[1]))                                                                                              // run lsdir and sort files.txt
        exit(2);
    FILE *file_ptr = fopen("files_sorted.txt", "r");
    if (file_ptr == NULL) {
        perror("Error opening files_sorted.txt.\n");
        exit(3);
    }
    int numberFiles = getNumberFiles(file_ptr);                                                                         // determine number of files
    struct FileInfo **fileInfos = malloc(numberFiles * sizeof(struct FileInfo*));
    if (getFileInfos(fileInfos, file_ptr))                                                                              // parse sorted file informations into array of structs
        exit(4);
    int i = 0;
    int j = 1;
    while (i < numberFiles - 1) {
        while (j < numberFiles) {
            int compare = compareFiles(fileInfos[i], fileInfos[j]);
            switch (compare) {
                case 3:                                                                                                 // same inode, no need to create hardlink, already done
                    j++;
                    break;
                case 2:                                                                                                 // different names, no need to compare with the rest, since the files are sorted
                    j = numberFiles;
                    break;
                case 1:                                                                                                 // different permissions, sizes or contents, move to the next file
                    j++;
                    break;
                case 0:                                                                                                 // same contents, create hardlink
                    createHardLink(fileInfos[i], fileInfos[j]);                                                         // does not stop on hardlink error, continues to next file
                    j++;
                    break;
                default:                                                                                                // does not stop on compare error, continues to next file
                    j++;
                    break;
            }
        }
        i++;
        j = i + 1;
    }
    exit(0);
}

