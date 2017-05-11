/********************************
* Student name: Noam Simon      *
* Student ID: 208388850         *
* Course Exercise Group: 04     *
* Exercise name: Exercise 1.1   *
********************************/

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
#define OPEN_INPUT_FILE_ERROR "open input file error"
#define OPEN_RESULTS_FILE_ERROR "open results file error"
#define OPEN_DIR_ERROR "open dir error"
#define FORK_ERROR "fork error"
#define WAITPID_ERROR "waitpid error"
#define DUP2_ERROR "dup2 error"
#define EXEC_ERROR "EXEC error"

#define FAILURE -1

// struct of student's grade.
typedef struct StudentGrade {
    char*  name;
    char  gradeDescription[MAX_PATH_LENGTH];
    int   grade;
} StudentGrade;


int isDir(char* name, char* path);

int isCFile(char* name);

char *appendPath(char name[256], char location[MAX_PATH_LENGTH]);

StudentGrade searchForCFile(char *studentName, char *path, int location, char *outputLocation, int depth);

StudentGrade compileCFile(char *studentName, char *path, int location, char *outputLocation, int depth);

StudentGrade runCFile(char *studentName, int fdInput, char *outputLocation, char *fileName, int depth);

int compareFile(char *fileLocation1, char *fileLocation2);

void updateGrade(StudentGrade *pStudentGrade, int depth);

/********************************************************************************
* function name : main                                                          *
* input : path to config file                                                   *
* output : 0 for success, -1 for failure.                                       *
* explanation : try to compile and run the c file of the students in the dir.   *
*               print the grades to results.cvs file.                           *
*********************************************************************************/
int main(int argc, char* argv[]) {
    int fdConfig;
    char dirLocation[MAX_PATH_LENGTH];
    char inputLocation[MAX_PATH_LENGTH];
    char outputLocation[MAX_PATH_LENGTH];
    /* create the file with read only permissions */
    fdConfig = open(argv[1],O_RDONLY);
    if (fdConfig < 0) {
        perror(OPEN_CONFIG_FILE_ERROR);
        exit(FAILURE);
    }

    // change stdin to the config file.
    int copySTDIN = dup2(fdConfig, 0);
    if (copySTDIN < 0){
        perror(DUP2_ERROR);
        exit(FAILURE);
    }
    /* split config file. */
    scanf("%s" ,dirLocation);
    scanf("%s" ,inputLocation);
    scanf("%s" ,outputLocation);
    // close config file.
    close(fdConfig);
    // change back to the stdin.
    if (dup2(copySTDIN, 0) < 0){
        perror(DUP2_ERROR);
        exit(FAILURE);
    }

    // open the inputFile
    int fdInput = open(inputLocation, O_RDONLY);
    if (fdInput < 0) {
        perror(OPEN_INPUT_FILE_ERROR);
        exit(FAILURE);
    }

    // open directory.
    DIR* pDir = opendir(dirLocation);
    if (pDir == NULL) {
        perror(OPEN_DIR_ERROR);
        exit(FAILURE);
    }

    // create the result file.
    int fdResults = open("results.cvs", O_APPEND | O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fdResults < 0) {
        perror(OPEN_RESULTS_FILE_ERROR);
        exit(FAILURE);
    }
    // all the prints of the program will be printed to the results file.
    int copySTDOUT = dup2(fdResults,1);
    if(copySTDOUT < 0) {
        perror(DUP2_ERROR);
        exit(FAILURE);
    }


    struct dirent *pDirent;
    // running over the students dirs.
    while ( (pDirent = readdir(pDir) ) != NULL ) {
        char * subDirPath = appendPath(pDirent->d_name, dirLocation);
        if (isDir(pDirent->d_name, subDirPath)) { // case this is a directory of a student.
            StudentGrade studentGrade = searchForCFile(pDirent->d_name, subDirPath,fdInput, outputLocation, 0);
            printf("%s,%d,%s\n", studentGrade.name, studentGrade.grade, studentGrade.gradeDescription);
        }
        free(subDirPath);
    }

    // change back - stdout.
    if(dup2(copySTDOUT,1) < 0) {
        perror(DUP2_ERROR);
        exit(FAILURE);
    }

    // close the dir and input file.
    close(fdInput);
    closedir( pDir );
    return 0;
}

