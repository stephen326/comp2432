#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUMBER_OF_CHILD 3
#define READY "R" // student can send this signal to parent to state that he finish on receive 
#define END "E" // END game signal
#define CAPACITY 100 // maximum number of orders to be handled
#define PRODUCTIVITY_X 300
#define PRODUCTIVITY_Y 400
#define PRODUCTIVITY_Z 500

// define struct to store date
typedef struct {
    int year;
    int month;
    int day;
    // string
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
int datecmp(Date date1, Date date2) { // if 1 earlier than 2, return -1; if 1 later than 2, return 1; if equal, return 0
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

// Date method: Calculate date sum between start and end (including both sides) (consider 30/31/Feb. and leap year)
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

// define order struct to store order information
typedef struct {
    char orderNo[20];
    char due_str[11]; // Date format is YYYY-MM-DD, including null character \0
    Date due; // Date struct to store due date
    int  qty;
    char product[20];
} Order;

// a null Order
Order nullOrder = {"NN", "NN", {0, 0, 0}, -1, "NN"};

// a tombstone Order (only used for rejected orders)
Order tombstoneOrder = {"XX", "XX", {0, 0, 0}, -2, "XX"};

typedef struct {
    Order orders[CAPACITY]; // array of all orders
    Order priority_1[CAPACITY]; // array of orders with priorities
    Order priority_2[CAPACITY];
    Order priority_3[CAPACITY];
} Todo;

typedef struct {
    int batchNo;
    Order order;
    int batchQty;
} ProdBatch;

// a null ProdBatch
ProdBatch nullBatch = {-1, {"NN", "NN", {0, 0, 0}, -1, "NN"}, -1};

typedef struct {
    Date date;
    ProdBatch batches[CAPACITY];
    int batchCount;
    int batchQty;
    int capacity;
} oneDaySchedule;

// schedule init
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
    for (i = 0; i < CAPACITY; i++) {
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
    if (schedule->batchCount >= CAPACITY) {
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

// define a period struct to store period information
typedef struct {
    Date startDate;
    Date endDate;
    int interval;
} Period;

int findIndex(int arr[], int size, int value) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (arr[i] == value) {
            return i;
        }
    }
    return -1;
}

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
}

void executeMainPLS(char algorithm[], char outputFileName[]) {
    int i = 0, n = 0; // loop variables

    /*
    1.
    before running PLS, we need to read the orders and period from the files
    orders file format (line by line): orderNumber dueDate quantity productName
    example: "P0001 2024-06-10 2000 Product_A" (end with \n)
    period file format (fist line only): startDate endDate
    example: "2024-06-01 2024-06-30"
    */

    Todo todo;
    // padding nullOrder to all orders and priority arrays
    for (i = 0; i < CAPACITY; i++) {
        todo.orders[i] = nullOrder;
        todo.priority_1[i] = nullOrder;
        todo.priority_2[i] = nullOrder;
        todo.priority_3[i] = nullOrder;
    }
    readTodo(&todo);
    // testing to print out orders' No until nullOrder /////////////////////////
    for (i = 0; i < CAPACITY; i++) {
        if (strcmp(todo.orders[i].orderNo, "NN") == 0) {
            break;
        }
        printf("TESTING Order No: %s\n", todo.orders[i].orderNo);
    }
    // read period
    Period period;
    readPeriod("periods.txt", &period);
    // testing to print out period st/ed/in ////////////////////////////////////////////
    printf("TESTING Period: %d-%d-%d to %d-%d-%d, interval: %d\n", period.startDate.year, period.startDate.month, period.startDate.day, period.endDate.year, period.endDate.month, period.endDate.day, period.interval);


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
        // test the timetable print out date padding of each slot
        /*
        for (i = 0; i < 3; i++) {
            for (n = 0; n < period.interval; n++) {
                printf("TESTING Factory %d, Day %d: %d-%d-%d\n", i, n, timetable[i][n].date.year, timetable[i][n].date.month, timetable[i][n].date.day);
            }
        }
        */


    // 3. If Algorithm is FCFS:
    // PR 算法与 FCFS 实质相同，对于PR的实现可以先按照priority排序，然后按照FCFS的方式处理

    // set a array for rejected orders
    Order rejectedOrders[CAPACITY];
    int rejectedCount = 0;
    // pad nullOrder to rejected orders
    for (i = 0; i < CAPACITY; i++) {
        rejectedOrders[i] = nullOrder;
    }

    if (strcmp(algorithm, "FCFS") == 0) {
        printf("Running PLS with FCFS algorithm\n");

        // 先模拟填，如果填不进去（多次切片之后依然过due），尝试填下一个订单。如果填进去了且不超过due，就实际填。
        // 填入时间表按照每天的三个工厂，然后依次往后进行第二天第三天

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
                    // commented due to SIM //ProdBatch batch = sliceOrder2Batch(todo.orders[orderIndex], SIMbatchNo, todo.orders[orderIndex].qty - SIMcurrentOrderScheduledQty);
                    SIMbatchNo++;
                    // commented due to SIM //addProdBatch(&timetable[SIMfactoryInDay][SIMdayIndex], batch); //(MARK: Here we currently do not check if the batches in the same slot are the same product)
                    SIMcurrentOrderScheduledQty = todo.orders[orderIndex].qty;
                    isOrderCanBeFulfilled = 1;
                    break;
                } else {
                    // if not, fill the slot with the remaining cap
                    // commented due to SIM //ProdBatch batch = sliceOrder2Batch(todo.orders[orderIndex], SIMbatchNo, timetable[SIMfactoryInDay][SIMdayIndex].capacity - timetable[SIMfactoryInDay][SIMdayIndex].batchQty);
                    SIMbatchNo++;
                    // commented due to SIM //addProdBatch(&timetable[SIMfactoryInDay][SIMdayIndex], batch); //(MARK: Here we currently do not check if the batches in the same slot are the same product)
                    SIMcurrentOrderScheduledQty += timetable[SIMfactoryInDay][SIMdayIndex].capacity - timetable[SIMfactoryInDay][SIMdayIndex].batchQty;
                }
                // if the slot is filled, move to the next slot
                // if factory index is 2, move to the next day, or move to the next factory
                if (SIMfactoryInDay == 2) {
                    SIMfactoryInDay = 0;
                    SIMdayIndex++;
                } else {
                    SIMfactoryInDay++;
                }
            }

            //print out the simulation result
            printf("Order %s simulation result: %d\n", todo.orders[orderIndex].orderNo, isOrderCanBeFulfilled);


            // if simulation is successful, then actually fill the timetable (same process but plus actual filling)
            if (isOrderCanBeFulfilled == 1) {
                // actual filling process (only consider actual pointers)
                while (1) { // for every slot, try to fill the order
                // if the remaining Qty can be fulfilled in this slot's remaining cap, fill the slot and break (final batch of the order)
                int isFinalBatch = (todo.orders[orderIndex].qty - currentOrderScheduledQty <= timetable[factoryInDay][dayIndex].capacity - timetable[factoryInDay][dayIndex].batchQty);
                if (isFinalBatch) {
                    ProdBatch batch = sliceOrder2Batch(todo.orders[orderIndex], batchNo, todo.orders[orderIndex].qty - currentOrderScheduledQty);
                    batchNo++;
                    addProdBatch(&timetable[factoryInDay][dayIndex], batch); //(MARK: Here we currently do not check if the batches in the same slot are the same product)
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
                    addProdBatch(&timetable[factoryInDay][dayIndex], batch); //(MARK: Here we currently do not check if the batches in the same slot are the same product)
                }
                // if the slot is filled, move to the next slot
                // if factory index is 2, move to the next day, or move to the next factory
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




    } // 结束结果应当为一个填好的timetable，以及一个rejected orders list

    // testing: print out the timetable (only the slot not empty) (also note which order is the batch from and batch qty)
    // order by batch number (Day0 (fac0 -> fac1 -> fac2), Day1 (fac0 -> fac1 -> fac2), ...
    // print out format like a table
    for (i = 0; i < period.interval; i++) {
        printf("Day %d\n", i);
        for (n = 0; n < 3; n++) {
            printf("Factory %d: ", n);
            for (int j = 0; j < CAPACITY; j++) {
                if (timetable[n][i].batches[j].batchNo != -1) {
                    printf("Batch %d: %s %d %s | ", timetable[n][i].batches[j].batchNo, timetable[n][i].batches[j].order.orderNo, timetable[n][i].batches[j].batchQty, timetable[n][i].batches[j].order.product);
                }
            }
            printf("\n");
        }
    }


    // 4. set up pipes and fork child processes

    // variables for pipe and fork
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
        printf("Parent: Sending ready signal to all child\n");
        for (i = 0; i < NUMBER_OF_CHILD; i++) {
            write(p2cPipe[i][1], READY, sizeof(READY));
        }

        // Parent 应当等待所有child完成后，从pipe读取report，将所有的report合并，然后输出到output file
        // 此外，Parent 应当等待所有child完成后，将所有的rejected orders list合并（如有从Child得到的update），然后输出到output file


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

        // Child 应当取用自己的timetable，然后开始生成output report，最后将report发送给parent

        /* 格式如下：

        ---------------------------------------------------------------------\n
        Plant_X (300 per day)\n
        2024-06-01 to 2024-06-30\n
        ---------------------------------------------------------------------\n
            Date   | Product Name |  Order Number |  Quantity  |  Due Date\n
        ---------------------------------------------------------------------\n
        2024-06-01 |  Product_A   |     P0001     |    300     |  2024-06-10\n
        2024-06-02 |  Product_A   |     P0001     |    300     |  2024-06-10\n
        2024-06-03 |  Product_A   |     P0001     |    300     |  2024-06-10\n
        2024-06-04 |  Product_A   |     P0001     |    300     |  2024-06-12\n
        2024-06-05 |  Product_A   |     P0002     |    300     |  2024-06-10\n
        2024-06-06 |  Product_A   |     P0003     |    300     |  2024-06-17\n
        2024-06-07 |  N/A         |     N/A       |    N/A     |  N/A       \n
        2024-06-08 |  Product_A   |     P0007     |    300     |  2024-06-19\n
        ---------------------------------------------------------------------\n
        
        */

        // a string for report
        char report[CAPACITY*80 + 200];
        // add header (child 0 for X, 1 for Y, 2 for Z)
        if (myIndex == 0) {
            sprintf(report, "---------------------------------------------------------------------\nPlant_X (300 per day)\n");
        } else if (myIndex == 1) {
            sprintf(report, "---------------------------------------------------------------------\nPlant_Y (400 per day)\n");
        } else if (myIndex == 2) {
            sprintf(report, "---------------------------------------------------------------------\nPlant_Z (500 per day)\n");
        }

        // add period
        char periodStr[50];
        // get the period string
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
            for (n = 0; n < CAPACITY; n++) {
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

        // print out the report
        //printf("Child %d report:\n%s\n", myIndex, report);

        // only child 0 print out the report
        if (myIndex == 0) {
            printf("TESTING Child %d report:\n%s\n", myIndex, report);
        }

        // write report to file named Plant_X.txt (if already exists, overwrite)
        char fileName[20];
        if (myIndex == 0) {
            strcpy(fileName, "Plant_X.txt");
        } else if (myIndex == 1) {
            strcpy(fileName, "Plant_Y.txt");
        } else if (myIndex == 2) {
            strcpy(fileName, "Plant_Z.txt");
        }
        FILE *file = fopen(fileName, "w");
        if (file == NULL) {
            perror("Error opening file");
            exit(1);
        }
        fprintf(file, "%s", report);
        fclose(file);






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


// functionality has been realized in executeMainPLS
/*
void runPLS(char algorithm[]) {
    // 在这里执行 PLS 算法
    printf("Running PLS with algorithm: %s\n", algorithm);
}
*/

void printREPORT(char outputFile[]) {
    // 在这里打印报告
    printf("Printing report to file: %s\n", outputFile);
}


int main() {
    char input[100];
    char command[20];
    char arg1[20], arg2[20], arg3[20], arg4[20];
    char algorithm[20] = "FCFS"; // for testing only
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