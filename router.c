#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5

int all_ports[] = {8088, 8089, 8090, 8091};
int PORT;
char all_hosts[100][100] = {"192.168.1.2", "192.168.2.254", "192.168.9.1"};
char neighbors[100][100];
int neighbour_count;
pthread_mutex_t mut;
int periodic_timer = 5;
int entry_expire_thresh = 30;
int entry_garbage_thresh = 30;

int get_rand_val(){
    int lower = 0, upper = 1;
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}


struct routing_table_entry
{
    char destination_port[5];
    int cost;
    char next_hop_port[5];
    int expiration_time;
    int garbage_collection_time;
};

struct routing_table_entry ROUTING_TABLE[10000];
int ROUTING_TABLE_SIZE = 0;

void print_routing_table()
{
    char routing_table[17];
    snprintf(routing_table, sizeof(routing_table), "routing_%i.txt\n", PORT);
    FILE *fptr;
    fptr = fopen(routing_table, "a");

    fputs("\ndest | cost | next | exp time | garbage time\n", fptr);
    printf("\ndest | cost | next | exp time | garbage time\n");

    int i = 0;
    for (i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
        fprintf(fptr, "%s | %d | %s | %d | %d \n", ROUTING_TABLE[i].destination_port, ROUTING_TABLE[i].cost, ROUTING_TABLE[i].next_hop_port, ROUTING_TABLE[i].expiration_time, ROUTING_TABLE[i].garbage_collection_time);
        printf("%s | %d | %s | %d | %d \n", ROUTING_TABLE[i].destination_port, ROUTING_TABLE[i].cost, ROUTING_TABLE[i].next_hop_port, ROUTING_TABLE[i].expiration_time, ROUTING_TABLE[i].garbage_collection_time);
    }
    fprintf(fptr, "\n\n");
    printf("\n\n");
    fclose(fptr);
}

void print_neighbours()
{
    int i;
    for (i = 0; i < neighbour_count; i++)
    {
        printf("neighbour: %s\n", neighbors[i]);
    }
}

void initialization_routing_table()
{
    char config_file[15];
    snprintf(config_file, sizeof(config_file), "config%i.txt\n", PORT);

    printf("config-file name %s\n", config_file);

    FILE *fptr;
    fptr = fopen(config_file, "r");

    char line[50];

    int entry_count = 0;
    neighbour_count = 0;

    while (fgets(line, 50, fptr))
    {
        //printf("config line: %s\n", line);

        char *words[3];
        int i = 0;
        char *token = strtok(line, " ");
        words[i++] = token;
        while (token != NULL)
        {
            //printf(" %d %s\n", i, token); //printing each token
            token = strtok(NULL, " ");
            words[i++] = token;
        }
        strcpy(neighbors[neighbour_count++], words[1]);
        strcpy(ROUTING_TABLE[entry_count].destination_port, words[1]);
        ROUTING_TABLE[entry_count].cost = atoi(words[2]);
        strcpy(ROUTING_TABLE[entry_count].next_hop_port, "$");
        ROUTING_TABLE[entry_count].expiration_time = 0;
        ROUTING_TABLE[entry_count].garbage_collection_time = 0;

        entry_count++;
    }
    ROUTING_TABLE_SIZE = entry_count;
    printf("\nInitial routing tabel:\n");
    print_routing_table();

    fclose(fptr);
    return;
}

void update_neighbour_list(char *sender_port)
{
    int i = 0;
    for (i = 0; i < neighbour_count; i++)
    {
        if (strcmp(sender_port, neighbors[i]) == 0)
        {
            break;
        }
    }
    if (i == neighbour_count)
    {
        strcpy(neighbors[neighbour_count], sender_port);
        neighbour_count++;
    }
    // print_neighbours();
}


int get_sender_cost(char *sender_port)
{
    int to_sender_cost = 1000000;
    int i;
    for (i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
        if (strcmp(ROUTING_TABLE[i].destination_port, sender_port) == 0)
        {
            to_sender_cost = ROUTING_TABLE[i].cost;
            break;
        }
    }
    return to_sender_cost;
}

