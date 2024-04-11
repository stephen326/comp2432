/**
 * This code has been integrated into interface.c.
 * This file is provided for reference purposes only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define NUMBER_OF_CHILD 3
#define READY "R" // student can send this signal to parent to state that he finish on receive 
#define TOKEN "T" // parent will assign this token to the leading child
#define END "E" // END game signal
// Cannot use char(s): A, K, Q, J, T, S, H, C, D

int main(int argc, char *argv[]) {
    int   childPids[NUMBER_OF_CHILD];
    int   p2cPipe[NUMBER_OF_CHILD][2]; // pipe sent from parent to child
    int   c2pPipe[NUMBER_OF_CHILD][2]; // pipe sent from child to parent
    int   n, cNum, endGame = 0;
    char  buf[80];


    for (cNum = 0; cNum < NUMBER_OF_CHILD; cNum++) { // create pipe and child processes
        if (pipe(p2cPipe[cNum]) < 0) {
            printf("P2C Pipe creation error\n");
            exit(1);
        }
        if (pipe(c2pPipe[cNum]) < 0) {
            printf("C2P Pipe creation error\n");
            exit(1);
        }
        if ((childPids[cNum] = fork()) < 0) {
            printf("Fork failed\n");
            exit(1);
        } else if (childPids[cNum] == 0) { // child
            printf("Child process %d with PID: %d\n", cNum+1, getpid());
            close(p2cPipe[cNum][1]); // close child out (This pipe is only read from parent)
            close(c2pPipe[cNum][0]); // close child in (This pipe is only write to parent)
            if ((n = read(p2cPipe[cNum][0], buf, 80)) > 0) { // read from pipe
                printf("Child %d received message from parent: %s\n", cNum+1, buf);
                //开始读取文件了，进行后面的两个步骤，Output Format和Analysis
            }

            write(c2pPipe[cNum][1], END, sizeof(END)); //子告诉父后面俩步骤完成了

            close(p2cPipe[cNum][0]);
            close(c2pPipe[cNum][1]);
            exit(0);
        } else { // parent
            printf("Parent created child process %d with PID: %d\n", cNum+1, childPids[cNum]);
            close(p2cPipe[cNum][0]); // close parent in (This pipe is only read from child)
            close(c2pPipe[cNum][1]); // close parent out (This pipe is only receive from child)
            
            write(p2cPipe[cNum][1], READY, sizeof(READY)); // 父亲给孩子说你可以开始读取文件了，进行后面的两个步骤，Output Format和Analysis
            
            if ((n = read(c2pPipe[cNum][0], buf, 80)) > 0) { // 子给父说后面2步骤完成了过后父亲有个反应
                printf("Parent received message from child %d: %s\n", cNum+1, buf);
                //随便有个反应就行，目的是为了满足有fork有pipe
            }
            close(p2cPipe[cNum][1]); // close parent out
            close(c2pPipe[cNum][0]); // close parent in
        

        }
    }
    return 0;
}

// Original code of interface.c child&pipe part
/*
    for (cNum = 0; cNum < NUMBER_OF_CHILD; cNum++) { // create pipe and child processes
        if (pipe(p2cPipe[cNum]) < 0) {
            printf("P2C Pipe creation error\n");
            exit(1);
        }
        if (pipe(c2pPipe[cNum]) < 0) {
            printf("C2P Pipe creation error\n");
            exit(1);
        }
        if ((childPids[cNum] = fork()) < 0) {
            printf("Fork failed\n");
            exit(1);
        } else if (childPids[cNum] == 0) { // child
            printf("Child process %d with PID: %d\n", cNum+1, getpid());
            close(p2cPipe[cNum][1]); // close child out (This pipe is only read from parent)
            close(c2pPipe[cNum][0]); // close child in (This pipe is only write to parent)
            if ((n = read(p2cPipe[cNum][0], buf, 80)) > 0) { // read from pipe
                printf("Child %d received message from parent: %s\n", cNum+1, buf);
                //开始读取文件了，进行后面的两个步骤，Output Format和Analysis
            }

            write(c2pPipe[cNum][1], END, sizeof(END)); //子告诉父后面俩步骤完成了

            close(p2cPipe[cNum][0]);
            close(c2pPipe[cNum][1]);
            exit(0);
        } else { // parent
            printf("Parent created child process %d with PID: %d\n", cNum+1, childPids[cNum]);
            close(p2cPipe[cNum][0]); // close parent in (This pipe is only read from child)
            close(c2pPipe[cNum][1]); // close parent out (This pipe is only receive from child)
            
            write(p2cPipe[cNum][1], READY, sizeof(READY)); // 父亲给孩子说你可以开始读取文件了，进行后面的两个步骤，Output Format和Analysis
            
            if ((n = read(c2pPipe[cNum][0], buf, 80)) > 0) { // 子给父说后面2步骤完成了过后父亲有个反应
                printf("Parent received message from child %d: %s\n", cNum+1, buf);
                //随便有个反应就行，目的是为了满足有fork有pipe
            }
            close(p2cPipe[cNum][1]); // close parent out
            close(c2pPipe[cNum][0]); // close parent in
        

        }
    }
    */



   // Original commented code of interface.c

   // void addOrder(char orderNumber[], char dueDate[], int quantity, char productName[]) {
