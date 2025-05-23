#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>


#define NUMBER_OF_CHILD 3
#define READY "R"
#define END "E"
#define CAPACITY 700 // maximum number of orders to be handled expected
#define CAPACITY2 2
#define PRODUCTIVITY_X 300
#define PRODUCTIVITY_Y 400
#define PRODUCTIVITY_Z 500

//#######  BLOCK 1: FUNCTIONS  #######//
//#### SECTION 1: Date & methods ####//
typedef struct {
    int year;
    int month;
    int day;
    char str[11]; // Date format is YYYY-MM-DD, including null character \0
} Date;

// a null date
Date nullDate = {0, 0, 0};

// Date method: str (YYYY-MM-DD) to date
Date strToDate(char str[]) {
    Date date;
    sscanf(str, "%d-%d-%d", &date.year, &date.month, &date.day);
    return date;
}

// Date method: compare dates
int datecmp(Date date1, Date date2) { 
    // if 1 earlier than 2, return -1; if 1 later than 2, return 1; if equal, return 0
    if (date1.year < date2.year) {
        return -1;
    } else if (date1.year > date2.year) {
        return 1;
    } else {
        if (date1.month < date2.month) {
            return -1;
        } else if (date1.month > date2.month) {
            return 1;
        } else {
            if (date1.day < date2.day) {
                return -1;
            } else if (date1.day > date2.day) {
                return 1;
            } else {
                return 0;
            }
        }
    }
}

// Date method: increment date by 1 (consider 30/31/Feb. and leap year)
Date dateInc(Date date) {
    Date newDate = date;
    if (date.day == 31) {
        newDate.day = 1;
        if (date.month == 12) {
            newDate.month = 1;
            newDate.year++;
        } else {
            newDate.month++;
        }
    } else if (date.day == 30 && (date.month == 4 || date.month == 6 || date.month == 9 || date.month == 11)) {
        newDate.day = 1;
        newDate.month++;
    } else if (date.day == 29 && date.month == 2) {
        newDate.day = 1;
        newDate.month++;
    } else if (date.day == 28 && date.month == 2 && (date.year % 4 != 0 || (date.year % 100 == 0 && date.year % 400 != 0))) {
        newDate.day = 1;
        newDate.month++;
    } else {
        newDate.day++;
    }
    return newDate;
}

// Date method: Calculate date sum between start and end (including both sides)
int dateInterval(Date start, Date end) {
    int interval = 0;
    Date date = start;
    while (datecmp(date, end) <= 0) {
        interval++;
        date = dateInc(date);
    }
    return interval;
}

// Date method: date to string  (and save to the corresponding string in the struct)
void dateToStr(Date* date) {
    // (YYYY-MM-DD) e.g.(2010-01-02)
    sprintf(date->str, "%d-%02d-%02d", date->year, date->month, date->day);
}

// define a period struct to store period information
typedef struct {
    Date startDate;
    Date endDate;
    int interval;
} Period;



//#### SECTION 2: Order & methods ####//
typedef struct {
    char orderNo[20];
    char due_str[11]; // Date format is YYYY-MM-DD, including null character \0
    Date due; // Date struct to store due date
    int  qty;
    char product[20];
    float wPriority; // weighted priority
} Order;

// a null Order
Order nullOrder = {"NN", "NN", {0, 0, 0}, -1, "NN", -1};

// a tombstone Order (only used for rejected orders)
Order tombstoneOrder = {"XX", "XX", {0, 0, 0}, -2, "XX", -2};

typedef struct {
    Order orders[CAPACITY]; // array of all orders
    Order priority_1[CAPACITY]; // array of orders with priorities
    Order priority_2[CAPACITY];
    Order priority_3[CAPACITY];
    Order orders_priority[CAPACITY + 10]; // array of orders with priorities
} Todo;

void usePRList(Todo* todo) {
    // use priority list to replace orders list
    int i;
    for (i = 0; i < CAPACITY; i++) {
        todo->orders[i] = todo->orders_priority[i];
    }
}

// a method to calculate weighted priority
void calcWPriority(Order* order, Date startDate) {
    // get amount and period
    int amount = order->qty;
    int period = dateInterval(startDate, order->due);
    float w = 2.1;
    // calculate weighted priority
    float wPriority = (amount * w) / period;
    order->wPriority = wPriority;
    return;
}

