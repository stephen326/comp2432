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
    char order[20];
    char due[11]; // 日期格式为 YYYY-MM-DD，包括空字符
    int qty[20]; // 数量以字符串形式存储
    char product[20];
} Order;


#define NUMBER_OF_CHILD 3
#define READY "R" // student can send this signal to parent to state that he finish on receive 
#define END "E" // END game signal

int findIndex(int arr[], int size, int value) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (arr[i] == value) {
            return i;
        }
    }
    return -1;
}

void executeMainPLS(char algorithm[], char outputFileName[]) {
    int i = 0, n = 0;

    /*
    before running PLS, we need to read the orders from the files
    file format (line by line): orderNumber dueDate quantity productName
    example: "P0001 2024-06-10 2000 Product_A" (end with \n)
    */
    // unfinished
    /*
    FILE *file;
    char line[100];
    Order orders[100]; // assume there are at most 100 orders
    int orderCount = 0;
    file = fopen("All_Orders.txt", "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s %d %s", orders[orderCount].order, orders[orderCount].due, &orders[orderCount].qty, orders[orderCount].product);
        orderCount++;
    }
    */

    // variables for pipe and fork
    int   childPids[NUMBER_OF_CHILD];
    int   p2cPipe[NUMBER_OF_CHILD][2]; // pipe sent from parent to child
    int   c2pPipe[NUMBER_OF_CHILD][2]; // pipe sent from child to parent
    //int   cNum, endGame = 0;
    char  buf[80];
    
    // creating pipes
    for (i = 0; i < NUMBER_OF_CHILD; i++) {
        if (pipe(p2cPipe[i]) == -1 || pipe(c2pPipe[i]) == -1) {
            perror("pipe"); // raise error for pipe creation
            exit(1); // exit with error code
        }
    }

    // creating childs
    int cpid = 0;
    int root = getppid();
    for (i = 0; i < NUMBER_OF_CHILD; i++) {
        if (getppid() == root) { // if not child
            cpid = fork();
        }
        if (cpid == 0) {
            childPids[i] = getpid();
        } else {
            childPids[i] = cpid;
        }
    }

    if (getppid() == root) {
        ////////////////////////// PARENT RUN THIS //////////////////////////

        // close unused pipe ends
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            close(p2cPipe[i][0]); // close parent in
            close(c2pPipe[i][1]); // close parent out
        }

        ///////////////// PARENT LOGIC START //////////////////

        // send ready signal to all child
        printf("Parent: Sending ready signal to all child\n");
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            write(p2cPipe[i][1], READY, sizeof(READY));
        }



        ///////////////// PARENT LOGIC END ////////////////////

        // close all pipes
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            close(p2cPipe[i][1]); // close parent out
            close(c2pPipe[i][0]); // close parent in
        }
        // wait for all child to finish
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            wait(NULL);
        }
        // exit parent
        exit(0);
        
    } else {
        ////////////////////////// CHILD RUN THIS //////////////////////////

        // get child index
        int myIndex = findIndex(childPids, NUMBER_OF_CHILD, getpid());
        if (myIndex == -1) {
            perror("child index not found, unexpected error");
            exit(1);
        }

        // close unused pipe ends
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            if (i != myIndex) {
                close(p2cPipe[i][1]); // close child out
                close(c2pPipe[i][0]); // close child in
            }
        }

        // print out child index (from 0) and pid
        printf("Child %d with PID: %d\n", myIndex, getpid());

        // NOTE: child can read from p2cPipe[myIndex][0] and write to c2pPipe[myIndex][1]

        ///////////////// CHILD LOGIC START //////////////////

        // wait until parent to send ready signal
        while (1) {
            n = read(p2cPipe[myIndex][0], buf, 80);
            if (n > 0 && strcmp(buf, READY) == 0) {
                printf("Child %d received ready signal from parent\n", myIndex);
                break;
            }
        }


        ///////////////// CHILD LOGIC END ////////////////////

        // write to parent that child is done
        write(c2pPipe[myIndex][1], END, sizeof(END));
        
        // close remaining pipes
        close(p2cPipe[myIndex][0]); // close child in
        close(c2pPipe[myIndex][1]); // close child out

        // exit child
        exit(0);
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
    char algorithm[20] = "Algorithm0"; // for testing only
    char outputFileName[20] = "output"; // for testing only
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
            sscanf(input, "%*s %s | printREPORT > %s", algorithm, outputFileName);
            // 调用同一文件夹中的main.c中的main函数，并传递算法和输出文件名作为参数
            executeMainPLS(algorithm, outputFileName);
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