/*
    > Read every 4th line in Reuters
    > Send it over the network
*/

#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <vector>

#define PORT 8080
#define BUFFER_SIZE_DEFAULT 32
#define FILENAME "PTI_file.csv"
#define SKIP 3

class server
{
    private:
        std::ifstream file_input;
        int current_line;
        //std::string filename;

        std::vector<uint64_t> values;
        uint32_t news_ID;
        uint32_t update_type;
        uint64_t sequence_number;
        uint64_t timestamp; 

        WSADATA win_sock_data;
        int WSA_startup_return;
        int WSA_cleanup_return;

        //int port_number;
        SOCKET TCP_server_socket;
        int close_socket_return;
        struct sockaddr_in TCP_server;

        SOCKET TCP_accepted_socket;

        int bind_return;
        int listen_return;

        unsigned char sender_buffer[BUFFER_SIZE_DEFAULT];//Intentionally a little larger.
        //int sender_buffer_size;
        int send_return;

        void pushNews_IDToByteStream()
        {
            sender_buffer[0] = (news_ID >> 24) & 0xFF;
            sender_buffer[1] = (news_ID >> 16) & 0xFF;
            sender_buffer[2] = (news_ID >> 8) & 0xFF;
            sender_buffer[3] = news_ID & 0xFF;
        }

        void pushUpdate_typeToByteStream()
        {
            sender_buffer[4] = (update_type >> 24) & 0xFF;
            sender_buffer[5] = (update_type >> 16) & 0xFF;
            sender_buffer[6] = (update_type >> 8) & 0xFF;
            sender_buffer[7] = update_type & 0xFF;
        }

        void pushSequence_numberToByteStream()
        {
            sender_buffer[8] = (sequence_number >> 56) & 0xFF;
            sender_buffer[9] = (sequence_number >> 48) & 0xFF;
            sender_buffer[10] = (sequence_number >> 40) & 0xFF;
            sender_buffer[11] = (sequence_number >> 32) & 0xFF;
            sender_buffer[12] = (sequence_number >> 24) & 0xFF;
            sender_buffer[13] = (sequence_number >> 16) & 0xFF;
            sender_buffer[14] = (sequence_number >> 8) & 0xFF;
            sender_buffer[15] = sequence_number & 0xFF;
        }

        void pushTimestampToByteStream()
        {
            sender_buffer[16] = (timestamp >> 56) & 0xFF;
            sender_buffer[17] = (timestamp >> 48) & 0xFF;
            sender_buffer[18] = (timestamp >> 40) & 0xFF;
            sender_buffer[19] = (timestamp >> 32) & 0xFF;
            sender_buffer[20] = (timestamp >> 24) & 0xFF;
            sender_buffer[21] = (timestamp >> 16) & 0xFF;
            sender_buffer[22] = (timestamp >> 8) & 0xFF;
            sender_buffer[23] = timestamp & 0xFF;
        }


    public:
        void serverSetup()
        {
            file_input.open(FILENAME, std::ifstream::in);
            current_line = 0;

            WSA_startup_return = WSAStartup(MAKEWORD(2,2), &win_sock_data); //WSA Version 2.2
            if(WSA_startup_return!=0)
            {
                std::cout<<"WSAStartup failed"<<std::endl;
                return;
            }
            std::cout<<"WSAStartup successful"<<std::endl;

            TCP_server.sin_family = AF_INET;
            TCP_server.sin_addr.s_addr = inet_addr("127.0.0.1");
            TCP_server.sin_port = htons(PORT);

            TCP_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(TCP_server_socket == INVALID_SOCKET)
            {
                std::cout<<"Server socket creation failed"<<std::endl;
                return;
            }
            std::cout<<"Server socket creation successful"<<std::endl;

            bind_return = bind (TCP_server_socket, (SOCKADDR*)&TCP_server, sizeof(TCP_server));
            if(bind_return == SOCKET_ERROR)
            {
                std::cout<<"Bind failed"<<std::endl;
                return;
            }
            std::cout<<"Binding successful"<<std::endl;

            listen_return = listen(TCP_server_socket, 2);
            if(listen_return==SOCKET_ERROR)
            {
                std::cout<<"Listen failed"<<std::endl;
                return;
            }
            std::cout<<"Listen Successful"<<std::endl; 
        }

        void sendData()
        {
            std::cout<<"Waiting for a connection request..."<<std::endl;

            struct sockaddr_in TCP_client;
            int TCP_client_size = sizeof(TCP_client);
            SOCKET new_socket = accept(TCP_server_socket, (SOCKADDR*)&TCP_client, &TCP_client_size);

            TCP_accepted_socket = new_socket;
            if(TCP_accepted_socket == INVALID_SOCKET)
            {
                std::cout<<"Accept error"<<std::endl;
                return;
            }
            std::cout<<"Accept successful"<<std::endl;

            while(!file_input.eof())
            {
                current_line = 0;
                std::string line;
                std::string val;
                values.clear();   

                while(current_line++<SKIP)
                {                    
                    std::getline(file_input, line);
                }

                file_input>>line;
                std::stringstream ss(line);
                while (std::getline(ss, val, ','))
                {
                    values.push_back(stol(val));
                }
                news_ID = values[0];
                update_type = values[1];
                sequence_number = values[2];
                timestamp = values[3];
                
                std::cout<<news_ID<<","<<update_type<<","<<sequence_number<<","<<timestamp<<std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                pushNews_IDToByteStream();
                pushUpdate_typeToByteStream();
                pushSequence_numberToByteStream();
                pushTimestampToByteStream();

                send_return = send(TCP_accepted_socket, (char*)sender_buffer, BUFFER_SIZE_DEFAULT, 0);
                if(send_return == SOCKET_ERROR)
                {
                    std::cout<<"Send failed"<<std::endl;
                    return;
                }
                std::cout<<"sending data successful"<<std::endl;
            }
        }

        void cleanupAndClose()
        {
            system("PAUSE");//Abruptly closing the socket might lead to loss of data
            
            file_input.close();
            std::cout<<"File closed"<<std::endl;
            close_socket_return = closesocket(TCP_server_socket);
            if(close_socket_return == SOCKET_ERROR)
            {
                std::cout<<"Socket error in close"<<std::endl;
                return;
            }
            std::cout<<"Closed successfully"<<std::endl;

            WSA_cleanup_return = WSACleanup();
            if(WSA_cleanup_return == SOCKET_ERROR)
            {
                std::cout<<"WSACleaup error"<<std::endl;
                return;
            }
            std::cout<<"WSACleaup successful"<<std::endl;
        }
};

int main()
{
    server reuters;

    reuters.serverSetup();
    reuters.sendData();
    reuters.cleanupAndClose();

    return 0;
}
