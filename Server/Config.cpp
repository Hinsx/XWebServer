#include "Config.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
void usage()
{
    printf("Usage: XWebServer [options]\n");
    printf("Options:\n");
    printf("%-20s the number of threads\n","  -c <number>");
    printf("%-20s enable asynchronous logging\n","  -a");
    printf("%-20s log level\n","  -l <0|1|2>");
    printf("%-20s chose poll mode\n","  -m");
    printf("%-20s server name\n","  -n <name>");
    printf("%-20s server ip\n","  -i <IP>");
    printf("%-20s server port\n","  -p <port>");
    printf("%-20s connection idle time\n","  -w <seconds>");
    printf("%-20s maximum number of connections\n","  -s <number>");
    printf("%-20s maximum number of sql connections\n","  -q <number>");
    //printf("Usage:[-c threadNums] [-a open asynclog] [-l loglevel] [-m close epoll(chose poll)] [-n server name] [i server ip] [p server port] [-w connection idle time] [-s connections number] [-q sql connections number]\n");
}
void Config::parse_arg(int argc, char *argv[])
{
    int opt;
    const char *str = "c:al:mn:i:p:w:s:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'c':
        {
            threadNum_ = atoi(optarg);
            break;
        }
        case 'a':
        {
            asynclogging = true;
            break;
        }
        case 'l':
        {
            logLevel_ = atoi(optarg);
            break;
        }
        case 'm':
        {
            mode = false;
            break;
        }
        case 'n':
        {
            name_ = optarg;
            break;
        }
        case 'i':
        {
            ip_ = optarg;
            break;
        }
        case 'p':
        {
            port_ = atoi(optarg);
            break;
        }
        case 'w':
        {
            idle_ = atoi(optarg);
            break;
        }
        case 's':
        {
            connectionNums_ = atoi(optarg);
            break;
        }
        case 'q':
        {
            sqlConnectionNums_ = atoi(optarg);
            break;
        }
        case '?':
        {
            usage();
            exit(0);
        }
        }
    }
}