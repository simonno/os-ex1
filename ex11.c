#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 20


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
        } else if (buffer1[i] + 32 == buffer2[j] || buffer1[i] == buffer2[j] + 32) {
            i++;
            j++;
            returnValue = 2;
        } else if (buffer1[i] == ' ' || buffer1[i] == '\n') {
            i++;
            returnValue = 2;
        } else if (buffer2[j] == ' ' || buffer2[j] == '\n') {
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
    int returnValue = 2;
    while(readBytes > 0 && buffer[i] != EOF) {
        if (buffer[i] != ' ' && buffer[i] != '\n') {
            returnValue = 3;
            break;
        }
        if (i == readBytes) {
            readBytes = (int) read(fd, buffer, SIZE);
            i = 0;
            continue;
        }
        i++;
    }
    return returnValue;
}



//        switch (compareBuffers(buffer1, buffer2, read1, read2)) {
//            case 1:
//                continue;
//            case 2:
//                returnValue = 2;
//                continue;
//            case 3:
//                close(fd1);		/* free allocated structures */
//                close(fd2);		/* free allocated structures */
//                return 3;
//            default:
//                break;
//        }

//int compareBuffers(char buffer1[SIZE], char buffer2[SIZE], int read1, int read2) {
//    int returnValue = 1;
//    int i = 0, j = 0;
//    while (i < read1 && j < read2) {
//        if (buffer1[i] == buffer2[j]){
//            i++;
//            j++;
//            continue;
//        }
//        if (buffer1[i] + 32 == buffer2[j] || buffer1[i] == buffer2[j] + 32) {
//            i++;
//            j++;
//            returnValue = 2;
//            continue;
//        }
//        if (buffer1[i] == ' ') {
//            i++;
//        }
//        if (buffer2[j] == ' ') {
//            j++;
//        }
//        return 3;
//    }
//    return returnValue;
//}