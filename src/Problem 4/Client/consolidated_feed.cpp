/*
    > getBloombergNewsItem() and getReutersNewsItem() called in an inf loop
    > They receive into a buffer, push it into a NewsItem queue and return one NewsItem at a time.
    > main thread compares the sequence numbers, starting with 1
    > Have the functions do the following:
        > Read every packet in the buffer and push everything into a queue (order doesn't matter)
        > return the front of the queue and pop it
        > if queue is empty, return a dropped packet (all 0s)
*/

#include <iostream>
#include <winsock2.h>
#include <queue>
#include <iomanip>
#include <vector>

#define REUTERS_PORT 8080
#define BLOOMBERG_PORT 8000
#define BUFFER_SIZE_DEFAULT 12800
#define PACKET_SIZE_DEFAULT 32


namespace clients
{
    struct NewsItem
    {
        uint32_t news_ID;
        uint32_t update_type;
        uint64_t sequence_number;
        uint64_t timestamp;

        explicit NewsItem(int i)
        {
            this->news_ID = i;
            this->update_type = i;
            this->sequence_number = i;
            this->timestamp = i;
        }

        NewsItem()
        {

        }

        friend std::ostream& operator<<(std::ostream& output, const NewsItem& news)
        {
            output<<"News ID: "<<std::setw(20) << std::left <<news.news_ID<<"Update Type: ";
            if(news.update_type == 1)
            {
                output<<std::setw(20) << std::left <<"NEW";
            }
            else if(news.update_type == 2)
            {
                output<<std::setw(20) << std::left <<"MODIFICATION";
            }
            else if(news.update_type == 3)
            {
                output<<std::setw(20) << std::left <<"DISCARD";
            }
            else
            {
                output<<std::setw(20) << std::left <<"FAKE NEWS";
            }
            output<<"Sequence Number: "<<std::setw(20) << std::left <<news.sequence_number<<"Timestamp: "<<news.timestamp<<"\n";

            return output;
        }
    };


    WSADATA client_data;

    SOCKET bloomberg_socket;
    struct sockaddr_in bloomberg_server;
    int bloomberg_connection_status;
    int bloomberg_buffer_size;
    unsigned char bloomberg_buffer[BUFFER_SIZE_DEFAULT];
    std::queue<NewsItem> bloomberg_queue;
    bool end_of_bloomberg_stream;

    SOCKET reuters_socket;
    struct sockaddr_in reuters_server;
    int reuters_connection_status;
    int reuters_buffer_size;
    unsigned char reuters_buffer[BUFFER_SIZE_DEFAULT];
    std::queue<NewsItem> reuters_queue;
    bool end_of_reuters_stream;

    void initBloomberFeed()
    {
        bloomberg_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        bloomberg_server.sin_family = AF_INET;
        bloomberg_server.sin_addr.s_addr = inet_addr("127.0.0.1");
        bloomberg_server.sin_port = htons(BLOOMBERG_PORT);

        bloomberg_connection_status = connect(bloomberg_socket, (SOCKADDR*)&bloomberg_server, sizeof(bloomberg_server));

        if(bloomberg_connection_status == SOCKET_ERROR)
        {
            std::cout<<"Error connecting to Bloomberg server"<<std::endl;
            return;
        }
        std::cout<<"Successfully connected to Bloomberg server"<<std::endl;

        bloomberg_buffer_size = 0;
        end_of_bloomberg_stream = false;
    }

    void initReutersFeed()
    {
        reuters_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        reuters_server.sin_family = AF_INET;
        reuters_server.sin_addr.s_addr = inet_addr("127.0.0.1");
        reuters_server.sin_port = htons(REUTERS_PORT);

        reuters_connection_status = connect(reuters_socket, (SOCKADDR*)&reuters_server, sizeof(reuters_server));

        if(reuters_connection_status == SOCKET_ERROR)
        {
            std::cout<<"Error connecting to Reuters server"<<std::endl;
            return;
        }
        std::cout<<"Successfully connected to Reuters server"<<std::endl;

        reuters_buffer_size = 0;
        end_of_reuters_stream = false;
    }

    void initAll()
    {
        int WSA_return = WSAStartup(MAKEWORD(2,2), &client_data);
        if(WSA_return!=0)
        {
            std::cout<<"WSAStartup failed"<<std::endl;
            return;
        }
        std::cout<<"WSAStartup successful"<<std::endl;

        initBloomberFeed();
        initReutersFeed();
    }