/****************************************************************************
* function name : searchForCFile                                            *
* input : student name, path of sub dir of the student, input file.         *
*         output file , depth of the c file.                                *
* output : the grade of the student.                                        *
* explanation : search the c file by recursion.                             *
****************************************************************************/
StudentGrade searchForCFile(char *studentName, char *dirLocation, int fdInput, char *outputLocation, int depth) {
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

    // running over the di and search a c file.
    while ((pDirent = readdir(pDir)) != NULL ) {
        char * itemPath = appendPath(pDirent->d_name, dirLocation);
        if (isDir(pDirent->d_name, itemPath)) { // case this is a directory of a student.
            dirCounter++;
            if (dirCounter == 1) {
                singleDirPath = itemPath;
                continue;
            }
        } else if (isCFile(itemPath)){
            studentGrade = compileCFile(studentName, itemPath, fdInput, outputLocation, depth);
            free(itemPath);
            closedir(pDir);
            return studentGrade;
        }
        free(itemPath);
    }

    // case there are MULTIPLE_DIRECTORIES and there isn't a C file.
    if (dirCounter > 1) {
        strcpy(studentGrade.gradeDescription, "MULTIPLE_DIRECTORIES");
        closedir( pDir );
        return studentGrade;
    }

    // case there is a single dir an no c file - keep looking for the C file;
    if (dirCounter == 1) {
        studentGrade = searchForCFile(studentName, singleDirPath, fdInput, outputLocation, ++depth);
        free(singleDirPath);
        closedir( pDir );
        return studentGrade;
    }

    // case there isn't a C file - return "NO_C_FILE"
    closedir( pDir );
    return studentGrade;
}

/****************************************************************************
* function name : compileCFile                                              *
* input : student name, path of the c file, input file,                     *
*         output file , depth of the c file.                                *
* output : the grade of the student.                                        *
* explanation : try to compile the c file, return the grade of the student  *
****************************************************************************/
StudentGrade compileCFile(char *studentName, char *path, int fdInput, char *outputLocation, int depth) {
    int status;
    pid_t pid;

    // create the name of the compiled file - "[studentName].out" .
    char compiledFileName[MAX_PATH_LENGTH];
    strcpy(compiledFileName, studentName);
    strcat(compiledFileName, ".out");

    if ((pid = fork()) < 0) {
        perror(FORK_ERROR);
        exit(FAILURE);

    } else if (pid == 0) { /* first child */
        // running gcc for compile the c file/
        char *args[] = {"gcc", "-o", compiledFileName, path, NULL};
        execvp("gcc", args);

        //shouldn't get here.
        perror(EXEC_ERROR);
        exit(FAILURE);

    } else if (waitpid(pid, &status, 0) != pid) {  /* wait for first child */
        perror(WAITPID_ERROR);
        exit(FAILURE);
    } else if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 1) {
            // COMPILATION_ERROR
            StudentGrade studentGrade = {studentName, "COMPILATION_ERROR", 0};
            return studentGrade;
        }

        // the file compiled properly - run the program and return the grade.
        return runCFile(studentName, fdInput, outputLocation, compiledFileName, depth);
    }
    //shouldn't get here.
    exit(FAILURE);
}