void useWP(Todo* todo, Period* period) {
    // calculate weighted priority for all orders
    int i;
    for (i = 0; i < CAPACITY; i++) {
        if (strcmp(todo->orders[i].orderNo, "NN") == 0) {
            break;
        }
        calcWPriority(&todo->orders[i], period->startDate);
    }
    // sort orders by weighted priority
    Order temp;
    for (i = 0; i < CAPACITY; i++) {
        if (strcmp(todo->orders[i].orderNo, "NN") == 0) {
            break;
        }
        int j;
        for (j = i + 1; j < CAPACITY; j++) {
            if (strcmp(todo->orders[j].orderNo, "NN") == 0) {
                break;
            }
            if (todo->orders[i].wPriority < todo->orders[j].wPriority) {
                temp = todo->orders[i];
                todo->orders[i] = todo->orders[j];
                todo->orders[j] = temp;
            }
        }
    }
}

//#### SECTION 3: Batch Schedule & methods ####//
typedef struct {
    int batchNo;
    Order order;
    int batchQty;
} ProdBatch;

// a null ProdBatch
ProdBatch nullBatch = {-1, {"NN", "NN", {0, 0, 0}, -1, "NN", -1}, -1};

typedef struct {
    Date date;
    ProdBatch batches[CAPACITY2];
    int batchCount;
    int batchQty;
    int capacity;
} oneDaySchedule;

// schedule initialization
oneDaySchedule initSchedule(Date date, int factory) { // factory 0, 1, 2 for X, Y, Z
    oneDaySchedule schedule;
    schedule.date = date;
    schedule.batchCount = 0;
    schedule.batchQty = 0;
    if (factory == 0) {
        schedule.capacity = PRODUCTIVITY_X;
    } else if (factory == 1) {
        schedule.capacity = PRODUCTIVITY_Y;
    } else if (factory == 2) {
        schedule.capacity = PRODUCTIVITY_Z;
    } else {
        schedule.capacity = 0;
    }
    // pad nullBatch to all batches
    int i;
    for (i = 0; i < CAPACITY2; i++) {
        schedule.batches[i] = nullBatch;
    }
    return schedule;
}

// add batch to schedule
void addProdBatch(oneDaySchedule* schedule, ProdBatch batch) {
    if (schedule == NULL) {
    perror("Schedule pointer is NULL");
    exit(EXIT_FAILURE);
    }
    if (schedule->batchCount >= CAPACITY2) {
        printf("Error: Reached maximum capacity\n");
        return;
    }
    schedule->batches[schedule->batchCount] = batch;
    schedule->batchCount++;
    schedule->batchQty += batch.batchQty;
}

ProdBatch sliceOrder2Batch(Order order, int batchNo, int batchQty) { // to slice an order into a batch
    ProdBatch batch;
    batch.batchNo = batchNo;
    batch.order = order;
    batch.batchQty = batchQty;
    return batch;
}



//#### SECTION 4: File Reading ####//
void readOrders(char fileName[], Order* orders, int size) {
    FILE *file;
    char line[CAPACITY];
    int orderCount = 0;
    file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s %d %s", orders[orderCount].orderNo, orders[orderCount].due_str, &orders[orderCount].qty, orders[orderCount].product);
        orders[orderCount].due = strToDate(orders[orderCount].due_str);
        orderCount++;
    }
    fclose(file);
}

void readPeriod(char fileName[], Period* period) {
    FILE *file;
    char line[CAPACITY];
    file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    char sd[11], ed[11];
    fscanf(file, "%s %s", sd, ed);
    period->startDate = strToDate(sd);
    period->endDate = strToDate(ed);
    period->interval = dateInterval(period->startDate, period->endDate);
    fclose(file);
}

