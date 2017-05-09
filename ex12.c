//
// Created by noam on 07/05/17.
//


#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <wait.h>
#include <string.h>


#define MAX_PATH_LENGTH 160
#define OPEN_CONFIG_FILE_ERROR "open config file error"
#define OPEN_OUTPUT_FILE_ERROR "open output file error"
#define OPEN_RESULTS_FILE_ERROR "open results file error"
#define OPEN_DIR_ERROR "open dir error"
#define FORK_ERROR "fork error"
#define WAITPID_ERROR "waitpid error"
#define DUP2_ERROR "dup2 error"
#define EXEC_ERROR "EXEC error"

#define FAILURE -1

typedef struct StudentGrade {
    char*  name;
    char*  gradeDescription;
    int   grade;
} StudentGrade;


int isDir(char* name, char* path);

int isCFile(char* name);

void subDir(char *dirLocation);

char *appendPath(char name[256], char location[MAX_PATH_LENGTH]);

StudentGrade searchForCFile(char *studentName, char *path, char *location, char *outputLocation);

StudentGrade compileCFile(char *studentName, char *path, char *location, char *outputLocation);

StudentGrade runCFile(char *studentName, char *inputLocation, char *outputLocation, char *fileName);

int compareFile(char *fileLocation1, char *fileLocation2);

int main(int argc, char* argv[]) {
    int fdConfig;
    int readBytes;
    char dirLocation[MAX_PATH_LENGTH];
    char inputLocation[MAX_PATH_LENGTH];
    char outputLocation[MAX_PATH_LENGTH];
    /* create the file with read only permissions */
    fdConfig = open(argv[1],O_RDONLY);
    if (fdConfig < 0) {
        perror(OPEN_CONFIG_FILE_ERROR);
        exit(-1);
    }

    dup2(fdConfig, 0);
    /* split config file. */
    scanf("%s" ,dirLocation);
    scanf("%s" ,inputLocation);
    scanf("%s" ,outputLocation);
    // close config file.
    close(fdConfig);

    // open directory.
    DIR* pDir = opendir(dirLocation);
    if (pDir == NULL) {
        perror(OPEN_DIR_ERROR);
        exit(-1);
    }

    int fdResults = open("results.cvs", O_APPEND | O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fdResults < 0) {
        perror(OPEN_RESULTS_FILE_ERROR);
        exit(FAILURE);
    }
    dup2(fdResults, 1);


    struct dirent *pDirent;

    while ( (pDirent = readdir(pDir) ) != NULL ) {
        if (isDir(pDirent->d_name, pDirent->d_name)) { // case this is a directory of a student.
            char * subDirPath = appendPath(pDirent->d_name, dirLocation);
            StudentGrade studentGrade = searchForCFile(pDirent->d_name, subDirPath,inputLocation, outputLocation);
            printf("%s,%d,%s\n", studentGrade.name, studentGrade.grade, studentGrade.gradeDescription);
            free(subDirPath);
        }
    }
}

StudentGrade searchForCFile(char *studentName, char *dirLocation, char *inputLocation, char *outputLocation) {
    struct dirent *pDirent;
    StudentGrade studentGrade = {studentName, "FILE_C_NO", 0};

    // open directory.
    DIR* pDir = opendir(dirLocation);
    if (pDir == NULL) {
        perror(OPEN_DIR_ERROR);
        exit(-1);
    }

    while ( (pDirent = readdir(pDir) ) != NULL ) {
        char * itemPath = appendPath(pDirent->d_name, dirLocation);
        if (isDir(pDirent->d_name, itemPath)) { // case this is a directory of a student.
            studentGrade = searchForCFile(studentName, itemPath,inputLocation, outputLocation);
            free(itemPath);
            return studentGrade;

        } else if (isCFile(itemPath)){
            studentGrade = compileCFile(studentName, itemPath, inputLocation, outputLocation);
            free(itemPath);
            return studentGrade;
        }
    }
    return studentGrade;
}

StudentGrade compileCFile(char *studentName, char *path, char *inputLocation, char *outputLocation) {
    int status;
    pid_t pid;

    char compiledFileName[MAX_PATH_LENGTH];
    strcpy(compiledFileName, studentName);
    strcat(compiledFileName, ".out");

    if ((pid = fork()) < 0) {
        perror(FORK_ERROR);

    } else if (pid == 0) { /* first child */
        char *args[] = {"gcc", "-o", compiledFileName, path, NULL};
        execvp("gcc", args);
        exit(FAILURE);
    }

    if (waitpid(pid, &status, 0) != pid)  /* wait for first child */
        perror(WAITPID_ERROR);
    if (WIFEXITED(status) ) {
        if (WEXITSTATUS(status) == 1) {
            // COMPILATION_ERROR
            StudentGrade studentGrade = {studentName, "ERROR_COMPILATION", 0};
            return studentGrade;
        }

        return runCFile(studentName, inputLocation, outputLocation, compiledFileName);
    }

}