//     FILE *file;
//     char fileName[] = "plan.txt"; // 使用固定文件名

//     // 打开文件以读取模式
//     file = fopen(fileName, "r");
//     if (file == NULL) {
//         // 如果文件不存在，则创建文件
//         file = fopen(fileName, "w");
//         if (file == NULL) {
//             printf("Error creating file\n");
//             return;
//         }
//     } else {
//         // 检查文件中是否已存在相同的订单号
//         char line[100];
//         while (fgets(line, sizeof(line), file)) {
//             // 检查每一行是否包含相同的订单号
//             char *token = strtok(line, " ");
//             if (token != NULL && strcmp(token, orderNumber) == 0) {
//                 printf("Order with the same order number already exists\n");
//                 fclose(file);
//                 return;
//             }
//         }
//     }

//     fclose(file);

//     // 打开文件以追加模式写入
//     file = fopen(fileName, "a");
//     if (file == NULL) {
//         printf("Error opening file\n");
//         return;
//     }

//     // 将订单信息写入文件
//     fprintf(file, "%s %s %d %s\n", orderNumber, dueDate, quantity, productName);

//     fclose(file);
// }

// void addOrder2(char orderNumber[], char dueDate[], int quantity, char productName[]) {
//     FILE *file;
//     // 根据产品名称确定文件名
//     char fileName[20];
//     if (strcmp(productName, "Product_A") == 0||strcmp(productName, "Product_B") == 0||strcmp(productName, "Product_C") == 0) {
//         strcpy(fileName, "Category_1.txt");
//     } else if (strcmp(productName, "Product_D") == 0||strcmp(productName, "Product_E") == 0||strcmp(productName, "Product_F") == 0) {
//         strcpy(fileName, "Category_2.txt");
//     } else if (strcmp(productName, "Product_G") == 0||strcmp(productName, "Product_H") == 0||strcmp(productName, "Product_I") == 0) {
//         strcpy(fileName, "Category_3.txt");
//     } else {
//         printf("Unknown product\n");
//         return;
//     }

//     // 打开文件以读取模式
//     file = fopen(fileName, "r");
//     if (file == NULL) {
//         // 如果文件不存在，则创建文件
//         file = fopen(fileName, "w");
//         if (file == NULL) {
//             printf("Error creating file\n");
//             return;
//         }
//     } else {
//         // 检查文件中是否已存在相同的订单号
//         char line[100];
//         while (fgets(line, sizeof(line), file)) {
//             // 检查每一行是否包含相同的订单号
//             char *token = strtok(line, " ");
//             if (token != NULL && strcmp(token, orderNumber) == 0) {
//                 printf("Order with the same order number already exists\n");
//                 fclose(file);
//                 return;
//             }
//         }
//     }

//     fclose(file);

//     // 打开文件以追加模式写入
//     file = fopen(fileName, "a");
//     if (file == NULL) {
//         printf("Error opening file\n");
//         return;
//     }

//     // 将订单信息写入文件
//     fprintf(file, "%s %s %d %s\n", orderNumber, dueDate, quantity, productName);

//     fclose(file);
// }