    uint32_t byteToNews_ID(unsigned char* buffer, int start_pos)
    {
        uint32_t news_ID = int(
                            (uint32_t)(buffer[start_pos]) << 24 |
                            (uint32_t)(buffer[start_pos+1]) << 16 |
                            (uint32_t)(buffer[start_pos+2]) << 8 |
                            (uint32_t)(buffer[start_pos+3]));

        return news_ID;
    }

    uint32_t byteToUpdate_type(unsigned char* buffer, int start_pos)
    {
        uint32_t update_type = int(
                            (uint32_t)(buffer[start_pos+4]) << 24 |
                            (uint32_t)(buffer[start_pos+5]) << 16 |
                            (uint32_t)(buffer[start_pos+6]) << 8 |
                            (uint32_t)(buffer[start_pos+7]));

        return update_type;
    }

    uint64_t byteToSequence_number(unsigned char* buffer, int start_pos)
    {
        uint64_t sequence_number = int(
                            (uint64_t)(buffer[start_pos+8]) << 56 |
                            (uint64_t)(buffer[start_pos+9]) << 48 |
                            (uint64_t)(buffer[start_pos+10]) << 40 |
                            (uint64_t)(buffer[start_pos+11]) << 32 |
                            (uint64_t)(buffer[start_pos+12]) << 24 |
                            (uint64_t)(buffer[start_pos+13]) << 16 |
                            (uint64_t)(buffer[start_pos+14]) << 8 |
                            (uint64_t)(buffer[start_pos+15]));
        
        return sequence_number;
    }

    uint64_t byteToTimestamp(unsigned char* buffer, int start_pos)
    {
        uint64_t timestamp = int(
                            (uint64_t)(buffer[start_pos+16]) << 56 |
                            (uint64_t)(buffer[start_pos+17]) << 48 |
                            (uint64_t)(buffer[start_pos+18]) << 40 |
                            (uint64_t)(buffer[start_pos+19]) << 32 |
                            (uint64_t)(buffer[start_pos+20]) << 24 |
                            (uint64_t)(buffer[start_pos+21]) << 16 |
                            (uint64_t)(buffer[start_pos+22]) << 8 |
                            (uint64_t)(buffer[start_pos+23]));
        
        return timestamp;
    }

    NewsItem getBloombergNewsItem()
    {
        bloomberg_buffer_size = 0;
        if(!end_of_bloomberg_stream)
        {
            bloomberg_buffer_size = recv(bloomberg_socket, (char*)bloomberg_buffer, BUFFER_SIZE_DEFAULT, 0);    
        }
        if(bloomberg_buffer_size==-1 || bloomberg_buffer_size==0)
        {
            if(bloomberg_queue.empty())
            {
                end_of_bloomberg_stream = true;
                //std::cout<<"Returning null from bloomberg"<<std::endl;
                return NewsItem(0);
            }
            //std::cout<<"Bloomberg stream over but queue not empty"<<std::endl;
        }
        else if(bloomberg_buffer_size==SOCKET_ERROR)
        {
            std::cout<<"Socket error while receiving from Bloomberg"<<std::endl;
            end_of_bloomberg_stream = true;
            return NewsItem(0);
        }

        int start = 0;
        
        while(bloomberg_buffer_size>=PACKET_SIZE_DEFAULT)
        {
            NewsItem latest_news;
            latest_news.news_ID = byteToNews_ID(bloomberg_buffer, start);
            latest_news.update_type = byteToUpdate_type(bloomberg_buffer, start);
            latest_news.sequence_number = byteToSequence_number(bloomberg_buffer, start);
            latest_news.timestamp = byteToTimestamp(bloomberg_buffer, start);

            if(latest_news.sequence_number == 0)
            {
                end_of_bloomberg_stream = true;
                break;
            }
            
            bloomberg_queue.push(latest_news);

            bloomberg_buffer_size-=PACKET_SIZE_DEFAULT;
            start += PACKET_SIZE_DEFAULT;
        }

        if(!bloomberg_queue.empty())
        {
            NewsItem new_news = bloomberg_queue.front();
            bloomberg_queue.pop();

            //std::cout<<"Returning from Bloomberg: "<<new_news;

            return new_news;
        }
        else
        {
            //std::cout<<"Returning null from bloomberg"<<std::endl;
            return NewsItem(0);
        }
        //std::cout<<"stuck here"<<std::endl;
    }

