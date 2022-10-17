#include <netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 5
int main(int argc, char *argv[] ){

    int all_ports[5] = {8088, 8089, 8090, 8091, 8092 };

    int id = atoi(argv[1]);
    int port = atoi(argv[2]);
    int other_port = atoi(argv[3]);

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
        exit(EXIT_FAILURE);
    }else{
        printf("bind successful %d\n", port);
    }




	int i;

	for(i=0;i<5;i++){
        if(all_ports[i]==port){
            continue;

        }else{
            neighbor_node.sin_addr.s_addr = INADDR_ANY;
            neighbor_node.sin_family = AF_INET;
            neighbor_node.sin_port = htons(other_port);


            while(1){
                if ((client_fd = connect(server_fd, (struct sockaddr*)&neighbor_node, sizeof(neighbor_node)))< 0) {
                    printf("\nConnection Failed \n");

                }else{
                    printf("Connected %d\n", other_port);
                    break;
                }
                //sleep(id);
            }
            break;

        }
	}

	int addrlen = sizeof(node);


	int count = 0;

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
