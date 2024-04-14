#define _XOPEN_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ORDERS 10000 // 假设的最大订单数，你可以根据实际情况调整
#define MAX_ORDER_NUM 1000
#define MAX_REJECTED 2000 // Define the maximum number of rejected orders

int total_days3 = 0; // 定义工厂的生产天数
int total_quantity3 = 0; // 定义工厂的生产数量

int total_days3_X = 0; // 定义工厂X的生产天数
int total_quantity3_X = 0; // 定义工厂X的生产数量

int total_days3_Y = 0; // 定义工厂Y的生产天数
int total_quantity3_Y = 0; // 定义工厂Y的生产数量

int total_days3_Z = 0; // 定义工厂Z的生产天数
int total_quantity3_Z = 0; // 定义工厂Z的生产数量


#define DATE_LENGTH 11 // Date size including null terminator
#define MAX_PRODUCT_NAME 50 // Product name size


int orderCount3 = 0; // 已存储的订单数量

typedef struct {
    char productName[50]; // 产品名
    char plantName[10];   // 工厂名
    char startDate[11];   // 开始日期，格式为 YYYY-MM-DD
    char orderNumber[10]; // 订单编号
    int quantity;         // 订单数量
    char dueDate[11];     // 结束日期，格式为 YYYY-MM-DD
} Order3;

// Rejected order structure
typedef struct {
    char orderNumber[MAX_ORDER_NUM];
    char date[DATE_LENGTH];
    int quantity;
    char productName[MAX_PRODUCT_NAME];
} Rejected_Order3;

Order3 orders3[MAX_ORDERS]; // 存储所有订单


// 计算文件中有数据的行数
int countLinesWithData(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    int count = 0;
    char buffer[1024]; // 假定一行不会超过1024个字符，更适合一般的文本处理

    while (fgets(buffer, sizeof(buffer), file)) {
        // 使用strspn来跳过前导空白字符
        int index = strspn(buffer, " \t\n\r");
        // 确保这一行在去除前导空格后不是空的（即有数据的）
        if (buffer[index] != '\0' && buffer[index] != '\n') {
            count++;
        }
    }

    fclose(file);
    return count;
}
// 计算接受的订单数量
int countAcceptedOrders(const char *allOrdersFile, const char *rejectedOrdersFile) {
    int totalOrders = countLinesWithData(allOrdersFile);
    int rejectedOrders = countLinesWithData(rejectedOrdersFile);

    if (totalOrders == -1 || rejectedOrders == -1) {
        // 发生错误时返回-1
        return -1;
    }

    return totalOrders - rejectedOrders;
}

// 第一部分开头
int writeReportHeader(const char *algorithmName, int acceptedOrders) {
    FILE *reportFile;

    // 打开文件用于写入
    reportFile = fopen("Report.txt", "w");
    if (reportFile == NULL) {
        perror("Error opening file");
        return -1;
    }

    // 写入报告头部
    fprintf(reportFile, "***PLS Schedule Analysis Report***\n");
    fprintf(reportFile, "Algorithm used: %s\n", algorithmName);
    // 写入已接受的订单数
    fprintf(reportFile, "There are %d Orders ACCEPTED. Details are as follows:\n", acceptedOrders);
    fprintf(reportFile, "ORDER NUMBER\tSTART\t\tEND\t\t\t\tDAYS\tQUANTITY\tPLANT\n");
    fprintf(reportFile, "===========================================================================\n");
    
    // 关闭文件
    fclose(reportFile); 

    return 0; // 成功写入报告头部
}

// 添加订单
void addOrder3(const char* plantName, const char* productName, const char* startDate, const char* orderNumber, int quantity, const char* dueDate) {
    if (orderCount3 < MAX_ORDERS) {
        strcpy(orders3[orderCount3].productName, productName);
        strcpy(orders3[orderCount3].plantName, plantName);
        strcpy(orders3[orderCount3].startDate, startDate);
        strcpy(orders3[orderCount3].orderNumber, orderNumber);
        orders3[orderCount3].quantity = quantity;
        strcpy(orders3[orderCount3].dueDate, dueDate);
        orderCount3++;
    }
}


