#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5

int all_ports[] = {8088, 8089, 8090};
int PORT;

// struct routing_table_entry{
//     char destination[50];
//     int cost;
//     char next_hop[50];
// }

struct routing_table_entry
{
    char destination_port[10];
    int cost;
    char next_hop_port[10];
};

struct routing_table_entry ROUTING_TABLE[10000];
int ROUTING_TABLE_SIZE;


void initialization_routing_table()
{
    char config_file[15];
    snprintf(config_file, sizeof(config_file), "config%i.txt\n", PORT);

    printf("config-file name %s\n", config_file);

    FILE *fptr;
    fptr = fopen(config_file, "r");

    char line[50];

    int entry_count = 0;

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
        
        strcpy(ROUTING_TABLE[entry_count].destination_port,words[1]);
        ROUTING_TABLE[entry_count].cost = atoi(words[2]);
        strcpy(ROUTING_TABLE[entry_count].next_hop_port, "$");

        entry_count++;
    }
    ROUTING_TABLE_SIZE = entry_count;
    printf("\nInitial routing tabel:\n");
    int i;
    for (i = 0; i < entry_count; i++)
    {
        printf("%s | %d | %s \n", ROUTING_TABLE[i].destination_port, ROUTING_TABLE[i].cost, ROUTING_TABLE[i].next_hop_port);        
    }
    fclose(fptr);
    return;
}


char * create_string_of_routing_table(){
    
    char *whole_data_pointer;
    
    char whole_data[100]="";

    
    snprintf(whole_data, sizeof(whole_data), "from %i#", PORT);
    
    int i; 
    for(i=0;i<ROUTING_TABLE_SIZE;i++){
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

void extract_data(char * data){
    char data_arr[1000];

    strcpy(data_arr,data);

    char *line = strtok(data_arr, "#");

    struct routing_table_entry incoming_routing_table[100];

    int incoming_tabl_entry_count = 0;
    
    while (line != NULL)
    {
        
        line = strtok(NULL, "#");
        printf("extracted line: %s\n", line);
        if(line!=NULL){
            strcpy(incoming_routing_table[incoming_tabl_entry_count].destination_port,strtok(line, "-"));
            incoming_routing_table[incoming_tabl_entry_count].cost = atoi(strtok(NULL, "-")) ; 
        } 
        printf("lalalalall");
        
    }
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
};

void *cli(void *arguments)
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct neighbor *neighbor_def = arguments;
    int client_fd;

    while (1)
    {

        if ((client_fd = connect(client_socket, (struct sockaddr *)&neighbor_def->neighbor_node, sizeof(neighbor_def->neighbor_node))) < 0)
        {
            continue;
        }

        printf("Connection established from %d to port %d\n", PORT, neighbor_def->neighbor_port);

        char data[1024];
        snprintf(data, sizeof(data), "Hellooooooooo from %i\n", PORT);
        send(client_socket, data, strlen(data), 0);
        printf("data sent from %d to port %d\n\n", PORT, neighbor_def->neighbor_port);
        close(client_fd);
        break;
    }
    return NULL;
}

void *sendingThread(void *arguments)
{

    struct arg_struct *args = arguments;
    int sock, this_node_port;
    sock = args->sock;
    this_node_port = args->this_port;

    FILE *fptr;
    char file_name[32];
    snprintf(file_name, sizeof(char) * 32, "sending-thread-%i.txt", this_node_port);
    fptr = fopen(file_name, "w");
    fprintf(fptr, "In sending thread!!!!");
    fprintf(fptr, "This node port %d\n", this_node_port);
    printf("This node port %d\n", this_node_port);

    int new_server_fd = socket(AF_INET, SOCK_STREAM, 0);

    while (1)
    {

        int i;
        pthread_t client_threads[5];
        int client_thread_count = 0;
        for (i = 0; i < 3; i = i + 1)
        {

            if (all_ports[i] == this_node_port)
            {
                continue;
            }

            struct sockaddr_in neighbor_node;
            int neighbor_port;
            int client_fd;

            neighbor_port = all_ports[i];
            neighbor_node.sin_addr.s_addr = INADDR_ANY;
            neighbor_node.sin_family = AF_INET;
            neighbor_node.sin_port = htons(neighbor_port);

            printf("\nport %d is trying to connect to port %d\n", this_node_port, neighbor_port);

            if (inet_pton(AF_INET, "127.0.0.1", &neighbor_node.sin_addr) <= 0)
            {
                printf("\nInvalid address/ Address not supported \n");
                return NULL;
            }
            pthread_t client_thread_id;
            struct neighbor *neighbor_def = malloc(sizeof(struct neighbor));
            neighbor_def->neighbor_node = neighbor_node;
            neighbor_def->neighbor_port = neighbor_port;

            pthread_create(&client_thread_id, NULL, cli, neighbor_def);
            client_threads[client_thread_count++] = client_thread_id;
        }
        int j;
        for (j = 0; j < client_thread_count; j++)
        {
            pthread_join(client_threads[j], NULL);
        }
        sleep(5);
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

    while (1)
    {

        if (listen(server_fd, 3) < 0)
        {
            perror("listen");
            continue;
        }
        if ((new_socket = accept(server_fd, (struct sockaddr *)&this_node,
                                 (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }
        printf("accepted\n");
        valread = read(new_socket, buffer, 1024);
        printf("port: %d %s \n", this_node.sin_port, buffer);
        close(new_socket);
    }

    return NULL;
}

int main(int argc, char *argv[])
{

    int all_ports[] = {8088, 8089, 8090};

    int id = atoi(argv[1]);
    int port = atoi(argv[2]);
    PORT = port;

    printf("node-id: %d, port: %d\n", id, port);

    initialization_routing_table();
    char *data = create_string_of_routing_table();
    extract_data(data);
    printf("%s", data);

    char buffer[1024] = {0};
    int new_socket, valread, client_fd;
    int opt = 1;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
    struct sockaddr_in node, neighbor_node;
    struct sockaddr_in neighbors[5];

    node.sin_addr.s_addr = INADDR_ANY;
    node.sin_family = AF_INET;
    node.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&node, sizeof(node)) < 0)
    {
        perror("bind failed\n");
        //exit(EXIT_FAILURE);
    }
    else
    {
        printf("bind successful %d\n", port);
    }

    pthread_t sending_thread_id, recieving_thread_id;
    int i;

    struct arg_struct *args = malloc(sizeof(struct arg_struct));
    args->sock = server_fd;
    args->this_port = port;
    //pthread_create(&sending_thread_id, NULL, sendingThread, args);

    struct arg_struct2 *args2 = malloc(sizeof(struct arg_struct2));
    args2->this_node = node;
    args2->server_fd = server_fd;
    //pthread_create(&recieving_thread_id, NULL, recievingThread, args2);

    //pthread_join(sending_thread_id, NULL);
    //pthread_join(recieving_thread_id, NULL);
}