void readTodo(Todo* todo) {
    readOrders("All_Orders.txt", todo->orders, CAPACITY);
    readOrders("Category_1.txt", todo->priority_1, CAPACITY);
    readOrders("Category_2.txt", todo->priority_2, CAPACITY);
    readOrders("Category_3.txt", todo->priority_3, CAPACITY);
    // add 3 categories to orders_priority follows order: [Cat1] [Cat2] [Cat3]. 非nullOrder的部分首尾相接
    int i;
    int ii; // index for orders_priority
    for (i = 0; i < CAPACITY; i++) {
        if (strcmp(todo->priority_1[i].orderNo, "NN") != 0) {
            todo->orders_priority[ii] = todo->priority_1[i];
            ii++;
        }
    }
    for (i = 0; i < CAPACITY; i++) {
        if (strcmp(todo->priority_2[i].orderNo, "NN") != 0) {
            todo->orders_priority[ii] = todo->priority_2[i];
            ii++;
        }
    }
    for (i = 0; i < CAPACITY; i++) {
        if (strcmp(todo->priority_3[i].orderNo, "NN") != 0) {
            todo->orders_priority[ii] = todo->priority_3[i];
            ii++;
        }
    }
}



//#### SECTION 5: Other functions ####//
int findIndex(int arr[], int size, int value) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (arr[i] == value) {
            return i;
        }
    }
    return -1;
}



