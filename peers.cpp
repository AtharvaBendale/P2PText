#include "config.hpp"

pthread_mutex_t lock;
peer_t peer;

void* receiving_thread(void* port){
    pthread_mutex_lock(&lock);
    std::cout << "Peer receiving thread initiated!" << std::endl;
    std::cout.flush();
    pthread_mutex_unlock(&lock);
    char buffer[MAX_QUERY_LEN];
    bzero(buffer, sizeof(buffer));
    char sender_name[MAX_NAME_LEN], message[MAX_QUERY_LEN];
    bzero(sender_name, sizeof(sender_name));
    bzero(message, sizeof(message));

    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(*(int*) port);
    peer_addr.sin_addr.s_addr = INADDR_ANY;

    int peer_fd, new_socket, option = 1;
    if ((peer_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("peer socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the given port
    if (setsockopt(peer_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(peer_fd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    for(;;){
        if(listen(peer_fd, 10) < 0){
            perror("listen");
            exit(EXIT_FAILURE);
        }
        if ((new_socket = accept(peer_fd, NULL, NULL)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        read(new_socket, buffer, MAX_QUERY_LEN * SCHAR);
        strncpy(sender_name, buffer, MAX_NAME_LEN);
        strncpy(message, buffer + SCHAR * MAX_NAME_LEN, (MAX_QUERY_LEN - MAX_NAME_LEN));
        pthread_mutex_lock(&lock);
        std::cout << "[" << sender_name << "] " << message << std::endl << "> ";
        std::cout.flush();
        pthread_mutex_unlock(&lock);
        close(new_socket);
    }
}


void* sending_thread(void* port){
    std::cout << "Peer sending thread initiated!" << std::endl;
    std::cout.flush();
    char buffer[MAX_QUERY_LEN], input_buffer[MAX_QUERY_LEN], peer_name[MAX_NAME_LEN], ip_addr[MAX_IP_ADDR_LEN], receiver_name[MAX_NAME_LEN], port_string[MAX_PORT_LENGTH];
    bzero(buffer, sizeof(buffer));
    bzero(peer_name, sizeof(peer_name));
    bzero(receiver_name, sizeof(receiver_name));
    std::cout << "Please enter user-name : ";
    std::cout.flush();
    if(std::cin.peek()=='\n') std::cin.ignore();
    std::cin.getline(peer_name, sizeof(peer_name));

    // Making the file name in which records will be stored
    std::string peer_storage_file_name = "./peer_storage/peer_";
    peer_storage_file_name += peer_name;
    peer_storage_file_name += ".txt";

    // Setting connection with tracker
    int tracker_fd, receiver_fd;
    struct sockaddr_in tracker_addr;
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(available_ports[TOTAL_CLIENT_PORTS%available_ports.size()]);
    if ((tracker_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error : tracker_fd");
        exit(EXIT_FAILURE);
    }
    if ((receiver_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error : receiver_fd");
        exit(EXIT_FAILURE);
    }
    if(inet_pton(AF_INET, TRACKER_ADDRESS, &tracker_addr.sin_addr) <= 0){
        perror("Invalid tracker IP address");
        exit(EXIT_FAILURE);
    }
    if (connect(tracker_fd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    std::ifstream posssible_peer_storage_file(peer_storage_file_name);

    if(posssible_peer_storage_file.good())
    {
        // Retrieving old contact records
        posssible_peer_storage_file.getline(port_string, MAX_PORT_LENGTH);
        *(int*) port = std::atoi(port_string);
        std::cout << "Welcome back! Continuing on port : " << *(int*) port << '\n';
        for(int contact_t = 0; !posssible_peer_storage_file.eof() && contact_t < MAX_CONTACTS; contact_t++)
        {
            peer.contacts[contact_t] = (char*) malloc(SCHAR * MAX_NAME_LEN);
            peer.contacts_ipaddr[contact_t] = (char*) malloc(SCHAR * MAX_IP_ADDR_LEN);
            posssible_peer_storage_file.getline(peer.contacts[contact_t], MAX_NAME_LEN);
            posssible_peer_storage_file.getline(peer.contacts_ipaddr[contact_t], MAX_IP_ADDR_LEN);
            posssible_peer_storage_file.getline(port_string, MAX_PORT_LENGTH);
            peer.contacts_port[contact_t] = std::atoi(port_string);
        }
    } else {
        // Registering new User
        buffer[0] = 'R';
        strncpy(buffer+SCHAR, peer_name, MAX_NAME_LEN);
        get_current_ip(ip_addr);
        strncpy(buffer+SCHAR*(1+MAX_NAME_LEN), ip_addr, MAX_IP_ADDR_LEN);
        send(tracker_fd, (void*) buffer, sizeof(buffer), 0);
        std::cout.flush();
        read(tracker_fd, buffer, sizeof(buffer));
        *(int*) port = atoi(buffer);
        std::cout << "Registration successfull, choosen port : " << *(int*) port << std::endl;
        std::cout.flush();
    }
    pthread_mutex_unlock(&lock);
    
    // Prepping sending sockaddr object
    struct sockaddr_in receiver_addr;
    receiver_addr.sin_family = AF_INET;

    close(tracker_fd);
    close(receiver_fd);
    sleep(0);       // To make sure receiving thread is initiated
    for(;;)
    {
        std::cout << "> ";
        bzero(input_buffer, sizeof(input_buffer));
        bzero(buffer, sizeof(buffer));
        if(std::cin.peek()=='\n') std::cin.ignore();

        // -------------- COMMANDS --------------
        // Add/add : Adds the given user to contact list
        //          '> Add Atharva'
        // Send/send : Prompts user to enter a message which is then sento provided user
        //          '> Send Riddhesh'
        //          '> Please enter your message : This is the message'
        // Exit/exit : Logs off the user from the session
        //          '> exit'

        std::cin.getline(input_buffer, sizeof(input_buffer));
        if(input_buffer[0] == 'E' || input_buffer[0] == 'e'){
            // Expected command 'Exit'/'exit'
            std::ofstream peer_storage_file;
            peer_storage_file.open(peer_storage_file_name);
            peer_storage_file << *(int*) port;
            for(int contact_t = 0; contact_t < MAX_CONTACTS; contact_t++){
                if(peer.contacts[contact_t]){
                    peer_storage_file << '\n' << peer.contacts[contact_t] << '\n' << peer.contacts_ipaddr[contact_t] << '\n' << peer.contacts_port[contact_t];
                    peer_storage_file.flush();
                    free(peer.contacts[contact_t]);
                    free(peer.contacts_ipaddr[contact_t]);
                }
            }
            exit(EXIT_SUCCESS);
        }
        if ((tracker_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error : tracker_fd");
            exit(EXIT_FAILURE);
        }
        if ((receiver_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error : receiver_fd");
            exit(EXIT_FAILURE);
        }
        if (connect(tracker_fd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&lock);
        if( input_buffer[0] == 'A' || input_buffer[0] == 'a' )
        {
            // Expected command 'Add'/'add'
            std::cout << "Adding contact..." << std::endl;
            int free_index = 0;
            for(free_index = 0; free_index < MAX_CONTACTS; free_index++){
                if(!peer.contacts[free_index]) break;
            }
            buffer[0] = 'G';
            peer.contacts[free_index] = (char*) malloc(SCHAR * MAX_NAME_LEN);
            strncpy(peer.contacts[free_index], input_buffer+4*SCHAR, MAX_NAME_LEN);
            strncpy(buffer+SCHAR, input_buffer+4*SCHAR, MAX_NAME_LEN);
            send(tracker_fd, buffer, sizeof(buffer), 0);
            if(read(tracker_fd, buffer, sizeof(buffer)) < 0){
                perror("read problem");
                exit(EXIT_FAILURE);
            }
            peer.contacts_ipaddr[free_index] = (char*) malloc(SCHAR * MAX_IP_ADDR_LEN);
            strncpy(peer.contacts_ipaddr[free_index], buffer, MAX_IP_ADDR_LEN);
            strncpy(port_string, buffer+SCHAR*MAX_IP_ADDR_LEN, MAX_PORT_LENGTH);
            peer.contacts_port[free_index] = std::atoi(port_string);
            std::cout << "Successfully added " << peer.contacts[free_index] << std::endl;
        }
        else if( input_buffer[0] == 'S' || input_buffer[0] == 's' )
        {
            // Expected command 'Send'/'send'
            strncpy(receiver_name, input_buffer + 5*SCHAR, MAX_NAME_LEN);
            int receiver_index = 0;
            for(receiver_index=0; receiver_index < MAX_CONTACTS; receiver_index++){
                if(strlen(peer.contacts[receiver_index]) && !strcmp(receiver_name, peer.contacts[receiver_index])) break;
            }
            receiver_addr.sin_port = htons(peer.contacts_port[receiver_index]);
            if(receiver_index >= MAX_CONTACTS){
                perror("Demanded user either not available or not added to contacts");
                exit(EXIT_FAILURE);
            }
            if(inet_pton(AF_INET, peer.contacts_ipaddr[receiver_index], &receiver_addr.sin_addr) < 0){
                perror("Invalid tracker IP address in send operation");
                exit(EXIT_FAILURE);
            }
            if(connect(receiver_fd, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0){
                perror("Connection failed in send operation");
                exit(EXIT_FAILURE);
            }
            else std::cout << "Please enter your message : ";
            strncpy(buffer, peer_name, sizeof(peer_name));
            if(std::cin.peek()=='\n') std::cin.ignore();
            std::cin.getline(buffer+sizeof(peer_name), MAX_QUERY_LEN-MAX_NAME_LEN);
            send(receiver_fd, buffer, sizeof(buffer), 0);
        }
        pthread_mutex_unlock(&lock);
        close(tracker_fd);
        close(receiver_fd);
    }

}


int main() {

    for(int i=0; i<MAX_CONTACTS; i++){
        peer.contacts_ipaddr[i] = NULL;
        peer.contacts[i] = NULL;
    }

    int cur_port;

    pthread_t sender, receiver;

    pthread_mutex_lock(&lock);
    pthread_create(&sender, NULL, sending_thread, (void*) &cur_port);
    pthread_create(&receiver, NULL, receiving_thread, (void*) &cur_port);

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

}
