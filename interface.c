#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// 定义订单结构体
typedef struct {
    char orderNumber[20];
    char dueDate[11]; // 日期格式为 YYYY-MM-DD，包括空字符
    int quantity[20]; // 数量以字符串形式存储
    char productName[20];
} Order;


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

    // 根据产品名称确定文件名
    if (strcmp(productName, "Product_A") == 0 || strcmp(productName, "Product_B") == 0 || strcmp(productName, "Product_C") == 0) {
        strcpy(fileName, "Category_1.txt");
        
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
    } 
    if (strcmp(productName, "Product_D") == 0 || strcmp(productName, "Product_E") == 0 || strcmp(productName, "Product_F") == 0) {
        strcpy(fileName, "Category_2.txt");

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
    } 
    if (strcmp(productName, "Product_G") == 0 || strcmp(productName, "Product_H") == 0 || strcmp(productName, "Product_I") == 0) {
        strcpy(fileName, "Category_3.txt");

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
    }

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
        } else if (strcmp(command, "runPLS") == 0) {
            sscanf(input, "%*s %s", algorithm);
            runPLS(algorithm);
        } else if (strcmp(command, "printREPORT") == 0) {
            sscanf(input, "%*s %s", outputFile);
            printREPORT(outputFile);
        } else if (strcmp(command, "exitPLS") == 0) {
            printf("Bye-bye!\n");
            exit(0);
        } else {
            printf("Invalid command\n");
        }
    }

    return 0;
}