int update_table(char *sender_port, struct routing_table_entry *incoming_table, int incoming_table_size)
{
    printf("incoming data table: \n");
    // int i;
    for (int i = 0; i < incoming_table_size; i++)
    {
        printf("%s | %d\n", incoming_table[i].destination_port, incoming_table[i].cost);
    }
    int change_made = 0;
    int i;
    pthread_mutex_lock(&mut);

    for (i = 0; i < incoming_table_size; i++)
    {
        if (atoi(incoming_table[i].destination_port) == PORT)
        {
            continue;
        }

        int j;
        for (j = 0; j < ROUTING_TABLE_SIZE; j++)
        {
            // printf("%s and %s comparison result: %d\n",ROUTING_TABLE[j].next_hop_port, sender_port, strcmp(ROUTING_TABLE[j].next_hop_port, sender_port));
        
            //update entry if an existing network cost is less through sender cost
            if ((strcmp(ROUTING_TABLE[j].destination_port, incoming_table[i].destination_port) == 0) &&
                (strcmp(ROUTING_TABLE[j].next_hop_port, sender_port) != 0))
            {
                //get cost of destination i through sender
                int cost_through_sender = get_sender_cost(sender_port) + incoming_table[i].cost;
                if (cost_through_sender < ROUTING_TABLE[j].cost)
                {
                    ROUTING_TABLE[j].cost = cost_through_sender;
                    strcpy(ROUTING_TABLE[j].next_hop_port, sender_port);
                    change_made = 1;
                }
                break;
            }
            //if this entry's next hop is sender then update the cost according to the sender cost
            else if((strcmp(ROUTING_TABLE[j].destination_port, incoming_table[i].destination_port) == 0) &&
                (strcmp(ROUTING_TABLE[j].next_hop_port, sender_port) == 0))
            {
                ROUTING_TABLE[j].cost = get_sender_cost(sender_port) + incoming_table[i].cost;
                break;
            }

        }
        //that means this destination port not found in current table
        //so we should entry
        if (j == ROUTING_TABLE_SIZE)
        {
            strcpy(ROUTING_TABLE[ROUTING_TABLE_SIZE].destination_port, incoming_table[i].destination_port);
            ROUTING_TABLE[ROUTING_TABLE_SIZE].cost = get_sender_cost(sender_port) + incoming_table[i].cost;
            strcpy(ROUTING_TABLE[ROUTING_TABLE_SIZE].next_hop_port, sender_port);
            ROUTING_TABLE_SIZE++;
        }
    }

    //update timers for this sender
    int k;
    for(k=0;k<ROUTING_TABLE_SIZE;k++){
        char * this_entry_next_hop = &ROUTING_TABLE[k].next_hop_port[0];
        char * this_entry_dest = &ROUTING_TABLE[k].destination_port[0];
        // strcpy(this_entry_dest,ROUTING_TABLE[k].destination_port);
        // printf("%s and %s comparison result: %d\n",this_entry_dest, sender_port, strcmp(this_entry_dest, sender_port));
        if((strcmp(this_entry_next_hop, sender_port)==0  || strcmp(this_entry_dest, sender_port)==0) && ROUTING_TABLE[k].cost<16){
            // printf("timers updated for sender port: %s\n",sender_port);
            ROUTING_TABLE[k].expiration_time = 0;
            ROUTING_TABLE[k].garbage_collection_time = 0;
        }
        
    }
    update_neighbour_list(sender_port);

    print_routing_table();

    pthread_mutex_unlock(&mut);

    return change_made;
}

char *create_string_of_routing_table(char * for_port)
{

    char *whole_data_pointer;

    char whole_data[100] = "";

    snprintf(whole_data, sizeof(whole_data), "from %i#", PORT);

    int i;
    for (i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
        //to avoid poisson reverse
        if(strcmp(for_port,&ROUTING_TABLE[i].destination_port[0])==0 || strcmp(for_port,&ROUTING_TABLE[i].next_hop_port[0])==0){
            continue;
        }
        strcat(whole_data, ROUTING_TABLE[i].destination_port);
        strcat(whole_data, "-");
        char coststr[10];
        sprintf(coststr, "%d", ROUTING_TABLE[i].cost);
        strcat(whole_data, coststr);
        strcat(whole_data, "#");
    }
    whole_data_pointer = whole_data;
    return whole_data_pointer;
}


