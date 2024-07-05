#include "config.hpp"

tracker_t tracker;
pthread_mutex_t lock;

void* start_tracker(void* port){
    pthread_mutex_lock(&lock);
    std::cout << "Tracker initiated with address : " << TRACKER_ADDRESS << ", port : " << *(int*) port << std::endl;
    pthread_mutex_unlock(&lock);
    char buffer[MAX_QUERY_LEN], client_ipaddr[MAX_IP_ADDR_LEN], client_name[MAX_NAME_LEN], port_string[MAX_PORT_LENGTH];
    bzero(buffer, sizeof(buffer));
    bzero(client_ipaddr, sizeof(client_ipaddr));
    bzero(client_name, sizeof(client_name));
    char *success = "SUCCESS", *failure = "FAILURE";
    int tracker_fd, new_socket, option = 1, addrlen;

    // Initializing tracker thread
    struct sockaddr_in tracker_addr;
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(*(int*) port);
    tracker_addr.sin_addr.s_addr = INADDR_ANY;

    for(;;){
        // Creating socket file descriptor
        if ((tracker_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // Forcefully attaching socket to the given port
        if (setsockopt(tracker_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(tracker_fd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        // close(new_socket);
        if(listen(tracker_fd, 10) < 0){
            perror("listen");
            exit(EXIT_FAILURE);
        }
        if ((new_socket = accept(tracker_fd, NULL, NULL)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        read(new_socket, buffer, sizeof(buffer));
        pthread_mutex_lock(&lock);
        // MESSAGE SHOULD START WITH MESSAGE IDENTIFIER:
        // 'R' FOR NEW USER REGISTRATION
        // 'G' FOR OBTAINING A PEER'S IP ADDRESS
        // 'D' FOR DELETING CURRENT USER
        // 'L' FOR LOGGING OFF
        if (buffer[0] == 'R')
        {
            int free_directory_entry;
            for(free_directory_entry = 0; free_directory_entry < MAX_USERS; free_directory_entry++){
                if(!tracker.directory[free_directory_entry]) break;
            }
            if(free_directory_entry >= MAX_USERS){
                perror("free_directory_entry");
                exit(EXIT_FAILURE);
            }
            tracker.directory[free_directory_entry] = (char*) malloc(MAX_NAME_LEN * SCHAR);
            tracker.peers_ipaddr[free_directory_entry] = (char*) malloc((MAX_IP_ADDR_LEN+MAX_PORT_LENGTH) * SCHAR);
            strncpy(tracker.directory[free_directory_entry], buffer+SCHAR, MAX_NAME_LEN);
            strncpy(tracker.peers_ipaddr[free_directory_entry], buffer+SCHAR*(1+MAX_NAME_LEN), MAX_IP_ADDR_LEN);
            sprintf(tracker.peers_ipaddr[free_directory_entry]+SCHAR*MAX_IP_ADDR_LEN, "%d", client_available_ports.back());
            client_available_ports.pop_back();
            send(new_socket, tracker.peers_ipaddr[free_directory_entry]+SCHAR*MAX_IP_ADDR_LEN, sizeof(tracker.peers_ipaddr[free_directory_entry]+SCHAR*MAX_IP_ADDR_LEN), MSG_CONFIRM);
            std::cout << "User " << tracker.directory[free_directory_entry] << " succesfully registered in P2PText" << std::endl;
        }
        else if(buffer[0] == 'G'){
            strncpy(client_name, buffer+SCHAR, MAX_NAME_LEN);
            int query_peer;
            for(query_peer=0; query_peer<MAX_USERS; query_peer++){
                if(!strcmp(tracker.directory[query_peer], client_name)) break;
            }
            if(query_peer >= MAX_USERS){
                perror("demanded user not found");
                exit(EXIT_FAILURE);
            }
            if(send(new_socket, tracker.peers_ipaddr[query_peer], SCHAR*(MAX_IP_ADDR_LEN+MAX_PORT_LENGTH), 0) < 0){
                perror("error in sending ip address");
                exit(EXIT_FAILURE);
            }
        }
        pthread_mutex_unlock(&lock);
        bzero(buffer, sizeof(buffer));
        bzero(client_ipaddr, sizeof(client_ipaddr));
        bzero(client_name, sizeof(client_name));
        close(tracker_fd);
        close(new_socket);
    }
}


int main() {
    
    pthread_t tracker_set[available_ports.size()];

    for(int i=0; i<available_ports.size(); i++){
        pthread_create(&tracker_set[i], NULL, start_tracker, (void*)&available_ports[i]);
    }

    for(int i=0; i<available_ports.size(); i++){
        pthread_join(tracker_set[i], NULL);
    }

}