void processOrders(const char* filename) {
    char plantName[10];
    const char* suffix = ".dat"; // 定义后缀字符串
    size_t suffix_len = strlen(suffix);
    
    // 查找后缀在文件名中的位置
    char* suffix_position = strstr(filename, suffix);
    
    if (suffix_position != NULL) {
        // 计算后缀前的字符数量
        size_t plant_name_len = suffix_position - filename;
        if (plant_name_len < sizeof(plantName)) {
            // 复制后缀前的部分到 plantName
            strncpy(plantName, filename, plant_name_len);
            // 添加字符串结束符
            plantName[plant_name_len] = '\0';
        } else {
            // 如果工厂名称超出 plantName 数组大小，处理错误或直接返回
            printf("The name is too long！\n");
            return;
        }
    } else {
        // 找不到后缀，处理错误或直接返回
        printf("The file name isn't include '.dat'！\n");
        return;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error！\n");
        return;
    }
    char line[1024];
  
    // 跳过文件头部(6行)
    for (int i = 0; i < 6; ++i) {
        fgets(line, sizeof(line), file);
    }
  
    // 读取和解析数据行
    while (fgets(line, sizeof(line), file)) {
        if (strcmp(line, "---------------------------------------------------------------------\n") == 0) {
            break; // 到达文件尾部分隔线
        }

        char startDate[11], productName[50], orderNumber[10], dueDate[11];
        int quantity;
        // 解析数据行
        if (sscanf(line, "%10s | %49s | %9s | %d | %10s", startDate, productName, orderNumber, &quantity, dueDate) == 5) {
            

	 	addOrder3(plantName, productName, startDate, orderNumber, quantity, dueDate);
	
        }
    }
  
    // 关闭文件
    fclose(file);
}



// 比较两个订单，首先按订单号排序，如果订单号相同则按工厂名排序
int compareOrders(const void *a, const void *b) {
    const Order3 *orderA = (const Order3 *)a;
    const Order3 *orderB = (const Order3 *)b;
    int orderCompare = strcmp(orderA->orderNumber, orderB->orderNumber);
    if (orderCompare == 0) { // 如果订单号相同，比较工厂名
        return strcmp(orderA->plantName, orderB->plantName);
    }
    return orderCompare;
}

// 冒泡排序算法，根据订单号和工厂名排序
void sortOrders() {
    for (int i = 0; i < orderCount3 - 1; ++i) {
        for (int j = 0; j < orderCount3 - i - 1; ++j) {
            if (compareOrders(&orders3[j], &orders3[j + 1]) > 0) {
                // 交换订单
                Order3 temp = orders3[j];
                orders3[j] = orders3[j + 1];
                orders3[j + 1] = temp;
            }
        }
    }
}

// 函数用于在 orderCounts 数组中查找特定的订单号和工厂名
// 如果找到则返回索引，否则返回 -1
int findOrderCount(const char* orderNumber, const char* plantName) {
    for (int i = 0; i < orderCount3; i++) {
        if (strcmp(orders3[i].orderNumber, orderNumber) == 0 &&
            strcmp(orders3[i].plantName, plantName) == 0) {
            return i;
        }
    }
    return -1;
}

// 计算文件中所有日期区间的总天数
int calculate_total_days_in_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    struct tm start_tm = {0}, end_tm = {0};
    char start_date[11], end_date[11];
    int total_days3 = 0, days;
    time_t start_time, end_time;

    while (fscanf(file, "%10s %10s", start_date, end_date) == 2) {
        // 清除结构体数据
        memset(&start_tm, 0, sizeof(struct tm));
        memset(&end_tm, 0, sizeof(struct tm));

        // 解析开始日期
        if (strptime(start_date, "%Y-%m-%d", &start_tm) == NULL || *start_date == '\0') {
            fprintf(stderr, "Error parsing start date: %s\n", start_date);
            fclose(file);
            return -1;
        }

        // 解析结束日期
        if (strptime(end_date, "%Y-%m-%d", &end_tm) == NULL || *end_date == '\0') {
            fprintf(stderr, "Error parsing end date: %s\n", end_date);
            fclose(file);
            return -1;
        }

        // 计算时间差
        start_time = mktime(&start_tm);
        end_time = mktime(&end_tm);
        days = (int)difftime(end_time, start_time) / (24 * 3600) + 1;

        // 累加到总天数
        total_days3 += days;
    }

    fclose(file);
    return total_days3;
}


