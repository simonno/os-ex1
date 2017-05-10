#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 20

int isSimilar(char first, char second);

int needToIgnore(char c);

int theRemained(char *buffer, int fd, int i, int read);

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

    if (fd1 < 0 || fd2 < 0) {
        perror("open error");
        exit(-1);
    }

    read1 = (int) read(fd1, buffer1, SIZE);
    read2 = (int) read(fd2, buffer2, SIZE);

    int j = 0;
    int i = 0;
    while (read1 > 0 && read2 > 0) {
        if (buffer1[i] == buffer2[j]){
            i++;
            j++;
        } else if (isSimilar(buffer1[i], buffer2[j])) {
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
            returnValue = 3;
            break;
        }

        if (i == read1) {
            read1 = (int) read(fd1, buffer1, SIZE);
            i = 0;
        }

        if (j == read2) {
            read2 = (int) read(fd2, buffer2, SIZE);
            j = 0;
        }
    }

    if (read1 > 0) {
        returnValue = theRemained(buffer1, fd1, i, read1);
    } else if (read2 > 0) {
        returnValue = theRemained(buffer2, fd2, j, read2);
    }

    close(fd1);		/* free allocated structures */
    close(fd2);		/* free allocated structures */

    return returnValue;
}

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


/*******************************************************************************
* function name : isSimilar                                                    *
* input : two chars.                                                           *
* output : 1 if true 0 if false.                                               *
* explanation : check if the two chars are similars (not equal).               *
*******************************************************************************/
int isSimilar(char first, char second)
{
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
int needToIgnore(char c)
{
    if ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))
        return 1;
    return 0;
}