void extract_data_and_update_table(char *data)
{
    char data_arr[1000];

    strcpy(data_arr, data);

    char *line = strtok(data_arr, "#");

    char *string, *found, *sender_port;
    string = strdup(line);
    sender_port = strsep(&string, " ");
    sender_port = strsep(&string, "-");
    printf("sender-port: %s\n", sender_port);

    // update_neighbour_list(sender_port);

    struct routing_table_entry incoming_routing_table[100];

    int incoming_tabl_entry_count = 0;

    while (line != NULL)
    {

        line = strtok(NULL, "#");
        // printf("extracted line: %s\n", line);
        if (line != NULL)
        {

            // char *string,*found;
            string = strdup(line);
            found = strsep(&string, "-");
            strcpy(incoming_routing_table[incoming_tabl_entry_count].destination_port, found);
            found = strsep(&string, "-");

            incoming_routing_table[incoming_tabl_entry_count].cost = atoi(found);
            incoming_tabl_entry_count++;
        }
    }

    update_table(sender_port, incoming_routing_table, incoming_tabl_entry_count);

    return;
}

struct arg_struct
{
    int sock;
    int this_port;
};

struct arg_struct2
{
    struct sockaddr_in this_node;
    int server_fd;
};

struct neighbor
{
    struct sockaddr_in neighbor_node;
    int neighbor_port;
    char data[2000];
};

void *cli(void *arguments)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct neighbor *neighbor_def = arguments;
    int client_fd;
    
    
    if ((client_fd = connect(client_socket, (struct sockaddr *)&neighbor_def->neighbor_node, sizeof(neighbor_def->neighbor_node))) < 0)
    {
        return NULL;
    }

    printf("Connection established from %d to port %d\n", PORT, neighbor_def->neighbor_port);
    char neighbor_port[5];
    sprintf(neighbor_port, "%d", neighbor_def->neighbor_port);

    char *data = create_string_of_routing_table(&neighbor_port[0]);

    // send(client_socket, data, strlen(data), 0);        // send(client_socket, data, strlen(data), 0);
    sendto(client_socket, data, strlen(data), MSG_CONFIRM, (const struct sockaddr *) &neighbor_def->neighbor_node,  sizeof(neighbor_def->neighbor_node));

    printf("data sent from %d to port %d: %s\n\n", PORT, neighbor_def->neighbor_port, data);
    close(client_fd);
        
    
    return NULL;
}

void *sendingThread(void *arguments)
{
    printf("Sending thread running!!!!\n");

    struct arg_struct *args = arguments;
    int sock, this_node_port;
    sock = args->sock;
    this_node_port = args->this_port;

    FILE *fptr;
    
    while (1)
    {

        int i;
        pthread_t client_threads[5];
        int client_thread_count = 0;
        // char *data = create_string_of_routing_table();
        // printf("created data from routing table: %s\n", data);
        for (i = 0; i < neighbour_count; i = i + 1)
        {

            struct sockaddr_in neighbor_node;
            int neighbor_port;
            int client_fd;

            neighbor_port = atoi(neighbors[i]);

            neighbor_node.sin_addr.s_addr = INADDR_ANY;
            neighbor_node.sin_family = AF_INET;
            neighbor_node.sin_port = htons(neighbor_port);

            
            if (inet_pton(AF_INET, "127.0.0.1", &neighbor_node.sin_addr) <= 0)
            {
                printf("\nInvalid address/ Address not supported \n");
                return NULL;
            }
            pthread_t client_thread_id;
            struct neighbor *neighbor_def = malloc(sizeof(struct neighbor));
            neighbor_def->neighbor_node = neighbor_node;
            neighbor_def->neighbor_port = neighbor_port;
            // strcpy(neighbor_def->data, data);
            pthread_create(&client_thread_id, NULL, cli, neighbor_def);
            client_threads[client_thread_count++] = client_thread_id;
        }
        int j;
        for (j = 0; j < client_thread_count; j++)
        {
            pthread_join(client_threads[j], NULL);
        }
        sleep(periodic_timer+get_rand_val());
    }

    return NULL;
}