// 我不确定total day是什么意思。///////////////////////////////////////////////////////
void writeFinal3(const char* filename){
	FILE *file = fopen(filename, "a");
	if (file == NULL) {
        	printf("Error opening file to write!\n");
        	return;
    	}
	const char *period = "periods.txt";
    	int total_days3 = calculate_total_days_in_file(period);


	fprintf(file,"***PERFORMANCE\n\n");
	fprintf(file,"Plant_X:\n");
	fprintf(file,"\t\tNumber of days in use:                    \t%3d days\n", total_days3_X);
	fprintf(file,"\t\tNumber of products produced:       %6d (in total)\n", total_quantity3_X);
	// 确保至少一个数是浮点数，并且Utilization格式为??.? %
        float utilization1 = (total_days3_X > 0) ? ((float)total_quantity3_X / (total_days3*300)) * 100 : 0.0f;
        fprintf(file, "\t\tUtilization of the plant:                     %5.1f %%\n\n", utilization1);


	fprintf(file,"Plant_Y:\n");
	fprintf(file,"\t\tNumber of days in use:                    \t%3d days\n", total_days3_Y);
	fprintf(file,"\t\tNumber of products produced:       %6d (in total)\n",total_quantity3_Y);
	// 确保至少一个数是浮点数，并且Utilization格式为??.? %
        float utilization2 = (total_days3_Y > 0) ? ((float)total_quantity3_Y / (total_days3*400)) * 100 : 0.0f;
        fprintf(file, "\t\tUtilization of the plant:                     %5.1f %%\n\n", utilization2);

	fprintf(file,"Plant_Z:\n");
	fprintf(file,"\t\tNumber of days in use:                    \t%3d days\n", total_days3_Z);
	fprintf(file,"\t\tNumber of products produced:       %6d (in total)\n", total_quantity3_Z);
	// 确保至少一个数是浮点数，并且Utilization格式为??.? %
        float utilization3 = (total_days3_Z > 0) ? ((float)total_quantity3_Z / (total_days3*500)) * 100 : 0.0f;
        fprintf(file, "\t\tUtilization of the plant:                     %5.1f %%\n\n", utilization3);
	
	float utilization4 = (total_days3 > 0) ? ((float)total_quantity3 / (total_days3*1200)) * 100 : 0.0f;
	fprintf(file,"Overall of utilization:\t\t\t\t\t %5.1f %%\n\n", utilization4);
	
	pclose(file);

}
// 将整理之后的数组写入文件之中
/*
P0001 2024-06-01 2024-06-02 2 600 Plant_X
P0001 2024-06-01 1 400 Plant_Y
P0002 2024-06-01 2024-06-DD 2024-06-DD XX XXXX ???
… … …
… … …
… … …
- End -
*/
void writeOrdersToFile(const char* filename) {
    FILE *file = fopen(filename, "a");
    char str1[] = "Plant_X";
    char str2[] = "Plant_Y";
    char str3[] = "Plant_Z";
    if (file == NULL) {
        printf("Error opening file to write!\n");
        return;
    }
    
    for (int i = 0; i < orderCount3; i++) {
	// 判断orderNumber是否为"N/A"，如果不是"N/A"，则添加订单到数组
	Order3 order = orders3[i];
        if (strcmp(order.orderNumber, "N/A") != 0) {
        int occurrences = 1;
        
        int start = i; // Assuming this is meant to track the start index for occurrences
		
	int quantity = order.quantity;
	int days = 1;

        // Check for duplicate orders and calculate occurrences
        for (int j = i + 1; j < orderCount3; ++j) {
            if (strcmp(order.plantName, orders3[j].plantName) == 0 && strcmp(order.orderNumber, orders3[j].orderNumber) == 0) {
                
			occurrences++;	
			quantity += orders3[j].quantity;
			
			days += 1;
            }
        }
         
        total_quantity3 += quantity;
	total_days3 += days;
	if(strcmp(str1,order.plantName) == 0){
		 total_quantity3_X += quantity;
		 total_days3_X += days;
	}if(strcmp(str2,order.plantName) == 0){
		 total_quantity3_Y += quantity;
		 total_days3_Y += days;
	}if(strcmp(str3,order.plantName) == 0){
		 total_quantity3_Z += quantity;
		 total_days3_Z += days;
	}
	
        // Write order information
        fprintf(file, "%-8s\t\t\t %-10s\t %-10s\t\t %-2d\t\t %-6d \t\t%-8s\n",
                order.orderNumber,
                order.startDate,
                orders3[start + occurrences - 1].startDate, // Assuming this is the correct due date
                occurrences,
                quantity,
                order.plantName);

        // Skip over duplicate orders
        i += (occurrences - 1);
    }}

    // Write end marker
    fprintf(file, "\n\t\t\t- End -\n");
    fprintf(file, "===========================================================================\n\n\n");

    // Close the file
    fclose(file);
}

// 写入rejected_部分的表头
void appendReportWithRejectedCount(const char *rejectedOrdersFile, const char *reportFile) {
    int rejectedCount = 0;
    FILE *rejectedFile = fopen(rejectedOrdersFile, "r");
    FILE *reportFilePointer;
    char line[1024];

    if (!rejectedFile) {
        perror("Error opening rejected_orders.dat");
        exit(1);
    }

    // 计算rejected_orders.dat文件中的行数
    while (fgets(line, sizeof(line), rejectedFile)) {
        rejectedCount++;
    }

    fclose(rejectedFile);
    // 打开report.txt文件以追加模式
    reportFilePointer = fopen(reportFile, "a");
    if (!reportFilePointer) {
        perror("Error opening Report.txt");
        exit(1);
    }

    // 写入特定文本内容并在其中替换XX为rejectedCount
    fprintf(reportFilePointer, "There are %d Orders REJECTED. Details are as follows:\n", rejectedCount);
    fprintf(reportFilePointer, "ORDER NUMBER\tPRODUCT NAME\t\tDue Date\t\t\tQUANTITY\n");
    fprintf(reportFilePointer, "===========================================================================\n");
    fclose(reportFilePointer);
} 