//#######  BLOCK 2: MAIN PLS  #######//
void executeMainPLS(char algorithm[]) {
    int i = 0, n = 0; // loop variables
    // if algorithm is not FCFS or PR or WP, raise error
    if (strcmp(algorithm, "FCFS") != 0 && strcmp(algorithm, "PR") != 0 && strcmp(algorithm, "WP") != 0) {
        perror("Invalid algorithm");
        exit(1);
    }

    // 1. before running PLS, read the orders and period from files
    Todo todo;
    // padding nullOrder to all orders and priority arrays
    for (i = 0; i < CAPACITY; i++) {
        todo.orders[i] = nullOrder;
        todo.priority_1[i] = nullOrder;
        todo.priority_2[i] = nullOrder;
        todo.priority_3[i] = nullOrder;
        todo.orders_priority[i] = nullOrder;
    }
    readTodo(&todo);
    printf("\nPLS:  Orders load complete\n");

    // read period
    Period period;
    readPeriod("periods.txt", &period);
    printf("PLS:  Period load complete: From %d-%d-%d to %d-%d-%d, Total %d days\n", period.startDate.year, period.startDate.month, period.startDate.day, period.endDate.year, period.endDate.month, period.endDate.day, period.interval);

    // 2. Construct the timetable for each factory
    oneDaySchedule timetable[3][period.interval]; // 3 factories, each factory has a timetable for each day
        // init every time slot with its corresponding date
        for (i = 0; i < 3; i++) {
            Date day = period.startDate;
            for (n = 0; n < period.interval; n++) {
                timetable[i][n] = initSchedule(day, i);
                day = dateInc(day);
            }
        }

    // 3. Deal with rejected orders
    Order rejectedOrders[CAPACITY];
    int rejectedCount = 0;
    // pad nullOrder to rejected orders
    for (i = 0; i < CAPACITY; i++) {
        rejectedOrders[i] = nullOrder;
    }

    // 4. Start scheduling
    if (strcmp(algorithm, "FCFS") == 0) {
        printf("PLS:  Use FCFS algorithm\n");
    } else if (strcmp(algorithm, "PR") == 0) {
        printf("PLS:  Use PR algorithm\n");
        usePRList(&todo);
    } else if (strcmp(algorithm, "WP") == 0) {
        printf("PLS:  Use WP algorithm\n");
        useWP(&todo, &period);
    } else {
        perror("Invalid algorithm");
        exit(1);
    }
    int dayIndex = 0, factoryInDay = 0, batchNo = 0;
    int SIMdayIndex = 0, SIMfactoryInDay = 0, SIMbatchNo = 0;
    int orderIndex;
    for (orderIndex = 0; orderIndex < CAPACITY; orderIndex++) {
        if (todo.orders[orderIndex].qty == -1) { // if order is nullOrder, break
            break;
        }
        int currentOrderScheduledQty = 0;
        int SIMcurrentOrderScheduledQty = 0;
        SIMdayIndex = dayIndex;
        SIMfactoryInDay = factoryInDay; // reset simulation day and factory to current day and factory

        int isOrderCanBeFulfilled = 0; // flag to check if the order can be fulfilled
        // loop here to simulate the filling process
        while (1) { // for every slot, try to fill the order
            // if unexpected scheduled qty > order qty, raise error
            if (SIMcurrentOrderScheduledQty > todo.orders[orderIndex].qty) {
                perror("unexpected error: scheduled qty > order qty");
                exit(1);
            }
            // if all slots are filled, reject current order
            if (SIMdayIndex >= period.interval) {
                isOrderCanBeFulfilled = 0;
                break;
            }
            // if the date of current slot is later than due date, reject current order
            if (datecmp(timetable[SIMfactoryInDay][SIMdayIndex].date, todo.orders[orderIndex].due) > 0) {
                isOrderCanBeFulfilled = 0;
                break;
            }
            // if the remaining Qty can be fulfilled in this slot's remaining cap, fill the slot and break (final batch of the order)
            int isFinalBatch = (todo.orders[orderIndex].qty - SIMcurrentOrderScheduledQty <= timetable[SIMfactoryInDay][SIMdayIndex].capacity - timetable[SIMfactoryInDay][SIMdayIndex].batchQty);
            if (isFinalBatch) {
                SIMbatchNo++;
                SIMcurrentOrderScheduledQty = todo.orders[orderIndex].qty;
                isOrderCanBeFulfilled = 1;
                break;
            } else {
                // if not, fill the slot with the remaining cap
                SIMbatchNo++;
                SIMcurrentOrderScheduledQty += timetable[SIMfactoryInDay][SIMdayIndex].capacity - timetable[SIMfactoryInDay][SIMdayIndex].batchQty;
            }
            // if factory index is 2, move to the next day, or move to the next factory
            if (SIMfactoryInDay == 2) {
                SIMfactoryInDay = 0;
                SIMdayIndex++;
            } else {
                SIMfactoryInDay++;
            }
        }

        // if simulation is successful, then actually fill the timetable (same process but plus actual filling)
        if (isOrderCanBeFulfilled == 1) {
            // actual filling process (only consider actual pointers)
            while (1) { // for every slot, try to fill the order
                // if the remaining Qty can be fulfilled in this slot's remaining cap, fill the slot and break (final batch of the order)
                int isFinalBatch = (todo.orders[orderIndex].qty - currentOrderScheduledQty <= timetable[factoryInDay][dayIndex].capacity - timetable[factoryInDay][dayIndex].batchQty);
                if (isFinalBatch) {
                    ProdBatch batch = sliceOrder2Batch(todo.orders[orderIndex], batchNo, todo.orders[orderIndex].qty - currentOrderScheduledQty);
                    batchNo++;
                    addProdBatch(&timetable[factoryInDay][dayIndex], batch);
                    currentOrderScheduledQty = todo.orders[orderIndex].qty;
                    if (factoryInDay == 2) {
                        factoryInDay = 0;
                        dayIndex++;
                    } else {
                        factoryInDay++;
                    }
                    break;
                } else {
                    // if not, fill the slot with the remaining cap
                    ProdBatch batch = sliceOrder2Batch(todo.orders[orderIndex], batchNo, timetable[factoryInDay][dayIndex].capacity - timetable[factoryInDay][dayIndex].batchQty);
                    batchNo++;
                    currentOrderScheduledQty += timetable[factoryInDay][dayIndex].capacity - timetable[factoryInDay][dayIndex].batchQty;
                    addProdBatch(&timetable[factoryInDay][dayIndex], batch);
                }
                if (factoryInDay == 2) {
                    factoryInDay = 0;
                    dayIndex++;
                } else {
                    factoryInDay++;
                }
            }
        } else {
            // if not, add the order to rejected list
            rejectedOrders[rejectedCount] = todo.orders[orderIndex];
            rejectedCount++;
        }
    }
    // Here, we expect the timetable is completed

    // 5. set up pipes and fork child processes
    int   childPids[NUMBER_OF_CHILD];
    int   p2cPipe[NUMBER_OF_CHILD][2]; // pipe sent from parent to child
    int   c2pPipe[NUMBER_OF_CHILD][2]; // pipe sent from child to parent
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
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            write(p2cPipe[i][1], READY, sizeof(READY));
        }
        char report[3*(CAPACITY*80 + 200)];
        strcpy(report, "");
        // loop to read from all child, and 1. wait for ready signal, and then 2. read report to merge
        char largeBuf[(CAPACITY*80 + 200)+200];
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            while (1) {
                n = read(c2pPipe[i][0], buf, 80);
                if (n > 0 && strcmp(buf, READY) == 0) {
                    break;
                }
            }
            // read report from child
            n = read(c2pPipe[i][0], largeBuf, (CAPACITY*80 + 200) + 200);
            if (n > 0) {
                strcat(report, largeBuf);
            }
        }
        // write report to output file, if exists, overwrite
        char outputFileNameSchedule[20] = "PLS_Schedule_G06.txt";
        // delete the file if exists
        remove(outputFileNameSchedule);
        FILE *file = fopen(outputFileNameSchedule, "w");
        if (file == NULL) {
            perror("Error opening file");
            exit(1);
        }
        fprintf(file, "%s", report);
        printf("PLS:  Overall schedule report written to %s\n", outputFileNameSchedule);
        fclose(file);

        // output rejected orders files
        FILE *file2 = fopen("rejected_orders.dat", "w");
        file2 = fopen("rejected_orders.dat", "w");
        if (file2 == NULL) {
            perror("Error opening file");
            exit(1);
        }
        for (i = 0; i < CAPACITY; i++) {
            if (strcmp(rejectedOrders[i].orderNo, "NN") == 0) {
                break;
            }
            fprintf(file2, "%s\n", rejectedOrders[i].orderNo);
        }
        fclose(file2);

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
        // return to the main function
        return;

        // Here, PLS Main is done, go to the next block
        
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
        // NOTE: child can read from p2cPipe[myIndex][0] and write to c2pPipe[myIndex][1]

        ///////////////// CHILD LOGIC START //////////////////

        // wait until parent to send ready signal
        while (1) {
            n = read(p2cPipe[myIndex][0], buf, 80);
            if (n > 0 && strcmp(buf, READY) == 0) {
                break;
            }
        }
        char report[CAPACITY*80 + 200];
        if (myIndex == 0) {
            sprintf(report, "---------------------------------------------------------------------\nPlant_X (300 per day)\n");
        } else if (myIndex == 1) {
            sprintf(report, "---------------------------------------------------------------------\nPlant_Y (400 per day)\n");
        } else if (myIndex == 2) {
            sprintf(report, "---------------------------------------------------------------------\nPlant_Z (500 per day)\n");
        }
        // add period
        char periodStr[50];
        dateToStr(&period.startDate);
        dateToStr(&period.endDate);
        sprintf(periodStr, "%s to %s\n", period.startDate.str, period.endDate.str);
        strcat(report, periodStr);
        // add table header
        strcat(report, "---------------------------------------------------------------------\n");
        strcat(report, "    Date   | Product Name |  Order Number |  Quantity  |  Due Date\n");
        strcat(report, "---------------------------------------------------------------------\n");
        // add table content (if one day has multiple batches, print multiple lines. but if no batch, print N/A)
        for (i = 0; i < period.interval; i++) {
            // get the date
            char dateStr[50];
            dateToStr(&timetable[myIndex][i].date);
            sprintf(dateStr, "%s | ", timetable[myIndex][i].date.str);
            strcat(report, dateStr);
            // get the batches (to maintain the format, we need to pad spaces if the string is shorter)
            for (n = 0; n < CAPACITY2; n++) {
                if (timetable[myIndex][i].batches[n].batchNo != -1) {
                    char batchStr[100];
                    // qty string (total 8 positions, first fill the qty num, and then pad to 8)
                    char qtyStr[10];
                    sprintf(qtyStr, "%d", timetable[myIndex][i].batches[n].batchQty);
                    int qtyStrLen = strlen(qtyStr);
                    // pad spaces to qtyStr
                    for (int j = 0; j < 8 - qtyStrLen; j++) {
                        strcat(qtyStr, " ");
                    }
                    // add the batch string
                    sprintf(batchStr, " %s   |     %s     |    %s|  %s\n", timetable[myIndex][i].batches[n].order.product, timetable[myIndex][i].batches[n].order.orderNo, qtyStr, timetable[myIndex][i].batches[n].order.due_str);
                    strcat(report, batchStr);
                }
            }
            if (timetable[myIndex][i].batchCount == 0) {
                strcat(report, " N/A         |     N/A       |    N/A     |  N/A       \n");
            }
        }
        // add end line
        strcat(report, "---------------------------------------------------------------------\n");

        // write report to file named Plant_N.txt (if already exists, overwrite)
        char fileName[20];
        if (myIndex == 0) {
            strcpy(fileName, "Plant_X.dat");
        } else if (myIndex == 1) {
            strcpy(fileName, "Plant_Y.dat");
        } else if (myIndex == 2) {
            strcpy(fileName, "Plant_Z.dat");
        }
        // delete the file if exists
        remove(fileName);
        FILE *file = fopen(fileName, "w");
        if (file == NULL) {
            perror("Error opening file");
            exit(1);
        }
        fprintf(file, "%s", report);
        fclose(file);

        // char for factory letter(X/Y/Z)
        char factoryLetter;
        if (myIndex == 0) {
            factoryLetter = 'X';
        } else if (myIndex == 1) {
            factoryLetter = 'Y';
        } else if (myIndex == 2) {
            factoryLetter = 'Z';
        }
        printf("PLS:  [Child %d - %d] Schedule for factory %c OK\n", myIndex, getpid(), factoryLetter);

        // write ready signal to parent
        write(c2pPipe[myIndex][1], READY, sizeof(READY));
        // write report to parent
        char reportMsg[CAPACITY*80 + 200];
        sprintf(reportMsg, "\n-----------------------------------------------------------------------------\nReport from Child %d\n", myIndex);
        strcat(reportMsg, report);
        write(c2pPipe[myIndex][1], reportMsg, sizeof(reportMsg));

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