void *recievingThread(void *arguments)
{

    struct sockaddr_in this_node;
    struct arg_struct2 *args = arguments;
    int server_fd, new_socket;
    this_node = args->this_node;
    server_fd = args->server_fd;
    int valread, addrlen = sizeof(this_node);
    char buffer[1024] = {0};
    struct sockaddr_in cliaddr;

    while (1)
    {

        memset(&cliaddr, 0, sizeof(cliaddr));
        int len = sizeof(cliaddr);
        recvfrom(server_fd, (char *)buffer, 4024,  MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        printf("\nrecieved data: %s \n", buffer);
        extract_data_and_update_table(buffer);
        // print_neighbours();
        close(new_socket);
    }

    return NULL;
}

void remove_garbage_entry(int position)
{
    printf("removing garbage value!!!!\n");
    int i, j;
    int routing_table_updated=0,neighbour_list_updated = 0;
    for (i = 0; i < neighbour_count; i++)
    {

        if (strcmp( ROUTING_TABLE[j].destination_port, neighbors[i])==0 )
        {
            strcpy(neighbors[i], neighbors[i+1]);
            neighbour_list_updated=1;
        }
    }
    if(neighbour_list_updated==1)
        neighbour_count--;
    
    
    for (j = position; j < ROUTING_TABLE_SIZE; j++)
    {
        ROUTING_TABLE[j] = ROUTING_TABLE[j + 1];
        routing_table_updated=1;
    }
    if(routing_table_updated==1)
        ROUTING_TABLE_SIZE--;
}

void *timer(void *arguments)
{
    while (1)
    {
        pthread_mutex_lock(&mut);

        int i;
        for (i = 0; i < ROUTING_TABLE_SIZE; i++)
        {
            if (ROUTING_TABLE[i].garbage_collection_time >= entry_garbage_thresh)
            {
                remove_garbage_entry(i);
                i--; //since left shifted all the element deleting i-th element. So now in i position there is new element.
            }
            else if (ROUTING_TABLE[i].expiration_time >= entry_expire_thresh)
            {
                ROUTING_TABLE[i].cost = 16;
                ROUTING_TABLE[i].garbage_collection_time++;
            }
            else
            {
                ROUTING_TABLE[i].expiration_time++;
            }
        }
        // print_routing_table();
        // print_neighbours();
        pthread_mutex_unlock(&mut);

        sleep(1);
    }
}

int main(int argc, char *argv[])
{
    // sleep(5);

    int id = atoi(argv[1]);
    // char *ip = argv[2];
    int port = atoi(argv[2]);
    PORT = port;

    printf("node-id: %d, port: %d\n", id, port);

    initialization_routing_table();
    char *data = create_string_of_routing_table("0000");
    // printf("data created from table: %s\n", data);
    // extract_data_and_update_table(data);

    char buffer[1024] = {0};
    int new_socket, valread, client_fd;
    int opt = 1;
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    //setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
    struct sockaddr_in node, neighbor_node;
    struct sockaddr_in neighbors[5];
    // node.sin_addr.s_addr = inet_addr(ip);
    node.sin_addr.s_addr = INADDR_ANY;
    node.sin_family = AF_INET;
    node.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&node, sizeof(node)) < 0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("bind successful %d\n", port);
    }

    pthread_t sending_thread_id, recieving_thread_id, timer_thread_id;
    int i;

    struct arg_struct *args = malloc(sizeof(struct arg_struct));
    args->sock = server_fd;
    args->this_port = port;
    pthread_create(&sending_thread_id, NULL, sendingThread, args);

    struct arg_struct2 *args2 = malloc(sizeof(struct arg_struct2));
    args2->this_node = node;
    args2->server_fd = server_fd;
    pthread_create(&recieving_thread_id, NULL, recievingThread, args2);

    pthread_create(&timer_thread_id, NULL, timer, NULL);
    
    pthread_join(sending_thread_id, NULL);
    pthread_join(recieving_thread_id, NULL);
    pthread_join(timer_thread_id, NULL);
}
