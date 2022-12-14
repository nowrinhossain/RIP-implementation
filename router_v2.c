#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "gen_funcs.h"

int PORT;
char neighbors[100][100];
int neighbour_count;
pthread_mutex_t mut;
int periodic_timer = 5;
int entry_expire_thresh = 30;
int entry_garbage_thresh = 30;

//RIPV1 packet fields
char * command = "00000010";
char * version = "00000001";
char * reserved = "0000000000000000";
char * family = "0000000000000000";
char * all_zeros = "0000000000000000";
char * zeros32 = "00000000000000000000000000000000";



struct router
{
    int id;
    char ip[20];
    int port;
    int sending_port;
};



struct routing_table_entry_v2
{
    char destination_net[20];
    int cost;
    char next_hop_ip[20];
    int expiration_time;
    int garbage_collection_time;
};

struct router *ROUTER;
struct routing_table_entry_v2 ROUTING_TABLE_V2[10000];
int ROUTING_TABLE_SIZE = 0;
struct router ROUTERS_CONF[1000];
int ROUTERS_COUNT;

struct sending_thread_arg
{
    int sock;
    int this_port;
};

struct recieving_thread_arg
{
    struct sockaddr_in this_node;
    int server_fd;
};

struct neighbor
{
    struct sockaddr_in neighbor_node;
    int neighbor_port;
    char neighbor_ip[20];
    char data[2000];
};


void read_config_file(int router_id){
    FILE *fptr;
    fptr = fopen("config.txt", "r");

    ROUTER = malloc(sizeof(struct router));

    char line[50];

    int entry_count = 0;
    ROUTERS_COUNT = 0;

    while (fgets(line, 50, fptr))
    {
        char *token = strtok(line, " ");
        int id = atoi(token);
        if(id!=router_id){
            char *words[4];
            int i=0;
            while (token != NULL)
            {
                words[i++] = token;   
                token = strtok(NULL, " \n");
            }
            ROUTERS_CONF[entry_count].id = id;
            strcpy(ROUTERS_CONF[entry_count].ip, words[1]);
            ROUTERS_CONF[entry_count].port = atoi(words[2]);
            ROUTERS_CONF[entry_count].sending_port = atoi(words[3]);
            entry_count++;
        }else{
            char *words[4];
            int i=0;
            
            while (token != NULL)
            {
                words[i++]=token;
                //printf("token: %s\n", token);
                token = strtok(NULL, " \n");
                  
            }
            
            ROUTER->id = id;
            strcpy(ROUTER->ip, words[1]);
            ROUTER->port = atoi(words[2]);
            ROUTER->sending_port = atoi(words[3]);

        }
        
    }
    ROUTERS_COUNT = entry_count;
    printf("From config file: id-%d ip-%s port-%d\n",ROUTER->id, ROUTER->ip, ROUTER->port);
}

int get_port_by_ip(char * ip){
    
    for(int i=0;i<ROUTERS_COUNT; i++){
        // printf("ip-%s and ip-%s comparison %d\n",ip, ROUTERS_CONF[i].ip,strcmp(ROUTERS_CONF[i].ip, ip));
        if(strcmp( ROUTERS_CONF[i].ip, ip)==0){
            return ROUTERS_CONF[i].port;
        }
    }
    printf("No corresponsind port found for ip %s\n", ip);
    return 0;
}

char * get_ip_by_sender_port(int sender_port){
    char * ip_ptr;
    for(int i=0;i<ROUTERS_COUNT; i++){
        // printf("ip-%s and ip-%s comparison %d\n",ip, ROUTERS_CONF[i].ip,strcmp(ROUTERS_CONF[i].ip, ip));
        if(ROUTERS_CONF[i].sending_port == sender_port){
            ip_ptr = ROUTERS_CONF[i].ip;
            return ip_ptr;
        }
    }
}

