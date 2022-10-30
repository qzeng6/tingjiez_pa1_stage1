//update
/**
 * @ubitname_assignment1
 * @author  Fullname <ubitname@buffalo.edu>
 * @version 1.0.1
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <vector>
#include "../include/global.h"
#include "../include/logger.h"
#include <cstdlib>
#include <algorithm>
#include <stdint.h>
#include <netdb.h>
#include <map>

#define STDIN 0
#define LOGIN_Bit '0'
#define Refresh_Bit '1'
#define EXIT_Bit '2'
#define Send_Bit '3'
#define Broadcast_Bit '4'
#define Block_Bit '5'
#define Unblock_Bit '6'
#define LOGOUT_Bit '7'


using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

struct client_info{
    int port;
    string ip_addr;
    string hostname;

    int messages_send;
    int messages_received;
    string logging_status;

};

string convertInt(int number)
{
    if (number == 0)
        return "0";
    string temp="";
    string returnvalue="";
    while (number>0)
    {
        temp+=number%10+48;
        number/=10;
    }
    for (int i=0;i<temp.length();i++)
        returnvalue+=temp[temp.length()-i-1];
    return returnvalue;
}
bool input_contains_str(string input, char* str){

    int i=0;
    while(true){
        if(!*str) break;
        if(i>=input.length()) return false;

        if(*str!= input[i]) return false;
        str++;
        i++;
    }
    if(input[i] != ' '){
        return false;
    }
    return true;
}

bool Compare(const client_info a, const client_info b){
    return a.port < b.port;
}

void print_list(vector<client_info>& c) {
    for (int i = 0; i < c.size(); ++i) {
        cout << c[i].hostname.c_str() << endl;
        cse4589_print_and_log("%-5d\t%-35s\t%-20s\t%-8d\n",i+1,c[i].hostname.c_str(),c[i].ip_addr.c_str(),c[i].port);
    }
}

void print_stat(vector<client_info>& c) {
    for (int i = 0; i < c.size(); ++i) {
        int list_id = i+1;
        string hostname = c[i].hostname;
        int num_msg_sent = c[i].messages_send;
        int num_msg_rcv = c[i].messages_received;
        string status = c[i].logging_status;
        cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", list_id, hostname.c_str(), num_msg_sent, num_msg_rcv, status.c_str());
    }
}

bool port_contains_letter(string port){
    for (int i = 0; i < port.size(); ++i) {
        if(!isdigit(port[i])) return true;
    }
    return false;
}


int main(int argc, char **argv)
{
    if(argc<3){
        return -1;
    }
    /*Init. Logger*/
    cse4589_init_log(argv[2]);

    /* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

    /*Start Here*/

    bool is_login = false;
    string SERVER_IP_ADDR = " ";
    int SERVER_PORT_ID = 0;

    int PORT = atoi(argv[2]);

    if(PORT < 1){
        printf("Invalid Port number\n");
        return -1;
    }

    char* role = argv[1];
    vector<client_info> all_client;

    map<string,vector<client_info> > blocked_list;
    map<string,int> socket_map;

    char* _IP = (char*) malloc(256);
    memcpy(_IP,"IP",256);
    char* _PORT = (char*) malloc(256);
    memcpy(_PORT,"PORT",256);
    char* _LIST = (char*) malloc(256);
    memcpy(_LIST,"LIST",256);
    char* _REFRESH = (char*) malloc(256);
    memcpy(_REFRESH,"REFRESH",256);
    char* _EXIT = (char*) malloc(256);
    memcpy(_EXIT,"EXIT",256);
    char* _LOGIN = (char*) malloc(256);
    memcpy(_LOGIN,"LOGIN",256);
    char* _AUTHOR = (char*) malloc(256);
    memcpy(_AUTHOR,"AUTHOR",256);

    char* _STATISTICS = (char*) malloc(32);
    memcpy(_STATISTICS,"STATISTICS",32);
    char* _BLOCKED = (char*) malloc(32);
    memcpy(_BLOCKED,"BLOCKED",32);
    char* _SEND = (char*)malloc(32);
    memcpy(_SEND,"SEND",32);
    char* _BROADCAST = (char*) malloc(32);
    memcpy(_BROADCAST,"BROADCAST",32);
    char* _BLOCK =(char*)malloc(32);
    memcpy(_BLOCK,"UNBLOCK",32);
    char* _UNBLOCK =(char*)malloc(32);
    memcpy(_UNBLOCK,"UNBLOCK",32);
    char* _LOGOUT = (char*) malloc(32);
    memcpy(_LOGOUT,"LOGOUT",32);

    char myIP[16];
    struct sockaddr_in google_addr, my_addr;
    int google_socket;

    // Connect to server
    if ((google_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Can't open stream socket.");
        exit(-1);
    }

    // Set server_addr
    bzero(&google_addr, sizeof(google_addr));
    google_addr.sin_family = AF_INET;
    google_addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    google_addr.sin_port = htons(PORT);

    // Connect to google server
    if (connect(google_socket, (struct sockaddr *) &google_addr, sizeof(google_addr)) < 0) {
        perror("Connect server error");
        close(google_socket);
        exit(-1);
    }

    // Get my ip address and port
    bzero(&my_addr, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    getsockname(google_socket, (struct sockaddr *) &my_addr, &len);
    // inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
    struct addrinfo info, *bind_addr;
    memset(&info, 0, sizeof(addrinfo));
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_STREAM;
    info.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, argv[2], &info, &bind_addr);
    my_addr.sin_port = htons(PORT);

//    printf("Local ip address: %s\n", myIP);
//    printf("Local port : %u\n", ntohs(my_addr.sin_port));

    if (role[0] == 's'){

        int clientSize = 5;
        int opt = 1;
        int clientSocketList[5];
        for(int i=0;i<clientSize;i++){
            clientSocketList[i] = 0;
        }

        if(setsockopt(google_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt) <0){
            cout<<"Socket setting failed!"<<endl;
            return -1;
        }

        int serverSocket;
        serverSocket = socket(AF_INET,SOCK_STREAM,0);
        if(serverSocket<0) cout<<"server socket failed!"<<endl;


        if(bind(serverSocket,bind_addr->ai_addr,bind_addr->ai_addrlen) == -1){
            cout<<"Socket binding failed!"<<endl;
            perror("bind failed. Error");
            return -1;
        }

        if(listen(serverSocket,5) != 0){
            cout<<"server is not ready"<<endl;
        }

        int addrLen = sizeof(my_addr);

//        struct timeval tv = {
//        };
//        tv.tv_sec = 2;
//        tv.tv_usec = 50000;

        for (int i=0;i < clientSize;i++) {
            clientSocketList[i] = 0;
        }

        int max_sd = serverSocket;
        printf("\n[PA1-Server@CSE489/589]$ ");
        fflush(stdout);

        fd_set master_fd;
        fd_set read_fd_set;
        FD_ZERO(&master_fd);
        FD_ZERO(&read_fd_set);
        FD_SET(STDIN,&master_fd);
        FD_SET(serverSocket,&master_fd);

        while(true) {

            read_fd_set = master_fd;

            if (::select(max_sd + 1, &read_fd_set, NULL, NULL, NULL) < 0) {
                perror("error");
                exit('4');
            }

            for (int i = 0; i <= max_sd; i++) {
                if (FD_ISSET(i, &read_fd_set)) {
                    if (i == serverSocket) {
                        int new_fd = accept(serverSocket, (struct sockaddr *) &my_addr, (socklen_t *) &addrLen);
                        if (new_fd == -1) {
                            perror("accept");
                        } else {
                            FD_SET(new_fd, &master_fd);
                            if (new_fd > max_sd) {
                                max_sd = new_fd;
                            }
                            cout << "new connection is joining in" << endl;
                        }
                    } else if(i == STDIN) {

                        string input;
                        std::getline(std::cin,input);
                        if(input == _PORT){
                            cse4589_print_and_log("[PORT:SUCCESS]\n");
                            cse4589_print_and_log("PORT:%d\n", ntohs(my_addr.sin_port));
                            cse4589_print_and_log("[PORT:END]\n");

                        }else if(input == _IP){
                            cse4589_print_and_log("[IP:SUCCESS]\n");
                            cse4589_print_and_log("IP:%s\n", inet_ntoa(my_addr.sin_addr));
                            cse4589_print_and_log("[IP:END]\n");
                        }else if(input == _LIST){
                            cse4589_print_and_log("[LIST:SUCCESS]\n");
                            print_list(all_client);
                            cse4589_print_and_log("[LIST:END]\n");

                        }else if(input == _AUTHOR){
                            cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n","tingjiez");
                            cse4589_print_and_log("[AUTHOR:END]\n");
                        }else if(input == _REFRESH){
                            // server does not refresh
                        }else if(input == _EXIT){
                            cse4589_print_and_log("[EXIT:SUCCESS]\n");
                            clientSocketList[i] = 0;
                            FD_CLR(clientSocketList[i],&read_fd_set);
                            cse4589_print_and_log("[EXIT:END]\n");
                            return 0;
                        }else if(input == _STATISTICS){
                            cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
                            print_stat(all_client);
                            cse4589_print_and_log("[STATISTICS:END]\n");
                        }else if(input_contains_str(input,_BLOCKED) ){
                            string ip_blocked = "";
                            int ip_starting_pos = 9;
                            for (int j = ip_starting_pos; j < input.length(); ++j) {
                                ip_blocked += input[j];
                            }
                            bool ip_not_valid = true;
                            map<string, vector<client_info> >::iterator it;
                            for(it = blocked_list.begin();it!= blocked_list.end();it++){
                                if(it->first == ip_blocked){
                                    ip_not_valid = false;
                                    cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                                    for (int j = 0;j<it->second.size();j++){
                                        cse4589_print_and_log("%-5d\t%-35s\t%-20s\t%-8d\n",i+1,it->second[j].hostname.c_str(),it->second[j].ip_addr.c_str(),it->second[j].port);
                                    }
                                    cse4589_print_and_log("[BLOCKED:END]\n");

                                }
                            }

                            // Exceptions to be handled for not valid ip address input
                            if(ip_not_valid){
                                cse4589_print_and_log("[BLOCKED:FAILED]\n");
                                cse4589_print_and_log("[BLOCKED:END]\n");
                            }

                        }
                        else{
                            cse4589_print_and_log("Invalid request.\n");
                        }


                    } else {
                        char *receive_buf = (char *) malloc(1024);
                        memset(receive_buf, 0, 1024);
                        int recv_byte = recv(i, receive_buf, 1024, 0);
                        cout<<receive_buf<<endl;
                        if (recv_byte == 0) {
                            printf("selectserver: socket %d hung up\n", i);
                            close(i);
                            FD_CLR(i, &master_fd);
                        } else if (recv_byte < 0) {
                            cout<<"recv() return -1"<<endl;
                            perror("recv");
                            //close(i);
                            //FD_CLR(i, &master_fd);
                        } else {

                            char flag = receive_buf[0];
                            struct client_info new_client;
                            // handle new client connection
                            if (flag == LOGIN_Bit) {
                                // collect information for new client
                                int k, m;
                                string buffer_port = "";
                                char *buffer_ip = (char *) malloc(64);
                                char *buffer_host = (char *) malloc(64);
                                memset(buffer_host,0,64);
                                memset(buffer_ip,0,64);
                                for (k = 2, m = 0; receive_buf[k] != ' '; ++k, ++m) {
                                    buffer_ip[m] = receive_buf[k];
                                }
                                for (k = k + 1, m = 0; receive_buf[k] != ' '; ++k, ++m) {
                                    buffer_port += receive_buf[k];
                                }
                                for (k = k + 1, m = 0; k < recv_byte; ++k, ++m) {
                                    buffer_host[m] = receive_buf[k];
                                }
                                new_client.port = atoi(buffer_port.c_str());
                                new_client.ip_addr = buffer_ip;
                                new_client.hostname = buffer_host;
                                new_client.messages_received = 0;
                                new_client.messages_send = 0;
                                new_client.logging_status = "logged-in";

                                // find the right position in sorting list, then insert into it
                                all_client.push_back(new_client);
                                socket_map[buffer_ip] = i;

                                vector<client_info> new_block_list;
                                blocked_list[new_client.ip_addr] = new_block_list;

                                sort(all_client.begin(), all_client.end(), Compare);

                                // SEND BUFFER to client
                                string buffer = "0 ";
                                for (int n = 0; n < all_client.size(); ++n) {

                                    string b = all_client[n].hostname + " " + all_client[n].ip_addr + " " +
                                               convertInt(all_client[n].port) + " ";
                                    buffer += b;
                                }
                                buffer[buffer.size()-1] = '\0';
                                char *send_buf = (char *) malloc(1024);
                                memset(send_buf, 0, 1024);
                                int j;
                                for (j = 0; j < buffer.size(); ++j) {
                                    send_buf[j] = buffer[j];
                                }
                                int sendRes = send(i, (void *) send_buf, j, 0);

                                if(sendRes == -1) cout<<"send client failed."<<endl;

                                cout << "send client_list to client123: " << send_buf << endl;


                            } else if (flag == Refresh_Bit) {
                                // SEND BUFFER to client
                                string buffer = "0 ";
                                for (int j = 0; j < all_client.size(); ++j) {

                                    string b = all_client[j].hostname + " " + all_client[j].ip_addr + " " +
                                               convertInt(all_client[j].port) + " ";
                                    buffer += b;
                                }
                                buffer[buffer.size()-1] = '\0';
                                char *send_buf = (char *) malloc(1024);
                                memset(send_buf, 0, 1024);
                                int j;
                                for (j = 0; j < buffer.size(); ++j) {
                                    send_buf[j] = buffer[j];
                                }
                                int sendRes = send(i, (void *) send_buf, j, 0);

                                if(sendRes == -1) cout<<"send client failed."<<endl;
                                cout << "server recv refresh from client!" << endl;

                            }else if (flag == EXIT_Bit){
                                string p = "";
                                cout<<receive_buf<<endl;
                                for (int j = 1; receive_buf[j] != '\0'; ++j) {
                                    cout<<receive_buf[j];
                                    p +=receive_buf[j];
                                }
                                cout<<endl;
                                cout<<"p:"<<p<<endl;
                                int exit_client = atoi(p.c_str());
                                for (int j = 0; j < all_client.size(); ++j) {
                                    if(all_client[j].port == exit_client){
                                        all_client.erase(all_client.begin()+j);
                                    }
                                }
                                cout<<"portID: "<<exit_client<<" is exited."<<endl;
                                clientSocketList[i] = 0;
                                FD_CLR(clientSocketList[i],&read_fd_set);
                            }else if(flag == Send_Bit){
                                char* from_client_ip = (char*) malloc(32);
                                memset(from_client_ip,0,32);
                                char* to_client_ip = (char*) malloc(32);
                                memset(to_client_ip,0,32);
                                char* msg = (char*) malloc(256);
                                memset(msg,0,256);

                                int index = 2;

                                for (int k=0;receive_buf[index] != ' ';index++,k++) {
                                    cout<<receive_buf[index]<<endl;
                                    from_client_ip[k] = receive_buf[index];
                                }

                                index ++;
                                for(int k=0;receive_buf[index] != ' ';index++,k++){
                                    to_client_ip[k] = receive_buf[index];
                                }

                                index ++;
                                for(int k=0;receive_buf[index] != '\0';index++,k++){
                                    msg[k] = receive_buf[index];
                                }
                                cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_client_ip,to_client_ip, msg);


                                // check if to_client has blocked on from_client
                                bool valid_send = true;
                                map<string, vector<client_info> >::iterator it;
                                for(it = blocked_list.begin();it!= blocked_list.end();it++){
                                    if(it->first == to_client_ip){
                                        for(int j=0;j<it->second.size();j++){
                                            if(it->second[j].ip_addr == from_client_ip){
                                                valid_send = false;
                                                cout<<"to_client has blocked from_client, send enabled!"<<endl;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if(valid_send) {


                                    char *packet = (char *) malloc(512);
                                    memset(packet, 0, 512);
                                    packet[0] = Send_Bit;
                                    packet[1] = ' ';

                                    int k = 2;
                                    char* temp = from_client_ip;
                                    for (; *from_client_ip; from_client_ip++, k++) {
                                        packet[k] = *from_client_ip;

                                    }
                                    from_client_ip = temp;
                                    packet[k] = ' ';
                                    k++;
                                    for (; *msg; msg++, k++) {
                                        packet[k] = *msg;
                                    }
                                    cout << "packet: " << packet << endl;
                                    map<string, int>::iterator it;
                                    for (it = socket_map.begin(); it != socket_map.end(); it++) {
                                        if (it->first == to_client_ip) {
                                            int send_res = send(it->second, (void *) packet, k + 1, 0);
                                            if (send_res > 1) {

                                                for (int m = 0; m < all_client.size(); ++m) {
                                                    if (all_client[m].ip_addr == from_client_ip) {
                                                        all_client[m].messages_send += 1;
                                                    }
                                                }
                                                for (int m = 0; m < all_client.size(); ++m) {
                                                    if (all_client[m].ip_addr == to_client_ip) {
                                                        all_client[m].messages_received += 1;
                                                    }
                                                }
                                                  cse4589_print_and_log("[RELAYED:END]");
//                                                cout << "send to to_client success!" << endl;
                                            } else {
//                                                cout << "send to to_client failed: " << send_res << endl;
                                                  cse4589_print_and_log("[RELAYED:FAILED");
                                                  cse4589_print_and_log("[RELAYED:END]");

                                            }
                                        }

                                    }

                                }

                            }else if(flag == Broadcast_Bit){
                                cout<<"recv: "<<receive_buf<<endl;
                                char* from_client_ip = (char*) malloc(32);
                                memset(from_client_ip,0,32);
                                char* msg = (char*) malloc(256);
                                memset(msg,0,256);
                                int index = 2;

                                for (int k=0;receive_buf[index] != ' ';index++,k++) {
                                    from_client_ip[k] = receive_buf[index];
                                }
                                index ++;
                                for(int k=0;receive_buf[index] != '\0';index++,k++){
                                    msg[k] = receive_buf[index];
                                }

                                char* packet = (char*) malloc(512);
                                memset(packet,0,512);
                                packet[0] = Send_Bit;
                                packet[1] = ' ';

                                int k = 2;
                                char* temp = from_client_ip;
                                for(;*from_client_ip;from_client_ip++,k++){
                                    packet[k] = *from_client_ip;

                                }
                                from_client_ip = temp;
                                packet[k] = ' ';
                                k++;
                                temp=msg;
                                for(;*msg;msg++,k++){
                                    packet[k] = *msg;
                                }
                                msg=temp;

                                map<string, int>::iterator it;
                                for(it = socket_map.begin();it!= socket_map.end();it++){
                                    if(it->first != from_client_ip){
                                        int send_res = send(it->second,(void*)packet,k+1,0);
                                        if(send_res > 1){

                                            for (int m = 0; m < all_client.size(); ++m) {
                                                if(all_client[m].ip_addr == from_client_ip){
                                                    all_client[m].messages_send += 1;
                                                }
                                            }
                                            for (int m = 0; m < all_client.size(); ++m) {
                                                if(all_client[m].ip_addr == it->first){
                                                    all_client[m].messages_received+= 1;
                                                }
                                            }


//                                            printf("msg from:%s, to:%s\n[msg]:%s\n", from_client_ip,it->first.c_str(),msg);
//                                            cout<<"broadcast to clientIP " << it->first<<" success!"<<endl;
                                        }else{
//                                            cout<<"broadcast to clientIP " << it->first<<" failed!"<<endl;
                                            cse4589_print_and_log("[RELAYED:FAILED]");
                                            cse4589_print_and_log("[RELAYED:END]");

                                        }
                                    }
                                }
                                cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_client_ip,"255.255.255.255",msg);
                                cse4589_print_and_log("[RELAYED:END]\n");

                            }else if(flag== Block_Bit){
                                cout<<"execute block command"<<endl;
                                char* client = (char*)malloc(32);
                                memset(client,0,32);
                                char* blocked_client = (char*)malloc(32);
                                memset(blocked_client,0,32);
                                int index = 2;
                                for(int j = 0;receive_buf[index] and receive_buf[index] != ' ';j++,index++){
                                    client[j] = receive_buf[index];
                                }
                                index++;
                                for(int j = 0;receive_buf[index];j++,index++){
                                    blocked_client[j] = receive_buf[index];
                                }

                                map<string, vector<client_info> >::iterator it;
                                for(it = blocked_list.begin();it!= blocked_list.end();it++){
                                    if(it->first == client){
                                        for(int j = 0;j<all_client.size();j++){
                                            if(all_client[i].ip_addr == blocked_client){
                                                it->second.push_back(all_client[i]);
                                                sort(it->second.begin(),it->second.end(), Compare);
//                                                cout<<"block success!"<<endl;
                                                cse4589_print_and_log("[BLOCK:SUCCESS]");
                                                cse4589_print_and_log("[BLOCK:END]");
                                                break;
                                            }
                                        }

                                    }
                                }

                            }else if(flag == Unblock_Bit){
                                char* client = (char*)malloc(32);
                                memset(client,0,32);
                                char* unblocked_client = (char*)malloc(32);
                                memset(unblocked_client,0,32);
                                int index = 2;
                                for(int j = 0;receive_buf[index] and receive_buf[index] != ' ';j++,index++){
                                    client[j] = receive_buf[index];
                                }
                                index++;
                                for(int j = 0;receive_buf[index];j++,index++){
                                    unblocked_client[j] = receive_buf[index];
                                }

                                map<string, vector<client_info> >::iterator it;
                                for(it = blocked_list.begin();it!= blocked_list.end();it++){
                                    if(it->first == client){
                                        for(int j = 0;j<it->second.size();j++){
                                            if(it->second[j].ip_addr == unblocked_client){
                                                it->second.erase(it->second.begin()+j);
                                                cout<<"unblock success!"<<endl;
                                                break;
                                            }
                                        }
                                    }
                                }

                            }

                            //inform user of socket number - used in send and receive commands
                        }
                    }
                }

            }

            printf("\n[PA1-Server@CSE489/589]$ ");
            fflush(stdout);



        }
    }
    else if (role[0] == 'c') {
        int opt =1;
        if(setsockopt(google_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt) <0){
            cout<<"Socket setting failed!"<<endl;
            return -1;
        }

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            cout << "client socket set up failed" << endl;
            return -1;
        }

        printf("\n[PA1-Client@CSE489/589]$ ");
        fflush(stdout);

        int max_sd = clientSocket;


        fd_set master_fd;
        fd_set read_fd_set;
        FD_ZERO(&master_fd);
        FD_ZERO(&read_fd_set);
        FD_SET(STDIN,&master_fd);
        FD_SET(clientSocket,&master_fd);

        while(true) {

            read_fd_set = master_fd;
            if (::select(max_sd + 1, &read_fd_set, NULL, NULL, NULL) < 0) {
                perror("error");
                exit('4');
            }

            for (int i = 0; i <= max_sd; i++) {

                if (FD_ISSET(i, &read_fd_set)) {
                    if (i == clientSocket) {

                        char *server_packet = (char *) malloc(1024);
                        memset(server_packet,0,1024);
                        int recv_byte = recv(i, server_packet, 1024, 0);

                        if (recv_byte == 0) {
                            cout<<"recv() return 0"<<endl;
                            printf("selectserver: socket %d hung up\n", i);
                            close(i);
                            FD_CLR(i, &master_fd);

                        } else if (recv_byte < 0 and errno != ENOTCONN) { //and errno != ENOTCONN
                            cout<<"recv() return -1"<<endl;
                            perror("recv");
                            //close(i);
                            //FD_CLR(i, &master_fd);

                        } else {

                            if(server_packet[0] == LOGIN_Bit){

                                server_packet += 2;
                                vector<client_info> client_from_server;
                                cout<<"client recv: "<<server_packet<<endl;
                                while (*server_packet){

                                    client_info c;
                                    string str = "";
                                    while (*server_packet !=' '){
                                        str += *server_packet;
                                        server_packet++;
                                    }

                                    c.hostname = str;
                                    str = "";
                                    server_packet++;

                                    while (*server_packet !=' '){
                                        str += *server_packet;
                                        server_packet++;
                                    }

                                    c.ip_addr = str;
                                    str = "";
                                    server_packet++;
                                    while (*server_packet and *server_packet !=' '){
                                        str += *server_packet;
                                        server_packet++;
                                        if(*server_packet == '\0') break;
                                    }
                                    if(*server_packet == ' ') server_packet++;
                                    c.port = atoi(&str[0]);
                                    client_from_server.push_back(c);
                                }
                                all_client = client_from_server;
                            }
                            else if (server_packet[0] == Refresh_Bit) {
                                cout << "client recv refresh from server!" << endl;
                            }
                            else if(server_packet[0] == Send_Bit){

                                char* from_client_ip = (char*) malloc(32);
                                memset(from_client_ip,0,32);
                                char* msg = (char*)malloc(256);
                                memset(msg,0,256);
                                int index = 2;
                                for (int j = 0; server_packet[index] and server_packet[index] != ' '; ++j,index++) {
                                    from_client_ip[j] = server_packet[index];
                                }
                                index++;
                                for (int j = 0; server_packet[index]; ++j,index++) {
                                    msg[j] = server_packet[index];
                                }
                                cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                cse4589_print_and_log("msg from:%s\n",from_client_ip);
                                cse4589_print_and_log("[msg]:%s\n",msg);
                                cse4589_print_and_log("[RECEIVED:end]\n");
                            }

                            //inform user of socket number - used in send and receive commands
                        }

                    }
                    else if(i == STDIN) {

                        string input;
                        std::getline(std::cin,input);

                        if(input == _PORT){
                            cse4589_print_and_log("[PORT:SUCCESS]\n");
                            cse4589_print_and_log("PORT:%d\n", htons(my_addr.sin_port));
                            cse4589_print_and_log("[PORT:END]\n");

                        }else if(input == _IP){
                            cse4589_print_and_log("[IP:SUCCESS]\n");
                            cse4589_print_and_log("IP:%s\n", inet_ntoa(my_addr.sin_addr));
                            cse4589_print_and_log("[IP:END]\n");
                        }else if(input == _LIST){
                            cse4589_print_and_log("[LIST:SUCCESS]\n");
                            print_list(all_client);
                            cse4589_print_and_log("[LIST:END]\n");

                        }else if(input == _AUTHOR){
                            cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n","qzeng6");
                            cse4589_print_and_log("[AUTHOR:END]\n");
                        }else if(input == _REFRESH){
                            char* sendmsg = (char*) malloc(1);
                            memset(sendmsg,0,1);
                            sendmsg[0] = 1;
                            if(send(clientSocket, (void *) sendmsg, 2, 0) == -1){
                                cout<<"refresh send failed"<<endl;
                            } else{
                                cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                                cse4589_print_and_log("[REFRESH:END]\n");
                            }

                        }else if(input_contains_str(input,_LOGIN)){

                            // login to server
                            string input_ip_addr, input_portID;
                            int index;
                            for (index = 6; input[index] != ' ' and input[index] != '\0'; index++) {
                                input_ip_addr += input[index];
                            }

                            for (index+= 1; input[index] != '\n' and input[index] != '\0' ; index++) {
                                input_portID += input[index];
                            }

                            char *_ipaddr = &input_ip_addr[0];
                            struct in_addr server_ip_addr;
                            inet_aton(_ipaddr, &(server_ip_addr));

                            int server_pID = atoi(&input_portID[0]);
                            cout<<"server_pID: "<<server_pID<<endl;
                            sockaddr_in server_sock;
                            memset(&server_sock,0, len);
                            server_sock.sin_family = AF_INET;
                            server_sock.sin_port = htons(server_pID);
                            server_sock.sin_addr = server_ip_addr;
                            cout<< "server_sock: "<< server_sock.sin_port<<endl;

                            cout<<"connecting to "<<_ipaddr<<":"<<server_sock.sin_port<<endl;

                            if(connect(clientSocket,(sockaddr *)&server_sock,len) < 0){
                                //if (connect(clientSocket, connaddr->ai_addr, connaddr->ai_addrlen)) {
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                cout<<"failed to connect 1"<<endl;
                                break;
                            }else if(port_contains_letter(input_portID)){
                                cout<<"PORT contains letter!"<<endl;
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                cout<<"failed to connect 2"<<endl;
                                break;
                            }else {
                                is_login = true;
                                SERVER_IP_ADDR = input_ip_addr;
                                SERVER_PORT_ID = atoi(input_portID.c_str());
                                cout<<"server_port_id :"<< SERVER_PORT_ID<<endl;
                                //my_addr = new_addr;
                                my_addr.sin_port = htons(PORT);
                                cout<<"my_addr :"<< my_addr.sin_port<<endl;
                                // send msg to server
                                string packet_flag = "0";
                                string packet_addr = inet_ntoa(my_addr.sin_addr);
                                string packet_port = convertInt(ntohs(my_addr.sin_port));
                                char *name = (char *) malloc(64);
                                gethostname(name, 64);

                                string packet_hostname = name;
                                string space = " ";
                                string packet = packet_flag + space + packet_addr + space + packet_port + space +
                                                packet_hostname;
                                char *buf = (char *) malloc(1024);
                                int j;
                                for (j = 0; j < packet.size(); ++j) {
                                    buf[j] = packet[j];
                                }
                                int sendRes = send(clientSocket, (void *) buf, j, 0);
                                if (sendRes == -1) {
                                    cout << "Send LOGIN msg to server Failed!" << endl;
                                } else {
                                    //cout << "Send msg success" << buf << endl;
                                }
                                cse4589_print_and_log("[LOGIN:SUCCESS]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                            }
                        }else if(input_contains_str(input,_SEND)){
                            string client_ip = "";
                            int client_ip_starting_pos = 5;
                            int j = client_ip_starting_pos;
                            for (; j < input.size(); ++j) {
                                if(input[j] == ' ') break;
                                client_ip += input[j];
                            }
                            bool invalid_ip_addr = true;

                            for (int k = 0; k < all_client.size(); ++k) {
                                if(all_client[k].ip_addr == client_ip){
                                    invalid_ip_addr = false;
                                }
                            }

                            // what if clientA send msg to itself?
                            // UNCOMMENT LATER
                            if(client_ip == inet_ntoa(my_addr.sin_addr)){
                                invalid_ip_addr = true;
                            }
                            // Exceptions
                            if(invalid_ip_addr){
                                cse4589_print_and_log("[SEND:FAILED]\n");
                                cse4589_print_and_log("[SEND:END]\n");
                            }else{
                                string msg = "";
                                for (j+=1;j<input.size();++j) {
                                    msg += input[j];
                                }

                                char* sendmsg = (char*) malloc(512);
                                memset(sendmsg,0,512);
                                // sendmsg = Send_Bit from_ip to_ip msg
                                sendmsg[0] = Send_Bit;
                                sendmsg[1] = ' ';
                                string fromIP = inet_ntoa(my_addr.sin_addr);
                                int index = 2;
                                for (int k=0; k < fromIP.size(); ++k,index++) {
                                    sendmsg[index] = fromIP[k];
                                }

                                sendmsg[index] = ' ';
                                index++;
                                for(int k=0;k<client_ip.size();k++,index++){
                                    sendmsg[index] = client_ip[k];
                                }

                                sendmsg[index] = ' ';
                                index++;
                                for (int k = 0; k < msg.size(); ++k,index++) {
                                    sendmsg[index] = msg[k];
                                }


                                if(send(clientSocket, (void *) sendmsg, 512+1, 0) == -1){
                                    cout<<"send msg failed"<<endl;
                                } else{
                                    cout<<"send msg success!"<<endl;
                                    cout<<sendmsg<<endl;
                                    cse4589_print_and_log("[SEND:SUCCESS]\n");
                                    cse4589_print_and_log("[SEND:END]\n");
                                }


                            }



                        }
                        else if(input == _EXIT){
                            if(is_login){
                                char* sendbuf = (char*)malloc(128);
                                memset(sendbuf,0,128);
                                sendbuf[0] = '2';
                                string sendPORT = convertInt(PORT);
                                cout<<"port: "<<PORT<<sendPORT<<endl;
                                for (int j = 0; j < sendPORT.size(); ++j) {
                                    sendbuf[j+1] = sendPORT[j];
                                }
                                cout<<"exit sending buf: "<<sendbuf<<endl;

                                if(send(clientSocket, (void *) sendbuf, sizeof(sendbuf)+1, 0) == -1){
                                    cout<<"refresh send failed"<<endl;
                                } else{
                                    //cse4589_print_and_log("[EXIT:SUCCESS]\n");
                                    //cse4589_print_and_log("[EXIT:END]\n");
                                }
                                is_login = false;
                                return 0;
                            }
                        }else if(input_contains_str(input,_BROADCAST)){
                            int index_start_pos = 10;
                            char* msg = (char*) malloc(256);
                            memset(msg,0,256);
                            for (int j = index_start_pos,k=0; input[j] != '\0'; ++j,k++) {
                                msg[k] = input[j];
                            }
                            cout<<"msg:"<< msg<<endl;
                            char* sendBuf = (char*) malloc(512);
                            memset(sendBuf,0,512);
                            sendBuf[0] = Broadcast_Bit;
                            sendBuf[1] = ' ';
                            string fromIP = inet_ntoa(my_addr.sin_addr);

                            int index = 2;
                            for(int j=0;fromIP[j]!='\0';j++,index++){
                                sendBuf[index] = fromIP[j];
                            }
                            sendBuf[index] = ' ';
                            cout<<"fromIP: "<<fromIP<<endl;
                            for(index+=1;*msg;index++,msg++){
                                sendBuf[index] = *msg;
                            }

                            cout<<"sendbuf: "<<sendBuf<<endl;
                            if(send(clientSocket, (void *) sendBuf, 512+1, 0) == -1){
                                cout<<"broadcast msg failed"<<endl;
                            } else{
                                cout<<"broadcast msg success!"<<endl;
                                cout<<sendBuf<<endl;
                                cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
                                cse4589_print_and_log("[BROADCAST:END]\n");
                            }

                        }else if(input_contains_str(input,_BLOCKED)){
                            int index_start_pos = 8;

                            char* blocked_client = (char*) malloc(32);
                            memset(blocked_client,0,32);
                            for (int j = index_start_pos,k=0; input[j] != '\0'; ++j,k++) {
                                blocked_client[k] = input[j];
                            }
                            string clientIp = inet_ntoa(my_addr.sin_addr);
                            char* sendBuf = (char*)malloc(64);
                            memset(sendBuf,0,64);
                            sendBuf[0] = Block_Bit;
                            sendBuf[1] = ' ';
                            int index = 2;
                            for(int j = 0;j<clientIp.size();j++,index++){
                                sendBuf[index] = clientIp[j];
                            }
                            sendBuf[index] = ' ';
                            index++;
                            for(;*blocked_client;blocked_client++,index++){
                                sendBuf[index] = *blocked_client;
                            }
                            if(send(clientSocket, (void *) sendBuf, 64+1, 0) == -1){
//                                cout<<"block failed"<<endl;
//                                cse4589_print_and_log("")
                                cse4589_print_and_log("[BLOCKED:FAILED]\n");
                                cse4589_print_and_log("[BLOCKED:END]\n");
                            } else{
                                cout<<"block success!"<<endl;
                                cout<<sendBuf<<endl;
                                cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                                cse4589_print_and_log("[BLOCKED:END]\n");
                            }


                        }else if(input == _LOGOUT){
                            is_login = false;
                            char* sendBuf = (char*) malloc(32);
                            sendBuf[0] = LOGOUT_Bit;
                            sendBuf[1] = ' ';
                            string clientIp = inet_ntoa(my_addr.sin_addr);
                            for (int j = 0; j < clientIp.size(); ++j) {
                                sendBuf[j+2] = clientIp[j];
                            }

                            if(send(clientSocket, (void *) sendBuf, 32+1, 0) == -1){
                                cout<<"logout failed"<<endl;
                            } else{

                                cout<<"logout success!"<<endl;
                                cout<<sendBuf<<endl;
                                cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
                                cse4589_print_and_log("[LOGOUT:END]\n");
                            }


                        }else{
                            printf("Client receive an invalid request.\n");
                        }


                        printf("\n[PA1-Client@CSE489/589]$ ");
                        fflush(stdout);

                    } else {
                        cout<<"wrong entry"<<endl;
                    }
                }
            }

            //free(server_packet);
        }

    } else {
        printf("invalid parameter!\n");
        return -1;
    }

    return 0;
}