    NewsItem getReutersNewsItem()
    {
        reuters_buffer_size = 0;

        if(!end_of_reuters_stream)
        {
            reuters_buffer_size = recv(reuters_socket, (char*)reuters_buffer, BUFFER_SIZE_DEFAULT, 0);
        }
        
        if(reuters_buffer_size==-1 || reuters_buffer_size == 0)
        {
            if(reuters_queue.empty())
            {
                end_of_reuters_stream = true;
                //std::cout<<"Returning null from reuters"<<std::endl;
                return NewsItem(0);
            }
            //std::cout<<"Reuters stream over but queue not empty"<<std::endl;            
        }
        else if(reuters_buffer_size==SOCKET_ERROR)
        {
            std::cout<<"Socket error while receiving from Reuters"<<std::endl;
            end_of_reuters_stream = true;
            return NewsItem(0);
        }

        int start = 0;
        
        while(reuters_buffer_size>=PACKET_SIZE_DEFAULT)
        {
            //std::cout<<"Stuck here"<<std::endl;
            NewsItem latest_news;
            latest_news.news_ID = byteToNews_ID(reuters_buffer, start);
            latest_news.update_type = byteToUpdate_type(reuters_buffer, start);
            latest_news.sequence_number = byteToSequence_number(reuters_buffer, start);
            latest_news.timestamp = byteToTimestamp(reuters_buffer, start);

            if(latest_news.sequence_number == 0)
            {
                end_of_reuters_stream = true;
                break;
            }
            
            reuters_queue.push(latest_news);

            reuters_buffer_size-=PACKET_SIZE_DEFAULT;
            start += PACKET_SIZE_DEFAULT;
        }

        if(!reuters_queue.empty())
        {
            NewsItem new_news = reuters_queue.front();
            reuters_queue.pop();

            //std::cout<<"Returning from Reuters: "<<new_news;

            return new_news;
        }
        else
        {
            //std::cout<<"Returning null from reuters"<<std::endl;
            return NewsItem(0);
        }
    }

    void closeAllAndCleanup()
    {
        int close_return_bloomberg = closesocket(bloomberg_socket);
        if(close_return_bloomberg == SOCKET_ERROR)
        {
            std::cout<<"Error closing Bloomberg socket"<<std::endl;
            return;
        }
        std::cout<<"Closed Bloomberg socket"<<std::endl;

        int close_return_reuters = closesocket(reuters_socket);
        if(close_return_reuters == SOCKET_ERROR)
        {
            std::cout<<"Error closing Reuters socket"<<std::endl;
            return;
        }
        std::cout<<"Closed Reuters socket"<<std::endl;

        int WSA_cleanup_return = WSACleanup();
        if(WSA_cleanup_return == SOCKET_ERROR)
        {
            std::cout<<"WSACleaup error"<<std::endl;
            return;
        }
        std::cout<<"WSACleaup successful"<<std::endl;
    }   
}

class Comparator
{
public:
    bool operator()(clients::NewsItem N1, clients::NewsItem N2)
    {
        return N1.sequence_number>N2.sequence_number;
    }
};

int main()
{
    clients::initAll();
    clients::NewsItem bloomberg_news;
    clients::NewsItem reuters_news;

    int current_sequence = 0;
    std::priority_queue<clients::NewsItem, std::vector<clients::NewsItem>, Comparator> news_queue;

    while(1)
    {
        bloomberg_news = clients::getBloombergNewsItem();
        if(bloomberg_news.sequence_number!=0)
        {
            news_queue.push(bloomberg_news);
        }        

        reuters_news = clients::getReutersNewsItem();
        if(reuters_news.sequence_number!=0)
        {
            news_queue.push(reuters_news);
        }

        if((bloomberg_news.sequence_number == 0) && (reuters_news.sequence_number == 0) && (news_queue.empty()))
        {
            std::cout<<"End of both Reuters and Bloomberg streams, all data printed"<<std::endl;
            break;
        }
        else if(news_queue.top().sequence_number == current_sequence)
        {
            news_queue.pop();
        }
        else if(news_queue.top().sequence_number == current_sequence+1)
        {
            std::cout<<news_queue.top();
            current_sequence++;
            news_queue.pop();
        }
    }

    clients::closeAllAndCleanup();

    return 0;
}
