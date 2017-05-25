#include "common.h"
//#define NET_DEBUG

#define EXIT_FAILURE 1

typedef struct limits
{
    double left;
    double right;
} limits_t;

double calculate_integral(int thrds_qty, double left, double right);

long pasreInput(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: ./command <quantity_of_threads>\n");
        exit(-1);
    }
    
    char* endptr = NULL;
    errno = 0;  
    long threads_req = strtol(argv[1], &endptr, 10);
    if (errno != 0)
    {
        perror("Strtol:");
        exit(errno);
    }
    if (*endptr != '\0')
    {
        printf("2nd arg is not a number\n");
        exit(-1);
    }

    return threads_req;
}

/*void alarm_checker(int signo)
{
    conn_timed_out = 1;
}*/

int main(int argc , char *argv[])
{
    const unsigned udp_port = 8886;
    const unsigned tcp_port = 8888;
    const int threads_req = (int)pasreInput(argc, argv);

    //UDP connection
    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udp_sock == -1)
    {
        perror("UDP Server socket creation");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in udp_sock_in;
    udp_sock_in.sin_addr.s_addr = INADDR_ANY;
    udp_sock_in.sin_port = htons(udp_port);
    udp_sock_in.sin_family = AF_INET;

    //bind udp socket
    if(bind(udp_sock, (struct sockaddr *)&udp_sock_in, sizeof(struct sockaddr)) == -1)
    {
        perror("UDP server binding");
        exit(EXIT_FAILURE);
    }

    //tcp connections
    int tcp_sock = socket(AF_INET , SOCK_STREAM , 0);
    if(tcp_sock == -1)
    {
        perror("TCP server socket creation");
        exit(EXIT_FAILURE);
    }
    //make tcp_sock nonblock
    int flags = fcntl(tcp_sock, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(tcp_sock, F_SETFL, flags);

    int keepalive = 1;
    int keepcnt = 1;
    int keepidle = 1;
    int keepintvl = 1;

    if (setsockopt (tcp_sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket:");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket:");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket:");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket:");
        exit(EXIT_FAILURE);
    }

    //alarm handler
    /*struct sigaction response;
    memset(&response, 0, sizeof(response));
    response.sa_handler = alarm_checker;
    if(sigaction(SIGALRM, &response, NULL) == -1)
    {
        perror("Sigaction");
        exit(EXIT_FAILURE);
    }*/
    /* After recieving broadcast msg, alarm is set to ensure that we will
     * not block forever trying to connect to the client, who needn't new servers.
     * I am using epoll with time limit to ensure connect to succeed.
     */
    int epollfd = epoll_create1(0);
    if(epollfd == -1)
    {
        perror("epoll");
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.fd = tcp_sock;
    struct epoll_event events[64];
    int conn_success = 0;
    do
    {
        //recieve and process client adress through udp broadcast msg 
        unsigned int sockaddr_len = sizeof(struct sockaddr);
        struct in_addr client_addr;
        memset(&client_addr, 0, sizeof(struct in_addr));
        if(recvfrom(udp_sock, &client_addr, sizeof(struct in_addr), 0, 
            (struct sockaddr *)&udp_sock_in, &sockaddr_len) == -1)
        {
            perror("UDP server broadcast msg recv");
            exit(EXIT_FAILURE);
        }

        #ifdef NET_DEBUG
        //display result
        printf("recv = %s\n", inet_ntoa(((struct sockaddr_in *)&udp_sock_in)->sin_addr));
        printf("content = %s\n", inet_ntoa(client_addr));
        #endif
        fprintf(stderr, "Broadcast message recieved.\n");

        struct sockaddr_in dest;
        dest.sin_addr = client_addr;
        dest.sin_port = htons(tcp_port);
        dest.sin_family = AF_INET;

        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, tcp_sock, &event) == -1)
        {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
        connect(tcp_sock, (struct sockaddr *)&dest, sizeof(struct sockaddr));
        if(errno == EINPROGRESS)
        {
            int qty = epoll_wait(epollfd, events, 64, 10000);
            if(qty == -1)
            {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
            for (int j = 0; j < qty; ++j)
            {
                if(events[j].events & EPOLLERR ||
                    events[j].events & EPOLLHUP ||
                    events[j].events & EPOLLRDHUP)
                    //error occured
                {
                    printf("Socket problem registered by epoll. Aborting...\n");
                    exit(EXIT_FAILURE);
                }
                else if(events[j].events & EPOLLOUT)
                {
                    int optval = 0;
                    socklen_t optlen = 0;
                    if (getsockopt(tcp_sock, SOL_SOCKET, SO_ERROR, &optval, &optlen) != 0)
                    {
                        perror("Unable to set parameters for socket:");
                        exit(EXIT_FAILURE);
                    }
                    if(epoll_ctl(epollfd, EPOLL_CTL_DEL, tcp_sock, &event) == -1)
                    {
                        perror("epoll_ctl");
                        exit(EXIT_FAILURE);
                    }
                    if(optval == 0)
                        conn_success++;
                }
            }            
        }

        /*if(connect(tcp_sock, (struct sockaddr *)&dest, sizeof(struct sockaddr)) < 0)
        {
            printf("TCP server connect: Connection unsuccessful\n");
            exit(EXIT_FAILURE);
        }*/
    }
    while(!conn_success);

    close(udp_sock);
    fprintf(stderr, "Connection established.\n");

    //MAKE tcp_sock NONBLOCK AGAIN!
    flags = 0;
    flags = fcntl(tcp_sock, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(tcp_sock, F_SETFL, flags);

    if(send(tcp_sock, &threads_req, sizeof(int), 0) < 0)
    {
        printf("TCP server send: unsuccessful. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    limits_t limit;
    if(recv(tcp_sock, &limit, sizeof(limits_t), 0) < 0)
    {
        printf("TCP server recv: unsuccessful. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Necessary data recieved. Starting caclulation routine...\n");
    double res = calculate_integral(threads_req, limit.left, limit.right);
    fprintf(stderr, "Calculated value: res = %g\n", res);
    if(send(tcp_sock, &res, sizeof(double), 0) < 0)
    {
        printf("TCP server result send: unsuccessful. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    //sync message as an approval for ending
    int sync;
    if(recv(tcp_sock, &sync, sizeof(int), 0) < 0)
    {
        printf("TCP server sync recv: unsuccessful. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    close(tcp_sock);
     
    return 0;
}