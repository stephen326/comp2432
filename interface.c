#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


// 定义订单结构体
typedef struct {
    char orderNumber[20];
    char dueDate[11]; // 日期格式为 YYYY-MM-DD，包括空字符
    int quantity[20]; // 数量以字符串形式存储
    char productName[20];
} Order;


#define NUMBER_OF_CHILD 3
#define READY "R" // student can send this signal to parent to state that he finish on receive 
#define END "E" // END game signal


void executeMainLogic() {
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
}




void addPeriod(char startDate[], char endDate[]) {
    printf("Period added: %s to %s\n", startDate, endDate);

    FILE *file;
    
    // 删除已存在的文件
    remove("periods.txt");

    file = fopen("periods.txt", "a"); // 打开文件以追加模式写入
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }

    // 将时间段写入文件
    fprintf(file, "%s %s\n", startDate, endDate);

    fclose(file);
}


int checkDuplicate(char fileName[], char orderNumber[]) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        return 0; // 文件不存在，不存在重复
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, " ");
        if (token != NULL && strcmp(token, orderNumber) == 0) {
            fclose(file);
            return 1; // 存在重复
        }
    }
    fclose(file);
    return 0; // 不存在重复
}

void addOrder(char orderNumber[], char dueDate[], int quantity, char productName[]) {
    FILE *file;
    char fileName[20];
    char endDate[11]; // 保存期望的截止日期

    // 打开 period.txt 文件以读取期望的截止日期
    file = fopen("periods.txt", "r");
    if (file == NULL) {
        printf("Error opening periods.txt\n");
        return;
    }

    // 读取第二个日期
    if (fscanf(file, "%*s %s", endDate) != 1) {
        printf("Error reading end date from periods.txt\n");
        fclose(file);
        return;
    }
    fclose(file);

    // 根据产品名称确定文件名
    if (strcmp(productName, "Product_A") == 0 || strcmp(productName, "Product_B") == 0 || strcmp(productName, "Product_C") == 0) {
        strcpy(fileName, "Category_1.txt");
    } 
    else if (strcmp(productName, "Product_D") == 0 || strcmp(productName, "Product_E") == 0 || strcmp(productName, "Product_F") == 0) {
        strcpy(fileName, "Category_2.txt");
    } 
    else if (strcmp(productName, "Product_G") == 0 || strcmp(productName, "Product_H") == 0 || strcmp(productName, "Product_I") == 0) {
        strcpy(fileName, "Category_3.txt");
    }
    else {
        printf("Invalid product name\n");
        return;
    }

    // 检查截止日期是否超出范围
    if (strcmp(dueDate, endDate) > 0) {
        printf("Due date exceeds the specified period\n");
        return;
    }

    // 检查分类文件中是否存在相同的订单号
    if (checkDuplicate(fileName, orderNumber)) {
        printf("Order with the same order number already exists in %s\n", fileName);
        return;
    }

    // 打开分类文件以追加模式写入
    file = fopen(fileName, "a");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    // 将订单信息写入分类文件
    fprintf(file, "%s %s %d %s\n", orderNumber, dueDate, quantity, productName);
    fclose(file);

    // 打开总订单文件以追加模式写入
    file = fopen("All_Orders.txt", "a");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    // 将订单信息写入总订单文件
    fprintf(file, "%s %s %d %s\n", orderNumber, dueDate, quantity, productName);
    fclose(file);
}



// 添加批量订单
void addBatch(char batchFile[]) {
    FILE *file = fopen(batchFile, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }

    char orderNumber[20], dueDate[11], productName[20];
    int quantity;

    // 逐行读取批量文件
    while (fscanf(file, "%s %s %d %s", orderNumber, dueDate, &quantity, productName) != EOF) {
        addOrder(orderNumber, dueDate, quantity, productName);
    }

    fclose(file);
}




void runPLS(char algorithm[]) {
    // 在这里执行 PLS 算法
    printf("Running PLS with algorithm: %s\n", algorithm);
}

void printREPORT(char outputFile[]) {
    // 在这里打印报告
    printf("Printing report to file: %s\n", outputFile);
}


int main() {
    char input[100];
    char command[20];
    char arg1[20], arg2[20], arg3[20], arg4[20];
    char algorithm[20];
    char outputFile[20];
    char batchFile[20];

    printf("~~WELCOME TO PLS~~\n");

    while (1) {
        printf("Please enter:\n> ");
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%s", command);

        if (strcmp(command, "addPEIOD") == 0) {
            sscanf(input, "%*s %s %s", arg1, arg2);
            addPeriod(arg1, arg2);
        } else if (strcmp(command, "addORDER") == 0) {
            sscanf(input, "%*s %s %s %s %s", arg1, arg2, arg3, arg4);
            // Convert arg3 to integer
            int quantity = atoi(arg3);
            addOrder(arg1, arg2, quantity, arg4);
        } else if (strcmp(command, "addBATCH") == 0) {
            sscanf(input, "%*s %s", batchFile);
            addBatch(batchFile);
        }
        else  if (strcmp(command, "runPLS") == 0) {
            sscanf(input, "%*s %s | printREPORT > %s", algorithm, outputFile);
            // 调用同一文件夹中的main.c中的main函数，并传递算法和输出文件名作为参数
            executeMainLogic();
        }
         
        
        // else if (strcmp(command, "runPLS") == 0) {
        //     sscanf(input, "%*s %s", algorithm);
        //     runPLS(algorithm);
        // } else if (strcmp(command, "printREPORT") == 0) {
        //     sscanf(input, "%*s %s", outputFile);
        //     printREPORT(outputFile);
        // }

         else if (strcmp(command, "exitPLS") == 0) {
            printf("Bye-bye!\n");
            exit(0);
        } else {
            printf("Invalid command\n");
        }
    }

    return 0;
}




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