//#######  BLOCK 3: Analysis  #######//
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
int writeReportHeader(const char *algorithmName, int acceptedOrders, char outputFileName[]) {
    FILE *reportFile;

    // 打开文件用于写入
    reportFile = fopen(outputFileName, "w");
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
int analysis(char algorithm[], char outputFileName[]) {
    FILE *reportFile;

    // delete the file if it exists
    remove(outputFileName);

    int acceptedOrders = 0; // 你需要根据实际情况来计算接受的订单数量
    processOrders("Plant_X.dat");
    processOrders("Plant_Y.dat");
    processOrders("Plant_Z.dat");
    qsort(orders3, orderCount3, sizeof(Order3), compareOrders);
    
    // 假设这是你的算法名称和已接受的订单数
    const char *algorithmName = algorithm;
    const char *allOrdersFile = "All_Orders.txt";
    const char *rejectedOrdersFile = "rejected_orders.dat";
    acceptedOrders = countAcceptedOrders(allOrdersFile, rejectedOrdersFile);
    
    // 调用函数写入报告头部
    if (writeReportHeader(algorithmName, acceptedOrders, outputFileName) != 0) {
        fprintf(stderr, "Failed to write report header.\n");
        return 1;
    }

    writeOrdersToFile(outputFileName);
    appendReportWithRejectedCount("rejected_orders.dat", outputFileName);
    processRejectedOrders("rejected_orders.dat", "All_Orders.txt", outputFileName);
    writeFinal3(outputFileName);
    printf("PLS:  Analysis report written to %s\n\n", outputFileName);

    // reset orderCount3
    orderCount3 = 0;
    // clear the orders3 array
    memset(orders3, 0, sizeof(orders3));
    // reset total_days3, total_quantity3
    total_days3 = 0;
    total_quantity3 = 0;
    total_days3_X = 0;
    total_quantity3_X = 0;
    total_days3_Y = 0;
    total_quantity3_Y = 0;
    total_days3_Z = 0;
    total_quantity3_Z = 0;

    return 0;
}



//#######  BLOCK 4: Interface  #######//
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
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        return 0; // 文件不存在，不存在重复
    }
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        // 提取每行的订单号部分（前五个字符）
        char existingOrderNumber[6];
        strncpy(existingOrderNumber, line, 5);
        existingOrderNumber[5] = '\0';
        // 比较订单号是否相同
        if (strcmp(existingOrderNumber, orderNumber) == 0) {
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

    // 检查分类文件中是否存在相同的订单号
    if (checkDuplicate("All_Orders.txt", orderNumber)) {
    printf("Order with the same order number already exists in All_Orders.txt\n");
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

// // 添加批量订单
// void addBatch(char batchFile[]) {
//     FILE *file = fopen(batchFile, "r");
//     if (file == NULL) {
//         printf("Error opening file\n");
//         return;
//     }
//     char notuse[20], orderNumber[20], dueDate[11], productName[20];
//     int quantity;
//     // 逐行读取批量文件
//     while (fscanf(file, "%s %s %s %d %s", notuse, orderNumber, dueDate, &quantity, productName) != EOF) {
//         printf("TESTING: READ ORDER: %s %s %d %s\n", orderNumber, dueDate, quantity, productName);
//         addOrder(orderNumber, dueDate, quantity, productName);
//     }
//     fclose(file);
// }

void addBatch(char batchFile[]) {
    FILE *file = fopen(batchFile, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    char notuse[20], orderNumber[20], dueDate[11], productName[20];
    char quantityStr[20];
    int quantity;
    // 逐行读取批量文件
    while (fscanf(file, "%s %s %s %s %s", notuse, orderNumber, dueDate, quantityStr, productName) == 5) {
        // 检查订单号格式
        if (strlen(orderNumber) != 5 || orderNumber[0] != 'P' || !isdigit(orderNumber[1]) || !isdigit(orderNumber[2]) ||
            !isdigit(orderNumber[3]) || !isdigit(orderNumber[4])) {
            printf("Invalid order number format: %s\n", orderNumber);
            continue;
        }
        // 检查到期日期格式
        if (strlen(dueDate) != 10 || dueDate[4] != '-' || dueDate[7] != '-' ||
            !isdigit(dueDate[0]) || !isdigit(dueDate[1]) || !isdigit(dueDate[2]) || !isdigit(dueDate[3]) ||
            !isdigit(dueDate[5]) || !isdigit(dueDate[6]) || !isdigit(dueDate[8]) || !isdigit(dueDate[9])) {
            printf("Invalid due date format: %s\n", dueDate);
            continue;
        }
        // 检查数量是否为正整数
        int i;
        for (i = 0; quantityStr[i] != '\0'; i++) {
            if (!isdigit(quantityStr[i])) {
                printf("Invalid quantity format: %s\n", quantityStr);
                break;
            }
        }
        if (quantityStr[i] != '\0') {
            continue; // 跳过当前行
        }
        quantity = atoi(quantityStr); // 将数量字符串转换为整数
        if (quantity <= 0) {
            printf("Invalid quantity: %d\n", quantity);
            continue;
        }
        printf("TESTING: READ ORDER: %s %s %d %s\n", orderNumber, dueDate, quantity, productName);
        addOrder(orderNumber, dueDate, quantity, productName);
    }
    fclose(file);
}


void printREPORT(char outputFile[]) {
    // 在这里打印报告
    printf("Printing report to file: %s\n", outputFile);
}

int main() {
    char input[100];
    char command[20];
    char arg1[20], arg2[20], arg3[20], arg4[20];
    char algorithm[20] = "FCFS"; // default algorithm
    char outputFileName[20] = "Report.txt"; // default output file name
    char batchFile[20];

    printf("~~WELCOME TO PLS~~\n");

    while (1) {
        printf("Please enter:\n> ");
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%s", command);

        if (strcmp(command, "addPERIOD") == 0) {
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
        else if (strcmp(command, "runPLS") == 0) {
            sscanf(input, "%*s %s | printREPORT > %s", algorithm, outputFileName);
            // check if the algorithm is valid
            if (strcmp(algorithm, "FCFS") != 0 && strcmp(algorithm, "PR") != 0 && strcmp(algorithm, "WP") != 0) {
                printf("Invalid algorithm\n");
                continue;
            }
            executeMainPLS(algorithm);
            analysis(algorithm,outputFileName);
        } else if (strcmp(command, "exitPLS") == 0) {
            printf("Bye-bye!\n");
            exit(0);
        } else {
            printf("Invalid command\n");
        }
    }

    return 0;
}