char * get_net_addr_from_ip(char * ip, int net_prefix_len){
    char net_addr[40]="";
    char *words[5];
    int i = 0;
    char *token = strtok(ip, ".");
    words[i++] = token;
    while (token != NULL)
    {
        printf("%d %s\n", i, token); //printing each token
        token = strtok(NULL, ".");
        words[i++] = token;
    }
    // strcpy(net_add_ptr,net_addr);
    strcat(net_addr,words[0]);
    strcat(net_addr,".");
    strcat(net_addr,words[1]);
    strcat(net_addr,".");
    strcat(net_addr,words[2]);
    strcat(net_addr,".");
    strcat(net_addr,"0");
    
    char *net_add_ptr = net_addr;
    
    printf("id------%s\n",net_add_ptr);
    
    return net_add_ptr;
}

void print_routing_table_v2()
{
    char routing_table[17];
    snprintf(routing_table, sizeof(routing_table), "routing_%i.txt\n", PORT);
    FILE *fptr;
    fptr = fopen(routing_table, "a");

    fputs("\n    dest     |  cost  |    next     | exp time | garbage time\n", fptr);
    printf("\n   dest     |  cost  |    next     | exp time | garbage time\n");

    int i = 0;
    for (i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
        fprintf(fptr, "%s |   %d    |      %s      |  %d  |  %d \n", ROUTING_TABLE_V2[i].destination_net, ROUTING_TABLE_V2[i].cost, ROUTING_TABLE_V2[i].next_hop_ip, ROUTING_TABLE_V2[i].expiration_time, ROUTING_TABLE_V2[i].garbage_collection_time);
        printf("%s |   %d    |  %s  |  %d  |  %d \n", ROUTING_TABLE_V2[i].destination_net, ROUTING_TABLE_V2[i].cost, ROUTING_TABLE_V2[i].next_hop_ip, ROUTING_TABLE_V2[i].expiration_time, ROUTING_TABLE_V2[i].garbage_collection_time);
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

void initial_neighbors(){
    char config_file[20];
    snprintf(config_file, sizeof(config_file), "config_router_%i.txt\n", ROUTER->id);

    printf("config file name %s\n", config_file);

    FILE *fptr;
    fptr = fopen(config_file, "r");
    neighbour_count = 0;

    int entry_count = 0;
    char line[50];
    while (fgets(line, 50, fptr))
    {
        // printf("config line: %s\n", line);

        char *words[3];
        int i = 0;
        char *token = strtok(line, " ");
        while (token != NULL)
        {
            words[i++] = token;
            // printf("%d %s\n", i, token); //printing each token
            token = strtok(NULL, " \n");
        }
        strcpy(neighbors[neighbour_count++], words[2]);
        
        entry_count++;
    }
    print_neighbours();

}

void initialization_routing_table_v2()
{
    char connected_net[20];
    snprintf(connected_net, sizeof(connected_net), "connected_net_%i.txt\n", ROUTER->id);
    printf("connected-ent file name %s\n", connected_net);

    FILE *fptr;
    fptr = fopen(connected_net, "r");

    char line[50];

    int entry_count = 0;
    
    while (fgets(line, 50, fptr))
    {
        // printf("config line: %s\n", line);

        char *words[3];
        int i = 0;
        char *token = strtok(line, " ");
        words[i++] = token;
        while (token != NULL)
        {
            // printf("%d %s\n", i, token); //printing each token
            token = strtok(NULL, " \n");
            words[i++] = token;
        }
        
        strcpy(ROUTING_TABLE_V2[entry_count].destination_net, words[1]);
        ROUTING_TABLE_V2[entry_count].cost = 1;
        strcpy(ROUTING_TABLE_V2[entry_count].next_hop_ip, "$");
        ROUTING_TABLE_V2[entry_count].expiration_time = 0;
        ROUTING_TABLE_V2[entry_count].garbage_collection_time = 0;

        entry_count++;
    }
    
    ROUTING_TABLE_SIZE = entry_count;
    printf("\nInitial routing tabel:\n");
    print_routing_table_v2();
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



int get_sender_cost_v2(char *sender)
{
    int to_sender_cost = 16;
    int i;
    for (i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
        if (strcmp(ROUTING_TABLE_V2[i].destination_net, sender) == 0)
        {
            to_sender_cost = ROUTING_TABLE_V2[i].cost;
            break;
        }
    }
    return to_sender_cost;
}




void update_table_v2(char *sender, struct routing_table_entry_v2 *incoming_table, int incoming_table_size)
{
    
    int i;
    pthread_mutex_lock(&mut);

    for (i = 0; i < incoming_table_size; i++)
    {
        if (strcmp(incoming_table[i].next_hop_ip, ROUTER->ip)==0)
        {
            continue;
        }

        int j;
        for (j = 0; j < ROUTING_TABLE_SIZE; j++)
        {
            // printf("%s and %s comparison result: %d\n",ROUTING_TABLE_V2[j].next_hop_ip, incoming_table[i].next_hop_ip, strcmp(ROUTING_TABLE_V2[j].next_hop_ip, incoming_table[i].next_hop_ip));
        
            //sender knows a shorter route for a given destination
            if ((strcmp(ROUTING_TABLE_V2[j].destination_net, incoming_table[i].destination_net) == 0) &&
                (strcmp(ROUTING_TABLE_V2[j].next_hop_ip, sender) != 0))
            {
                
                // int cost_through_sender = get_sender_cost_v2(sender) + incoming_table[i].cost;
                int cost_through_sender = 1 + incoming_table[i].cost;

                if (cost_through_sender < ROUTING_TABLE_V2[j].cost)
                {
                    ROUTING_TABLE_V2[j].cost = cost_through_sender;
                    strcpy(ROUTING_TABLE_V2[j].next_hop_ip, sender);
                }
                break;
            }
            //this node currently routes to a destination through sender and sender???s cost to that destination has changed
            else if((strcmp(ROUTING_TABLE_V2[j].destination_net, incoming_table[i].destination_net) == 0) &&
                (strcmp(ROUTING_TABLE_V2[j].next_hop_ip, sender) == 0))
            {
                // int cost = get_sender_cost_v2(sender) + incoming_table[i].cost;
                int cost = 1 + incoming_table[i].cost;
                
                if(cost>16)cost=16;
                ROUTING_TABLE_V2[j].cost = cost;
                break;
            }

        }
        
        //sender knows a destination this node didn???t know about
        if (j == ROUTING_TABLE_SIZE)
        {
            strcpy(ROUTING_TABLE_V2[ROUTING_TABLE_SIZE].destination_net, incoming_table[i].destination_net);
            // ROUTING_TABLE_V2[ROUTING_TABLE_SIZE].cost = get_sender_cost_v2(sender) + incoming_table[i].cost;
            ROUTING_TABLE_V2[ROUTING_TABLE_SIZE].cost = 1 + incoming_table[i].cost;
            
            strcpy(ROUTING_TABLE_V2[ROUTING_TABLE_SIZE].next_hop_ip, sender);
            ROUTING_TABLE_SIZE++;
        }
    }

    //update timers for this sender
    int k;
    for(k=0;k<ROUTING_TABLE_SIZE;k++){
        char * this_entry_next_hop = &ROUTING_TABLE_V2[k].next_hop_ip[0];
        char * this_entry_dest = &ROUTING_TABLE_V2[k].destination_net[0];
        if((strcmp(this_entry_next_hop, sender)==0  || strcmp(this_entry_dest, sender)==0) && ROUTING_TABLE_V2[k].cost<16){
            ROUTING_TABLE_V2[k].expiration_time = 0;
            ROUTING_TABLE_V2[k].garbage_collection_time = 0;
        }
        
    }
    update_neighbour_list(sender);

    printf("Updated table:\n");

    print_routing_table_v2();

    pthread_mutex_unlock(&mut);

    return;
}


char * create_rip_response_packet(char * neighbor_ip){
    char packet[100000] = "";
    strcat(packet, command);
    strcat(packet, version);
    strcat(packet, reserved);
    strcat(packet, "\n");
    strcat(packet, family);
    strcat(packet, all_zeros);
    strcat(packet, "\n");

    // printf("------------------------------------------\n");
    // print_routing_table_v2();
    // printf("------------------------------------------\n");

    int i;
    for (i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
        //to avoid poisson reverse
        if(strcmp(neighbor_ip, &ROUTING_TABLE_V2[i].next_hop_ip[0])==0){
            continue;
        }

        strcat(packet, dec_ip_to_bin_ip(ROUTING_TABLE_V2[i].destination_net));
        strcat(packet, "\n");
        strcat(packet, zeros32);
        strcat(packet, "\n");
        strcat(packet, zeros32);
        strcat(packet, "\n");
        strcat(packet, get_32_bit_bin(ROUTING_TABLE_V2[i].cost));
        strcat(packet, "\n");
    
    }
    char * packet_ptr = packet;
    // printf("created data packet:\n%s\n",packet);
    return packet_ptr;
}

void extract_data_and_update_table_v2(char *sender, char * packet){
    char *line = strtok(packet, "\n");
    line = strtok(NULL, "\n");
    // printf("extracted-line: %s\n", line);
    
    int line_number=1;
    char * token;
    struct routing_table_entry_v2 incoming_routing_table[100];
    int incoming_tabl_entry_count = 0;

    while (1)
    {
        token = strtok(NULL, "\n");
        if(token==NULL)break;
        if(line_number%4==1 && token!=NULL){
            strcpy(incoming_routing_table[incoming_tabl_entry_count].destination_net, bin_ip_to_dec_ip(token));
            line_number++;
        }
        else if(line_number%4==0 && token!=NULL)
        {
            incoming_routing_table[incoming_tabl_entry_count].cost = atoi(token);
            line_number = 1;
            incoming_tabl_entry_count++;
        }else
        {
            line_number++;
        }
        
    }

    printf("\nincoming table:\n");
    for(int i=0;i<incoming_tabl_entry_count;i++){
        printf("%s | %d\n", incoming_routing_table[i].destination_net, incoming_routing_table[i].cost);
    }

    update_table_v2(sender, incoming_routing_table, incoming_tabl_entry_count);

    return;
    
}




void *cli(void *arguments)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in my_addr;
    
    // // Explicitly assigning port number 12010 by
    // // binding client with that port
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(ROUTER->sending_port);

    if (bind(client_socket, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in)) == 0)
        printf("bind successfull for sending socket\n");
    else
        perror("Unable to bind");

    struct neighbor *neighbor_def = arguments;
    int client_fd;
    
    int neighbor_port_num = get_port_by_ip(neighbor_def->neighbor_ip);

    struct sockaddr_in neighbor_node;
    neighbor_node.sin_addr.s_addr = INADDR_ANY;
    neighbor_node.sin_family = AF_INET;
    neighbor_node.sin_port = htons(neighbor_port_num);

    
    
    if ((client_fd = connect(client_socket, (struct sockaddr *)&neighbor_node, sizeof(neighbor_def->neighbor_node))) < 0)
    {
        close(client_fd);
        close(client_socket);
        return NULL;
    }

    printf("Connection established from %s to %s\n", ROUTER->ip, neighbor_def->neighbor_ip);
    // char neighbor_port[5];
    // sprintf(neighbor_port, "%d", neighbor_port_num);

    char *data = create_rip_response_packet(neighbor_def->neighbor_ip);


    sendto(client_socket, data, strlen(data), MSG_CONFIRM, (const struct sockaddr *) &neighbor_node,  sizeof(neighbor_node));
    printf("data sent from %s to %s\n\n", ROUTER->ip, neighbor_def->neighbor_ip);
    close(client_fd);
    close(client_socket);
        
    return NULL;
}

void *sendingThread(void *arguments)
{
    printf("Sending thread running!!!!\n");

    struct sending_thread_arg *args = arguments;
    int sock, this_node_port;
    sock = args->sock;
    this_node_port = args->this_port;

    FILE *fptr;
    
    while (1)
    {

        int i;
        pthread_t client_threads[5];
        int client_thread_count = 0;
        
        for (i = 0; i < neighbour_count; i = i + 1)
        {

            int neighbor_port;
            
            pthread_t client_thread_id;
            
            struct neighbor *neighbor_def = malloc(sizeof(struct neighbor));
            
            strcpy( neighbor_def->neighbor_ip, neighbors[i]);
            // printf("neighbor ip: ----%s\n", neighbor_def->neighbor_ip);
            
            pthread_create(&client_thread_id, NULL, cli, neighbor_def);
            
            client_threads[client_thread_count++] = client_thread_id;

            pthread_join(client_thread_id, NULL);
        }
        // int j;
        // for (j = 0; j < client_thread_count; j++)
        // {
        //     pthread_join(client_threads[j], NULL);
        // }
        sleep(periodic_timer+get_rand_val());
    }

    return NULL;
}

void *recievingThread(void *arguments)
{

    
    struct recieving_thread_arg *args = arguments;
    
    struct sockaddr_in this_node = args->this_node;
    int server_fd = args->server_fd;
    int valread, addrlen = sizeof(this_node);
    char buffer[1024] = {0};
    struct sockaddr_in cliaddr;

    while (1)
    {

        memset(&cliaddr, 0, sizeof(cliaddr));
        int len = sizeof(cliaddr);
        recvfrom(server_fd, (char *)buffer, 4024,  MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        char * sender = get_ip_by_sender_port(ntohs(cliaddr.sin_port));
        printf("\nrecieved data from %s: \n", sender);
        extract_data_and_update_table_v2(sender, buffer);
        
        // print_neighbours();
        
    }

    return NULL;
}


void remove_garbage_entry_v2(int position)
{
    printf("removing garbage entry at position %d!!!!\n", position);
    int i, j;
    int routing_table_updated=0,neighbour_list_updated = 0;
    for (i = 0; i < neighbour_count; i++)
    {

        if (strcmp( ROUTING_TABLE_V2[j].destination_net, neighbors[i])==0 )
        {
            strcpy(neighbors[i], neighbors[i+1]);
            neighbour_list_updated=1;
        }
    }
    if(neighbour_list_updated==1)
        neighbour_count--;
    
    
    for (j = position; j < ROUTING_TABLE_SIZE; j++)
    {
        ROUTING_TABLE_V2[j] = ROUTING_TABLE_V2[j + 1];
        routing_table_updated=1;
    }
    if(routing_table_updated==1){
        ROUTING_TABLE_SIZE--;
    }
}



void *timer_v2(void *arguments)
{
    while (1)
    {
        pthread_mutex_lock(&mut);

        int i;
        for (i = 0; i < ROUTING_TABLE_SIZE; i++)
        {
            if (ROUTING_TABLE_V2[i].garbage_collection_time >= entry_garbage_thresh)
            {
                remove_garbage_entry_v2(i);
                i--; //since left shifted all the element deleting i-th element. So now in i position there is new element.
            }
            else if (ROUTING_TABLE_V2[i].expiration_time >= entry_expire_thresh)
            {
                ROUTING_TABLE_V2[i].cost = 16;
                ROUTING_TABLE_V2[i].garbage_collection_time++;
            }
            else
            {
                ROUTING_TABLE_V2[i].expiration_time++;
            }
        }
        pthread_mutex_unlock(&mut);

        sleep(1);
    }
}

int main(int argc, char *argv[])
{

    int id = atoi(argv[1]);
    // char *ip = argv[2];
    int port = atoi(argv[2]);
    PORT = port;

    printf("node-id: %d, port: %d\n", id, port);

    
    read_config_file(id);
    initialization_routing_table_v2();
    
    initial_neighbors();

    char *data = create_rip_response_packet("8088");
    // printf("data created from table:\n%s\n", data);

    
    
    // extract_data_and_update_table_v2("0000",data);
    // extract_data_and_update_table(data);

    int new_socket, client_fd;
    int opt = 1;
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    struct sending_thread_arg *args = malloc(sizeof(struct sending_thread_arg));
    args->sock = server_fd;
    args->this_port = port;
    
    struct recieving_thread_arg *args2 = malloc(sizeof(struct recieving_thread_arg));
    args2->this_node = node;
    args2->server_fd = server_fd;

    
    pthread_create(&sending_thread_id, NULL, sendingThread, args);
    pthread_create(&recieving_thread_id, NULL, recievingThread, args2);
    pthread_create(&timer_thread_id, NULL, timer_v2, NULL);
    
    pthread_join(sending_thread_id, NULL);
    pthread_join(recieving_thread_id, NULL);
    pthread_join(timer_thread_id, NULL);
}
