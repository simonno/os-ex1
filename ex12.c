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
    char  gradeDescription[MAX_PATH_LENGTH];
    int   grade;
} StudentGrade;


int isDir(char* name, char* path);

int isCFile(char* name);

char *appendPath(char name[256], char location[MAX_PATH_LENGTH]);

StudentGrade searchForCFile(char *studentName, char *path, char *location, char *outputLocation, int depth);

StudentGrade compileCFile(char *studentName, char *path, char *location, char *outputLocation, int depth);

StudentGrade runCFile(char *studentName, char *inputLocation, char *outputLocation, char *fileName, int depth);

int compareFile(char *fileLocation1, char *fileLocation2);

void updateGrade(StudentGrade *pStudentGrade, int depth);

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
    if(dup2(fdResults,1) < 0) {
        perror(DUP2_ERROR);
        exit(FAILURE);
    }


    struct dirent *pDirent;

    while ( (pDirent = readdir(pDir) ) != NULL ) {
        char * subDirPath = appendPath(pDirent->d_name, dirLocation);
        if (isDir(pDirent->d_name, subDirPath)) { // case this is a directory of a student.
            StudentGrade studentGrade = searchForCFile(pDirent->d_name, subDirPath,inputLocation, outputLocation, 0);
            printf("%s,%d,%s\n", studentGrade.name, studentGrade.grade, studentGrade.gradeDescription);
        }
        free(subDirPath);
    }
}

StudentGrade searchForCFile(char *studentName, char *dirLocation, char *inputLocation, char *outputLocation, int depth) {
    struct dirent *pDirent;
    char * singleDirPath = NULL;
    StudentGrade studentGrade = {studentName, "NO_C_FILE", 0};
    int dirCounter = 0;

    // open directory.
    DIR* pDir = opendir(dirLocation);
    if (pDir == NULL) {
        perror(OPEN_DIR_ERROR);
        exit(-1);
    }

    while ( (pDirent = readdir(pDir) ) != NULL ) {
        char * itemPath = appendPath(pDirent->d_name, dirLocation);
        if (isDir(pDirent->d_name, itemPath)) { // case this is a directory of a student.
            dirCounter++;
            if (dirCounter == 1) {
                singleDirPath = itemPath;
                continue;
            }
        } else if (isCFile(itemPath)){
            studentGrade = compileCFile(studentName, itemPath, inputLocation, outputLocation, depth);
            free(itemPath);
            return studentGrade;
        }
        free(itemPath);
    }

    // case there are MULTIPLE_DIRECTORIES and there isn't a C file.
    if (dirCounter > 1) {
        strcpy(studentGrade.gradeDescription, "MULTIPLE_DIRECTORIES");
        return studentGrade;
    }

    // case there is a single dir an no c file - keep looking for the C file;
    if (dirCounter == 1) {
        studentGrade = searchForCFile(studentName, singleDirPath, inputLocation, outputLocation, ++depth);
        free(singleDirPath);
        return studentGrade;
    }

    // case there isn't a C file - return "NO_C_FILE"
    return studentGrade;
}

StudentGrade compileCFile(char *studentName, char *path, char *inputLocation, char *outputLocation, int depth) {
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
            StudentGrade studentGrade = {studentName, "COMPILATION_ERROR", 0};
            return studentGrade;
        }

        return runCFile(studentName, inputLocation, outputLocation, compiledFileName, depth);
    }

}

StudentGrade runCFile(char *studentName, char *inputLocation, char *outputLocation, char *fileName, int depth) {
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
                    if  (depth == 0) {
                        strcpy(studentGrade.gradeDescription, "GREAT_JOB");
                    } else {
                        strcpy(studentGrade.gradeDescription, "WRONG_DIRECTORY");
                    }
                    updateGrade(&studentGrade, depth);
                    studentGrade.grade = 100;
                    break;
                case 2:
                    if  (depth == 0) {
                        strcpy(studentGrade.gradeDescription, "SIMILLAR_OUTPUT");
                    } else {
                        strcpy(studentGrade.gradeDescription, "SIMILLAR_OUTPUT,WRONG_DIRECTORY");
                    }
                    updateGrade(&studentGrade, depth);
                    studentGrade.grade = 70;
                    break;
                case 3:
                    if  (depth == 0) {
                        strcpy(studentGrade.gradeDescription, "BAD_OUTPUT");
                    } else {
                        strcpy(studentGrade.gradeDescription, "BAD_OUTPUT,WRONG_DIRECTORY");
                    }
                    studentGrade.grade = 0;
                    break;
                default: //shouldn't get here.
                    exit(FAILURE);
            }
            unlink(fileName);
            unlink(outputName);
            return studentGrade;
        }
    }
}

void updateGrade(StudentGrade *pStudentGrade, int depth) {
    if (depth <= 0)
        return;

    int grade = pStudentGrade->grade - depth * 10;
    if (grade <= 0) {
        pStudentGrade->grade = 0;
    } else {
        pStudentGrade->grade = grade;
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