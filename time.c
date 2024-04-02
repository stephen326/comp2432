#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_DATE_LENGTH 11 // "yyyy-mm-dd\0"
#define MAX_ORDERS 100     // 最大订单数量

// 订单结构体
typedef struct {
    char id[6];
    char date_str[MAX_DATE_LENGTH];
    int days_difference;
} Order;

// 函数声明
int get_days_difference(const char *date_str, const char *start_date_str);
void read_period_start_date(char *start_date_str);
void read_orders(Order orders[], int *num_orders, const char *start_date_str);

int main() {
    char start_date_str[MAX_DATE_LENGTH];
    read_period_start_date(start_date_str);

    Order orders[MAX_ORDERS];
    int num_orders = 0;
    read_orders(orders, &num_orders, start_date_str);

    printf("订单规划结果：\n");
    for (int i = 0; i < num_orders; i++) {
        printf("订单 %s 的日期与计划开始的第0天之间相差 %d 天\n", orders[i].id, orders[i].days_difference);
    }

    return EXIT_SUCCESS;
}

// 读取计划开始的第0天的日期
void read_period_start_date(char *start_date_str) {
    FILE *periods_file = fopen("periods.txt", "r");
    if (periods_file == NULL) {
        perror("无法打开 periods.txt 文件");
        exit(EXIT_FAILURE);
    }
    fscanf(periods_file, "%s", start_date_str);
    fclose(periods_file);
}

// 读取订单信息
// 读取订单信息
void read_orders(Order orders[], int *num_orders, const char *start_date_str) {
    FILE *orders_file = fopen("All_Orders.txt", "r");
    if (orders_file == NULL) {
        perror("无法打开 All_Orders.txt 文件");
        exit(EXIT_FAILURE);
    }

    char order_id[6]; // 订单号最长为5位，再加上终止符'\0'
    char order_date_str[MAX_DATE_LENGTH];
    while ((*num_orders < MAX_ORDERS) && fscanf(orders_file, "%s %s", order_id, order_date_str) == 2) {
        // 若订单号为数字，则跳过，因为这应该是订单行的一部分
        if (atoi(order_id) != 0) continue;

        // 计算订单日期与计划开始的第0天之间的差值
        int days_difference = get_days_difference(order_date_str, start_date_str);

        // 存储订单信息
        strcpy(orders[*num_orders].id, order_id);
        strcpy(orders[*num_orders].date_str, order_date_str);
        orders[*num_orders].days_difference = days_difference;
        (*num_orders)++;
    }

    fclose(orders_file);
}


// 计算两个日期字符串之间的天数差值
int get_days_difference(const char *date_str, const char *start_date_str) {
    struct tm order_tm, start_tm;
    memset(&order_tm, 0, sizeof(struct tm));
    memset(&start_tm, 0, sizeof(struct tm));

    sscanf(date_str, "%d-%d-%d", &order_tm.tm_year, &order_tm.tm_mon, &order_tm.tm_mday);
    sscanf(start_date_str, "%d-%d-%d", &start_tm.tm_year, &start_tm.tm_mon, &start_tm.tm_mday);

    order_tm.tm_year -= 1900; // tm_year 表示的年份是自1900年起的年数
    order_tm.tm_mon -= 1;     // tm_mon 表示的月份是从0开始的
    start_tm.tm_year -= 1900;
    start_tm.tm_mon -= 1;

    time_t order_time = mktime(&order_tm);
    time_t start_time = mktime(&start_tm);

    double seconds_difference = difftime(order_time, start_time);
    return (int)(seconds_difference / (60 * 60 * 24));
}