// 处理被拒绝的订单
int processRejectedOrders(const char *rejectedOrdersFile, const char *allOrdersFilePath, const char *reportFilePath) {
   // Arrays to store the order numbers and the rejected orders
    char rejectedOrderNumbers[MAX_REJECTED][DATE_LENGTH];
    Rejected_Order3 rejectedArr[MAX_REJECTED];
   int rejectedCount = 0;
   
   
    // Open the file containing the rejected order numbers
    FILE *rejectedFile = fopen(rejectedOrdersFile, "r");
    if (!rejectedFile) {
        perror("Error opening rejected_orders.dat");
        return 1; // 返回错误代码
    }

    // Read the order numbers into the array
    while (rejectedCount < MAX_REJECTED && fscanf(rejectedFile, "%11s", rejectedOrderNumbers[rejectedCount]) == 1) {
        rejectedCount++;
    }

    fclose(rejectedFile);
    int rejectedIndex=0;

    // Open the file containing all orders
    FILE *allOrdersFile = fopen(allOrdersFilePath, "r");
    if (!allOrdersFile) {
        perror("Error opening all_orders.txt");
        return -1;
    }

    // Read through all orders and check if any are rejected
    char line[1024]; // Assuming the line will not exceed 1023 characters
    while (fgets(line, sizeof(line), allOrdersFile)) {
        char orderNumber[MAX_ORDER_NUM];
        char date[DATE_LENGTH];
        char productName[MAX_PRODUCT_NAME];
        int quantity;

        if (sscanf(line, "%s %s %d %s", orderNumber, date, &quantity, productName) == 4) {
            // Check against rejected order numbers
            for (int i = 0; i < rejectedCount; i++) {
                if (strcmp(orderNumber, rejectedOrderNumbers[i]) == 0) {
		    
		     
                    // Found a rejected order, store its details
                    strcpy(rejectedArr[rejectedIndex].orderNumber, orderNumber);
                    strcpy(rejectedArr[rejectedIndex].date, date);
                    rejectedArr[rejectedIndex].quantity = quantity;
                    strcpy(rejectedArr[rejectedIndex].productName, productName);
                    rejectedIndex++;
                    break; // No need to check the rest of the rejected orders
                }
            } 
        } else {
            fprintf(stderr, "Error parsing line: %s", line);
        }
    }
    fclose(allOrdersFile);

    // Open the report file to append the rejected orders
    FILE *reportFile = fopen(reportFilePath, "a");
    if (!reportFile) {
        perror("Error opening report.txt");
        return -1;
    }

    // Write the rejected orders to the report file
    for (int i = 0; i < rejectedIndex; i++) {
	
        fprintf(reportFile, "%s\t\t\t %s\t\t\t %s\t\t\t %d\n",
            rejectedArr[i].orderNumber,
            rejectedArr[i].productName,
	    rejectedArr[i].date,
	    rejectedArr[i].quantity);
    }


    // Write footer to the report file
    fprintf(reportFile, "\n\t\t\t- End -\n");
    fprintf(reportFile, "===========================================================================\n\n\n");

    fclose(reportFile);
    return 0;
}

// const char* file1, const char* file2, const char* file3, const char* rejectionFile
int main() {
    FILE *reportFile;

    // delete the file if it exists
    remove("Report.txt");

    int acceptedOrders = 0; // 你需要根据实际情况来计算接受的订单数量
    
    processOrders("Plant_X.dat");
    processOrders("Plant_Y.dat");
    processOrders("Plant_Z.dat");
	
    qsort(orders3, orderCount3, sizeof(Order3), compareOrders);
    
    // 假设这是你的算法名称和已接受的订单数
    const char *algorithmName = "YourAlgorithmName";
    const char *allOrdersFile = "All_Orders.txt";
    const char *rejectedOrdersFile = "rejected_orders.dat";

    acceptedOrders = countAcceptedOrders(allOrdersFile, rejectedOrdersFile);
    
    // 调用函数写入报告头部
    if (writeReportHeader(algorithmName, acceptedOrders) != 0) {
        fprintf(stderr, "Failed to write report header.\n");
        return 1;
    }


    // TODO: 在这里添加写入具体的订单数据的代码
    writeOrdersToFile("Report.txt");
    appendReportWithRejectedCount("rejected_orders.dat", "Report.txt");
    processRejectedOrders("rejected_orders.dat", "All_Orders.txt", "Report.txt");
	

    writeFinal3("Report.txt");

    return 0;
}

