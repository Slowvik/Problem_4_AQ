#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#define MAX_NEWS_ITEMS 100
#define NEWS_ID_BASE 10

int main()
{
    std::ofstream file_stream("PTI_file.csv", std::ofstream::out);

    //uint32_t news_ID = NEWS_ID_BASE;
    //uint32_t update_type = 0;
    uint64_t sequence_number = 1;
    
    //auto start_of_day =  std::chrono::high_resolution_clock::now();
    //auto time_now = std::chrono::high_resolution_clock::now();  

    uint64_t timestamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();


    for(int i=1; i<=MAX_NEWS_ITEMS; i++)
    {
        timestamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        file_stream<<NEWS_ID_BASE*i<<","<<"1"<<","<<sequence_number++<<","<<timestamp<<"\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(3));

        timestamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        file_stream<<NEWS_ID_BASE*i<<","<<"2"<<","<<sequence_number++<<","<<timestamp<<"\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        timestamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        file_stream<<NEWS_ID_BASE*i<<","<<"2"<<","<<sequence_number++<<","<<timestamp<<"\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(8));

        timestamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        file_stream<<NEWS_ID_BASE*i<<","<<"3"<<","<<sequence_number++<<","<<timestamp<<"\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(2));        
    }

    file_stream<<0<<","<<0<<","<<0<<","<<0; //no newline at the end of file, leads to reading errors because eof() isn't reached at the last line if a newline character is present

    file_stream.close();

    return 0;
}