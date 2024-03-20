Project was originally written in VSCode 1.87.0, compiled and run on Windows 10 with g++12.2.0. Minimum C++ version required: C++11.
We print a consolidated feed of "NewsItem"s after receiving data from two servers (called Reuters and Bloomberg).

Notes on the problem:
1. Based on the problem statement, it is clear that the sequence number of every individual NewsItem is unique and one greater than the previous NewsItem. This is the central variable with which we organise our feed.
2. On the server side, we have presented a file generator (PTI_file_generator.cpp) which generates a PTI_file.csv file containing "MAX_NEWS_ITEMS"x4 NewsItems. MAX_NEWS_ITEMS is set to 100 initially. Every single NewsItem has 1 NEW, 2 MODIFICATIONs and 1 DELETE. This is not super important since the entire algorithm is based on the sequence number, but this has been implemented for the sake of clarity.
3. After this, we have 2 servers (bloomberg.cpp and reuters.cpp) which read NewsItems from PTI_file.csv and send them over the network. They both use TCP sockets on Windows and their port numbers are defined at the top of the files (8000 and 8080 currently). Importantly, both of them have a variable called SKIP, which indicates how many NewsItems they are going to skip while reading from the file. Currently, Bloomberg is set to read every single line, while Reuters reads one in every 4 lines. This creates two out-of-sync data streams while maintaining the requirement that every single line of data is indeed received by the client.
4. On the client side, connecting to the servers and reading from them is handled inside the functions getBloombergNewsItem() and getReutersNewsItem(). As far as the end-user is concerned, they receive only one NewsItem per call to either of these two functions.
5. Since it was mentioned that we are supposed to call getBloombergNewsItem() and getReutersNewsItem() in an infinite loop, we have not used two separate threads to handle the two connections. Instead, we have an infinite loop where getBloombergNewsItem() and getReutersNewsItem() are called sequentially until both the data streams have ended. We have to be careful not to close the sockets till all the data has been read, or unread data will be lost.

Notes on the algorithm:
1. We use a simple priority_queue, which is a wrapper around a different data structure (in this case a min-heap wrapper around an std::vector) to store every NewsItem received. Once per loop, we check the top of the priority_queue to see if we have received the next NewsItem (based on sequence number).
2. If we have received duplicate NewsItems, we simply pop the top of the priority_queue without printing. Thus, we maintain order while ignoring duplicates.
3. Based on the unpredictable nature of the data streams combined with the guarantee that every single news item will be present in the consolidated feed, using a priority_queue (a min-heap) is the most obvious solution for me.

Compilation instructions:
1. Compile the PTI_file_generator.cpp file on Windows using g++: g++ PTI_file_generator.cpp -o PTI_file_generator
2. Run the PTI_file_generator.exe to generate a PTI_file.csv containing comma-separated NewsItems.
3. Compile the servers on windows with g++ bloomberg.cpp -lws2_32 -o bloomberg and g++ reuters.cpp -lws2_32 -o reuters
4. Start up the servers
5. Compile the client with g++ consolidated_feed.cpp -lws2_32 -o consolidated_feed
6. Run the client. It prints a consolidated feed of data received from the two servers.