/****************************************************************************
* function name : runCFile                                                  *
* input : student name, input file, output file for comparision,            *
*         name of the compiled file, and his depth.                         *
* output : the grade of the student.                                        *
* explanation : run the c file, and compare the outputs of this program to  *
*               the output file, return the grade of the student according  *
*               to the comparision.                                         *
****************************************************************************/
StudentGrade runCFile(char *studentName, int fdInput, char *outputLocation,
                      char *fileName, int depth) {
    int status;
    pid_t pid;

    char myOutputPath[MAX_PATH_LENGTH];
    //creating output file name for the c program.
    strcpy(myOutputPath, studentName);
    strcat(myOutputPath, ".txt");
    int fdOutput = open(myOutputPath, O_APPEND | O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fdOutput < 0) {
        perror(OPEN_OUTPUT_FILE_ERROR);
        exit(FAILURE);
    }

    if ((pid = fork()) < 0) {
        perror(FORK_ERROR);
        exit(FAILURE);
    }

    int copySTDOUT = dup2(fdOutput,1);
    int copySTDIN = dup2(fdInput, 0);
    if( copySTDIN < 0 || copySTDOUT < 0) {
        perror(DUP2_ERROR);
        exit(FAILURE);
    }

    if (pid == 0) {/* second  child */
        // run the compiled file.
        char execLine[MAX_PATH_LENGTH];
        strcpy(execLine, "./");
        strcat(execLine, fileName);
        //char *args[] = {fileName, NULL};
        execlp(execLine, fileName, NULL);

        //shouldn't get here.
        perror(EXEC_ERROR);
        exit(FAILURE);
    } else {

        // sleep 5 sec to check time out.
        sleep(5);
        pid_t state = waitpid(pid, &status, WNOHANG);  /* wait for first child */

        if (state < 0) {
            perror(WAITPID_ERROR);
            exit(FAILURE);
        } else {
            StudentGrade studentGrade;
            studentGrade.name = studentName;
            if (state == 0) { // the program wasn't done after 5 sec - timeout.
                strcpy(studentGrade.gradeDescription, "TIMEOUT");
                studentGrade.grade = 0;

            } else {// the program was done properly - calculate the grade.

                /* compare the output files and return the grade of
                 * the student according to the comparision. */
                switch(compareFile(myOutputPath, outputLocation)){
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
            }

            // change back the stdin/out.
            if( dup2(copySTDIN, 0) < 0 || dup2(copySTDOUT, 1) < 0) {
                perror(DUP2_ERROR);
                exit(FAILURE);
            }

            // closing and removed the files.
            close(fdOutput);
            unlink(fileName);
            unlink(myOutputPath);
            return studentGrade;
        }
    }
}

/*************************************************************************
* function name : updateGrade                                            *
* input : pointer to a StudentGrade, a depth of the c file.              *
* output : the updated grade.                                            *
* explanation : updated the grate according to the depth of the c file . *
*************************************************************************/
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

/*********************************************************************
* function name : compareFile                                        *
* input : 2 files Path.                                              *
* output : return the result of the comparision between the files.   *
* explanation : run thr comp program, that compare between the files *
*               and return the result.                               *
**********************************************************************/
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
            // return the result.
            return WEXITSTATUS(status);
        }
        return -1;
    }
}

/*****************************************************************
* function name : isCFile                                        *
* input : file name.                                             *
* output : 1 if true 0 if false.                                 *
* explanation : check if a file name is a file name of a c file. *
*****************************************************************/
int isCFile(char* name) {
    struct stat statP;
    stat(name, &statP);
    int len = (int) strlen(name);
    if ((len > 2) && (name[len - 1]=='c') && (name[len - 2]=='.'))
        return 1;
    return 0;
}

/**********************************************************************
* function name : appendPath                                          *
* input : name of a sub dir or file in a dir, path to the father dir. *
* output : the new Path of the sub dir or the file.                   *
* explanation : append the name to the end of the path.               *
**********************************************************************/
char * appendPath(char name[256], char location[MAX_PATH_LENGTH]) {
    char * newDirPath = (char*) malloc(strlen(location)+strlen(name)+2);
    newDirPath[0] = '\0';   // ensures the memory is an empty string
    strcat(newDirPath, location);
    strcat(newDirPath, "/");
    strcat(newDirPath, name);
    return  newDirPath;
}

/*********************************************************************
* function name : isDir                                              *
* input : file name, and his path.                                   *
* output : 1 if true 0 if false.                                     *
* explanation : check if a file name is a file name of a directory   *
*               (and not '.' or '..').                               *
*********************************************************************/
int isDir(char* name, char* path) {
    struct stat statP;
    stat(path, &statP);
    if((S_ISDIR(statP.st_mode)) && (strcmp(name ,".") != 0) && (strcmp(name ,"..") != 0))
        return 1;
    return 0;
}