StudentGrade runCFile(char *studentName, char *inputLocation, char *outputLocation, char *fileName) {
    int status;
    pid_t pid;

    char outputName[MAX_PATH_LENGTH];
    //creating output file name
    strcpy(outputName, studentName);
    strcat(outputName, ".txt");
    int myOutputFD = open(outputName, O_APPEND | O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (myOutputFD < 0) {
        perror(OPEN_OUTPUT_FILE_ERROR);
        exit(FAILURE);
    }

    if ((pid = fork()) < 0) {
        perror(FORK_ERROR);
        exit(FAILURE);
    }
    if (pid == 0) {/* second  child */
        if(dup2(myOutputFD,1) < 0) {
            perror(DUP2_ERROR);
            exit(FAILURE);
        }

        char execLine[MAX_PATH_LENGTH];
        strcpy(execLine, "./");
        strcat(execLine, fileName);
        char *args[] = {fileName, inputLocation, NULL};
        execvp(execLine, args);
        perror(EXEC_ERROR);
        exit(FAILURE);
    } else {

        sleep(5);
        pid_t state = waitpid(pid, &status, WNOHANG);  /* wait for first child */

        if (state < 0) {
            perror(WAITPID_ERROR);
            exit(FAILURE);
        } else if (state == 0) {
            StudentGrade studentGrade = {studentName, "TIMEOUT", 0};
            return studentGrade;
        } else {
            StudentGrade studentGrade;
            studentGrade.name = studentName;
            switch(compareFile(outputName, outputLocation)){
                case 1:
                    studentGrade.gradeDescription = "JOB_GREAT";
                    studentGrade.grade = 100;
                    break;
                case 2:
                    studentGrade.gradeDescription = "OUTPUT_SIMILLAR";
                    studentGrade.grade = 70;
                    break;
                case 3:
                    studentGrade.gradeDescription = "OUTPUT_BAD";
                    studentGrade.grade = 0;
                    break;
                default: //shouldn't get hear.
                    exit(FAILURE);
            }
            return studentGrade;
        }
    }
}

int compareFile(char *fileLocation1, char *fileLocation2) {
    int status;
    pid_t pid;

    if ((pid = fork()) < 0) { // fork a child process
        perror(FORK_ERROR);
        exit(FAILURE);
    } else if (pid == 0) { // for the child process:
        //running comp.out
        char *args[] = {"comp.out", fileLocation1, fileLocation2, NULL};
        execvp("./comp.out", args);
        perror(EXEC_ERROR);
        exit(FAILURE);
    } else {
        if (waitpid(pid, &status, 0) != pid) { /* wait for first child */
            perror(WAITPID_ERROR);
            exit(FAILURE);
        }
        if (WIFEXITED(status) ) {
            int i = WEXITSTATUS(status);
            return i;
        }
        return -1;
    }
}


int isCFile(char* name) {
    struct stat statP;
    stat(name, &statP);
    int len = (int) strlen(name);
    if ((len > 2) && (name[len - 1]=='c') && (name[len - 2]=='.'))
        return 1;
    return 0;
}

char * appendPath(char name[256], char location[MAX_PATH_LENGTH]) {
    char * newDirPath = (char*) malloc(strlen(location)+strlen(name)+2);
    newDirPath[0] = '\0';   // ensures the memory is an empty string
    strcat(newDirPath, location);
    strcat(newDirPath, "/");
    strcat(newDirPath, name);
    return  newDirPath;
}

int isDir(char* name, char* path) {
    struct stat statP;
    stat(path, &statP);
    if((S_ISDIR(statP.st_mode)) && (strcmp(name ,".") != 0) && (strcmp(name ,"..") != 0))
        return 1;
    return 0;
}

void subDir(char* dirLocation) {
//    DIR* pDir = opendir(dirLocation);
//    if (pDir == NULL) {
//        perror("open dir error");
//        exit(-1);
//    }
//
//
//    struct dirent *pDirent;
//    struct stat stat_p;
//    pid_t	pid;
//
//    // looping through the directory, printing the directory entry name
//    while ( (pDirent = readdir(pDir) ) != NULL ) {
//        if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
//            continue; /* skip self and parent */
//
//        char * subDirPath ;
//        subDirPath = (char*) malloc(strlen(dirLocation)+strlen(pDirent->d_name)+2);
//        subDirPath[0] = '\0';   // ensures the memory is an empty string
//        strcat(subDirPath, dirLocation);
//        strcat(subDirPath, "/");
//        strcat(subDirPath, pDirent->d_name);
//        stat(subDirPath, &stat_p);
//
//
//        if (S_ISDIR(stat_p.st_mode)) {
//            subDir(subDirPath);
//        } else if (S_ISREG(stat_p.st_mode)) {
//            if ((pid = fork()) < 0) {
//                perror("fork error");
//            } else {
//                if (pid == 0) { /* first child */
//                    char *args[] = {"gcc", subDirPath, NULL };
//                    execvp("gcc", args);
//                }
//                int status;
//                if (waitpid(pid, &status, 0) != pid)  /* wait for first child */
//                    perror("waitpid error");
//                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
//                    printf("%s - COMPILATION_ERROR\n", subDirPath);
//                    return;
//                }
//                if ((pid = fork()) < 0) {
//                    perror("fork error");
//                }
//                if (pid == 0) {    /* second  child */
//                    char *args[] = {"a.out", NULL};
//                    execvp("a.out", args);
//                }
//
//            }
//        }
//
//    }
}

