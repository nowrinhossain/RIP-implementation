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

void read_file(){
    char config_file[15];
    snprintf(config_file, sizeof(config_file), "config%i.txt\n", 8088);

    //printf("config-file name %s", config_file);


    FILE *fptr;
    fptr = fopen(config_file, "r");
    printf("File set for read mode");

    char line[50];
    while(fgets(line, 50, fptr)){
        printf("%s", line);
    }
    fclose(fptr);
}

int main(){
    struct send_data_row data[100];

    strcpy(data[0].destination, "12");
    strcpy(data[0].cost, "1");

    //create_sending_data(data, 1);

    read_file();
}
