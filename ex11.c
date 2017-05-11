/********************************
* Student name: Noam Simon      *
* Student ID: 208388850         *
* Course Exercise Group: 04     *
* Exercise name: Exercise 1.1   *
********************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 20

int isSimilar(char first, char second);

int needToIgnore(char c);

int theRemained(char *buffer, int fd, int i, int read);

/*******************************************************************************
* function name : main                                                         *
* input : path for 2 files .                                                   *
* output : 3 if not equal, 2 if similar, 1 if equal.                           *
* explanation : the program compare between the to file and return the result. *
*******************************************************************************/
int main(int argc, char* argv[]) {
    int returnValue = 1;
    int fd1;
    int fd2;
    char buffer1[SIZE];
    char buffer2[SIZE];
    int read1;
    int read2;
    /* create the file with read only permissions */
    fd1 = open(argv[1],O_RDONLY);
    fd2 = open(argv[2],O_RDONLY);

    // check if the opening failed.
    if (fd1 < 0 || fd2 < 0) {
        perror("open error");
        exit(-1);
    }

    // read [SIZE] bytes form the files.
    read1 = (int) read(fd1, buffer1, SIZE);
    read2 = (int) read(fd2, buffer2, SIZE);

    int j = 0;
    int i = 0;
    // start the compare operation.
    while (read1 > 0 && read2 > 0) {
        if (buffer1[i] == buffer2[j]){ // the bytes are equal - continue to the next bytes.
            i++;
            j++;
        } else if (isSimilar(buffer1[i], buffer2[j])) {
            // the bytes are similar - change the return value and continue to the next bytes.
            i++;
            j++;
            returnValue = 2;
        } else if (needToIgnore(buffer1[i])) {
            i++;
            returnValue = 2;
        } else if (needToIgnore(buffer2[j])) {
            j++;
            returnValue = 2;
        } else {
            // the bytes are diff - change the return value and stop the comparison,
            returnValue = 3;
            break;
        }

        // got to the end of the buffer - read another [SIZE] bytes.
        if (i == read1) {
            read1 = (int) read(fd1, buffer1, SIZE);
            i = 0;
        }

        if (j == read2) {
            read2 = (int) read(fd2, buffer2, SIZE);
            j = 0;
        }
    }

    // case the file are similar or equal to that point, check the rest of the file that wasn't done.
    if (returnValue != 3) {
        if (read1 > 0) {
            returnValue = theRemained(buffer1, fd1, i, read1);
        } else if (read2 > 0) {
            returnValue = theRemained(buffer2, fd2, j, read2);
        }
    }

    close(fd1);		/* free allocated structures */
    close(fd2);		/* free allocated structures */

    return returnValue;
}

/************************************************************************************
* function name : theRemained                                                       *
* input : buffer, file descriptor, current index, how many read from file.          *
* output : 3 if not equal, 2 if similar.                                            *
* explanation : if one file was done and there is chars in the second file we need  *
*               to check if there is new lines or spaces and then the file are      *
*               similar and if there is something else the files are not equal.     *
************************************************************************************/
int theRemained(char *buffer, int fd, int i, int readBytes) {
    do {
        while (i < readBytes) {
            if (!needToIgnore(buffer[i])) {
                return  3;
            }
            i++;
        }
        readBytes = (int) read(fd, buffer, SIZE);
        i = 0;
    } while (readBytes > 0);
    return 2;
}

/*****************************************************************
* function name : isSimilar                                      *
* input : two chars.                                             *
* output : 1 if true 0 if false.                                 *
* explanation : check if the two chars are similar (not equal).  *
******************************************************************/
int isSimilar(char first, char second) {
    if ((((first >= 92) && first <= 122) && (first == second + 32)) ||
        ((second >= 92) && (second <= 122) && (second == first + 32)))
        return 1;
    return 0;
}

/*******************************************************************************
* function name : needToIgnore                                                 *
* input : char.                                                                *
* output : 1 if true 0 if false.                                               *
* explanation : check if need to ignore the char (equal to new line or space). *
*******************************************************************************/
int needToIgnore(char c) {
    if ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))
        return 1;
    return 0;
}