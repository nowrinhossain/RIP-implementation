#include<stdio.h>
#include<string.h>
#include<stdlib.h>
struct send_data_row{
    char destination[20];
    char cost[20];
};

void create_sending_data(struct send_data_row *list_of_data, int length){


    char data[100] ="";

    char buffer[100];
    //printf("%s",sprintf(buffer,100, "%d", 1));
    //itoa(list_of_data[0].destination);
    //snprintf(data, 50, row.destination, row.cost);
    strcat(data, list_of_data[0].destination);
    printf("%s",data);
}

int main(){
    struct send_data_row data[100];

    strcpy(data[0].destination, "12");
    strcpy(data[0].cost, "1");

    create_sending_data(data, 1);
}
