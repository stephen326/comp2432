#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 定义订单结构体
typedef struct {
    char orderNumber[20];
    char dueDate[11]; // 日期格式为 YYYY-MM-DD，包括空字符
    int quantity;
    char productName[20];
    int priority; // 优先级
} Order;


void addPeriod(char startDate[], char endDate[]) {
    printf("Period added: %s to %s\n", startDate, endDate);
    // 在这里你可以将周期存储到相应的数据结构中

    FILE *file;
    file = fopen("periods.txt", "a"); // 打开文件以追加模式写入
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }

    // 将时间段写入文件
    fprintf(file, "%s %s\n", startDate, endDate);

    fclose(file);
}




void addOrder(char orderNumber[], char dueDate[], int quantity, char productName[]) {
    FILE *file;
    // 根据产品名称确定文件名
    char fileName[20];
    if (strcmp(productName, "Product_A") == 0) {
        strcpy(fileName, "orders_A.txt");
    } else if (strcmp(productName, "Product_B") == 0) {
        strcpy(fileName, "orders_B.txt");
    } else if (strcmp(productName, "Product_C") == 0) {
        strcpy(fileName, "orders_C.txt");
    } else {
        printf("Unknown product\n");
        return;
    }

    // 打开文件以读取模式
    file = fopen(fileName, "r");
    if (file == NULL) {
        // 如果文件不存在，则创建文件
        file = fopen(fileName, "w");
        if (file == NULL) {
            printf("Error creating file\n");
            return;
        }
    } else {
        // 检查文件中是否已存在相同的订单号
        char line[100];
        while (fgets(line, sizeof(line), file)) {
            // 检查每一行是否包含相同的订单号
            char *token = strtok(line, " ");
            if (token != NULL && strcmp(token, orderNumber) == 0) {
                printf("Order with the same order number already exists\n");
                fclose(file);
                return;
            }
        }
    }

    fclose(file);

    // 打开文件以追加模式写入
    file = fopen(fileName, "a");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }

    // 将订单信息写入文件
    fprintf(file, "%s %s %d %s\n", orderNumber, dueDate, quantity, productName);

    fclose(file);
}


//完全是为了留档，下面这个代码如果出现对同一个订单号重复输入会有就会有重复订单。上面的修改版解决了这个问题
// void addOrder(char orderNumber[], char dueDate[], int quantity, char productName[]) {
//     FILE *file;
//     // 根据产品名称确定文件名
//     char fileName[20];
//     if (strcmp(productName, "Product_A") == 0) {
//         strcpy(fileName, "orders_A.txt");
//     } else if (strcmp(productName, "Product_B") == 0) {
//         strcpy(fileName, "orders_B.txt");
//     } else if (strcmp(productName, "Product_C") == 0) {
//         strcpy(fileName, "orders_C.txt");
//     } else {
//         printf("Unknown product\n");
//         return;
//     }

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

int main() {
    // 示例用法
    addPeriod("2024-06-01", "2024-06-30");
    addOrder("P0001", "2024-06-10", 2000, "Product_A");
    addBatch("orderBATCH01.dat");

    return 0;
}
