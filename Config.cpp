#include"Config.h"
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
void usage(){
    printf("Usage:[-c threadNums] [-a] [-l loglevel] [-m] [-n server name] [i server ip] [p server port] [-w connection idle time] [-s max connections number]\n");
}
void Config::parse_arg(int argc, char*argv[]){
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
            asynclogging = 1;
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
            ip_= optarg;
            break;
        }
        case 'p':
        {
            port_ = atoi(optarg);
            break;
        }
        case 'w':
        {
            idle_=atoi(optarg);
            break;
        }
        case 's':
        {
            connectionNums_=atoi(optarg);
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