#include <netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5

struct send_data_row{
    char destination[20];
    char cost[20];
};

struct arg_struct
{
    int sock;
    int neighbor_port;
};

struct arg_struct2
{
   struct sockaddr_in this_node;
   int server_fd;
};


void create_sending_data(struct send_data_row *list_of_data, int length){

    for(int i=1;i<=length ; i++){
        //struct send_data_row row= list_of_data[i];
        char data[50]="";
        //snprintf(data, 50, row.destination, row.cost);
        strcat(data, list_of_data[i].destination);
        strcat(data, "");
        strcat(data, list_of_data[i].cost);
        strcat(data, "\n");
    }
}


void *sendingThread(void *arguments)
{
    struct sockaddr_in  neighbor_node;
    int neighbor_port;
    int client_fd, sock;

    struct arg_struct *args = arguments;
    sock = args->sock;
    neighbor_port = args->neighbor_port;

    neighbor_node.sin_addr.s_addr = INADDR_ANY;
    neighbor_node.sin_family = AF_INET;
    neighbor_node.sin_port = htons(neighbor_port);


    while(1){



        if ((client_fd
            = connect(sock, (struct sockaddr*)&neighbor_node,
                    sizeof(neighbor_node)))
            < 0) {
            printf("\nConnection Failed \n");
            //return -1;
        }else{
            printf("\nConnection established from to port %d", neighbor_port);
            send(sock, "Hellooooooooo", strlen("Hellooooooooo"), 0);
            break;

        }
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
    	char buffer[1024] = { 0 };



    while(1){
        if (listen(server_fd, 3) < 0) {
            perror("listen");
            continue;
        }
        if ((new_socket
            = accept(server_fd, (struct sockaddr*)&this_node,
                    (socklen_t*)&addrlen))
            < 0) {
            perror("accept");
            continue;
        }
        valread = read(new_socket, buffer, 1024);
        printf("%s\n", buffer);
	}

    return NULL;
}

int main(int argc, char *argv[] ){

    int all_ports[5] = {8088, 8089, 8090};

    int id = atoi(argv[1]);
    int port = atoi(argv[2]);
    //int other_port = atoi(argv[3]);

    printf("%d %d", id, port);
    char buffer[1024] = { 0 };
    int new_socket, valread, client_fd;
    int opt = 1;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
    struct sockaddr_in node, neighbor_node;
    struct sockaddr_in neighbors[5];

    node.sin_addr.s_addr = INADDR_ANY;
    node.sin_family = AF_INET;
    node.sin_port = htons(port);


    if (bind(server_fd, (struct sockaddr*)&node,sizeof(node))< 0) {
        perror("bind failed\n");
        //exit(EXIT_FAILURE);
    }else{
        printf("bind successful %d\n", port);
    }

    pthread_t sending_thread_id, recieving_thread_id;
    int i;

    struct arg_struct *args = malloc(sizeof (struct arg_struct));
    args->sock = server_fd;
    args->neighbor_port = all_ports[i] ;
    //printf("%d", sending_thread_id);
    //printf("%d",&sendingThread);
    //pthread_create(&sending_thread_id, NULL, &sendingThread, args);
    if (pthread_create(&sending_thread_id, NULL, &sendingThread, args) != 0)
    {
          printf("Uh-oh!\n");
          return -1;
    }

    struct arg_struct2 *args2 = malloc(sizeof (struct arg_struct));
    args2->this_node = node;
    args2->server_fd = server_fd ;
    //pthread_create(&recieving_thread, NULL, &recievingThread, args2);


	/*int i;

	for(i=0;i<3;i++){
        if(all_ports[i]==port){
            continue;

        }else{
            neighbor_node.sin_addr.s_addr = INADDR_ANY;
            neighbor_node.sin_family = AF_INET;
            neighbor_node.sin_port = htons(all_ports[i]);


            while(1){
                if ((client_fd = connect(server_fd, (struct sockaddr*)&neighbor_node, sizeof(neighbor_node)))< 0) {
                    printf("\n%d Connection Failed \n", id);

                }else{
                    printf("Connected %d\n", all_ports[i]);
                    break;
                }
                //sleep(id);
            }

        }
	}

	int addrlen = sizeof(node);


	int count = 0;*/

/*	while(1){

        if (connect(server_fd , (struct sockaddr *)&node , sizeof(node)) < 0)
        {
            puts("connect error");

        }
        if(count == 0){
            char *message = "Hello from node: 1\n";
            send(server_fd, message, strlen(message), 0);
        }
        count = (count+1)%25;

        if (listen(server_fd, BUFFER_SIZE) < 0) {
            perror("listen");

        }
        if (new_socket = accept(server_fd, (struct sockaddr*)&node, (socklen_t*)&addrlen) < 0) {
            perror("accept");
        }

        valread = read(new_socket, buffer, 1024);
        printf("%s\n", buffer);

	}

*/
}
