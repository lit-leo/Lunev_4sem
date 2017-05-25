#include "common.h"
#define NET_DEBUG
//#define INET_LOOPBACK/**/
#define EXIT_FAILURE 1

long pasreInput(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: ./command <quantity_of_servers>\n");
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

typedef struct limits
{
    double left;
    double right;
} limits_t;

typedef struct server_fd
{
    int fd;
    int threads_avail;
    limits_t limit;
} server_struc_t;

int conn_acc = 0;
int conn_req;
void alarm_checker(int signo)
{
    if (conn_acc != conn_req)
    {
        printf("TCP client connection time exceeded. Aborting...\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc , char *argv[])
{
    //alarm handler
    struct sigaction response;
    memset(&response, 0, sizeof(response));
    response.sa_handler = alarm_checker;
    if(sigaction(SIGALRM, &response, NULL) == -1)
    {
        perror("Sigaction");
        exit(EXIT_FAILURE);
    }

    const unsigned port = 8888;
    const int servers_qty = (int)pasreInput(argc, argv);
    conn_req = servers_qty;
    const double left = 0;
    const double right = 20;
    
    //UDP - broadcast connection
    int udp_socket;
    if((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("UDP client socket creation");
        exit(EXIT_FAILURE);
    }

    //get current ip adress
    struct ifreq ifr_ip;
    ifr_ip.ifr_addr.sa_family = AF_INET;
    strncpy(ifr_ip.ifr_name, "wlp3s0", IFNAMSIZ-1);
    if(ioctl(udp_socket, SIOCGIFADDR, &ifr_ip) == -1)
    {
        perror("Getting ip address");
        exit(EXIT_FAILURE);
    }
    struct in_addr ip_addr = ((struct sockaddr_in *)&ifr_ip.ifr_addr)->sin_addr;

    //get broadcast address
    struct ifreq ifr_brd;
    ifr_brd.ifr_addr.sa_family = AF_INET;
    strncpy(ifr_brd.ifr_name, "wlp3s0", IFNAMSIZ-1);
    if(ioctl(udp_socket, SIOCGIFBRDADDR, &ifr_brd) == -1)
    {
        perror("Getting broadcast address:");
        exit(EXIT_FAILURE);
    }


    struct in_addr brd_addr = ((struct sockaddr_in *)&ifr_brd.ifr_addr)->sin_addr;

    struct sockaddr_in sock_in;
    sock_in.sin_addr.s_addr = ip_addr.s_addr;
    sock_in.sin_port = htons(port);
    sock_in.sin_family = AF_INET;

    //bind
    if(bind(udp_socket, (struct sockaddr*)&sock_in, sizeof(struct sockaddr)) == -1)
    {
        perror("UDP client socket binding");
        exit(EXIT_FAILURE);
    }

    int enable_brd = 1;
    if(setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &enable_brd, sizeof(int)) == -1)
    {
        perror("UDP client socket broadcast enabling");
        exit(EXIT_FAILURE);
    }

    sock_in.sin_addr.s_addr = brd_addr.s_addr;
    sock_in.sin_port = htons(port);
    sock_in.sin_family = AF_INET;

    if(sendto(udp_socket, &ip_addr, sizeof(struct in_addr), 0,
        (struct sockaddr *)&sock_in, sizeof(struct sockaddr_in)) == -1)
    {
        perror("Broadcast message sendto:");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Broadcast message sent.\n");
    close(udp_socket);

    //tcp connection
    int tcp_sock = socket(AF_INET , SOCK_STREAM, 0);
    if(tcp_sock == -1)
    {
        perror("TCP client socket creation");
        exit(EXIT_FAILURE);
    }

    //set reuse and keep alive options
    int enable_reuse = 1;
    int keepalive = 1;
    int keepcnt = 1;
    int keepidle = 1;
    int keepintvl = 1;

    if(setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) == -1)
    {
        perror("UDP client socket broadcast enabling");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt (tcp_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int)) != 0)
    {
        perror("Unable to set parameters for socket");
        exit(EXIT_FAILURE);
    }

    sock_in.sin_addr.s_addr = ip_addr.s_addr;
    sock_in.sin_port = htons(port);
    sock_in.sin_family = AF_INET;
    if(bind(tcp_sock, (struct sockaddr*)&sock_in, sizeof(struct sockaddr)) == -1)
    {
        perror("TCP client socket binding");
        exit(EXIT_FAILURE);
    }

    listen(tcp_sock, servers_qty);

    int epollfd = epoll_create1(0);
    if(epollfd == -1)
    {
        perror("epoll");
        exit(EXIT_FAILURE);
    }
    /* event.data.ptr will contain a pointer to the correspondent
     * server structure.
     */
    struct epoll_event event;
    struct epoll_event events[64];

    server_struc_t server[servers_qty];
    int threads_total = 0;
    struct sockaddr_in server_sock;
    unsigned int sockaddr_len = sizeof(struct sockaddr);
    /* In order to counter race condition in acception, e.g.
     * when client waits infinitly for server connection
     * alarm is used. If by the time specified in 1st alarm (3 sec)
     * sufficient amount of peers was not achieved, than we perform exit.
     * Logic can be seen in alarm_handler above
     */
    alarm(3);
    for (int i = 0; i < servers_qty; ++i)
    {
        server[i].fd = accept(tcp_sock, (struct sockaddr *)&server_sock, (socklen_t *)&sockaddr_len);
        if(server[i].fd < 0)
        {
            printf("TCP client accept: Connection failed\n");
            exit(EXIT_FAILURE);
        }
        conn_acc++;
    }
    // All connections resolved, set alarm off
    alarm(0);
    fprintf(stderr, "All connections resolved.\n");
    
    for (int i = 0; i < servers_qty; ++i)
    {
        if(recv(server[i].fd, &(server[i].threads_avail), sizeof(int), 0) <= 0)
        {
            printf("TCP client threads_ avail recv: FAILEd\n");
            exit(EXIT_FAILURE);            
        }
        threads_total += server[i].threads_avail;

        //make server[i].fd nonblock
        int flags = fcntl(server[i].fd, F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(server[i].fd, F_SETFL, flags);

        //watch on 
        event.data.ptr = &(server[i]);
        event.events = EPOLLOUT;
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, server[i].fd, &event) == -1)
        {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
    }

    //limits distribution
    double seg_size = (right - left) / threads_total;
    double current_left = left;
    for (int i = 0; i < servers_qty; ++i)
    {
        server[i].limit.left = current_left;
        server[i].limit.right = current_left + server[i].threads_avail * seg_size;
        current_left = server[i].limit.right;
    }

    #ifdef NET_DEBUG
    printf("server struct info:\n");
    for (int i = 0; i < servers_qty; ++i)
    {
        printf("server[%d].fd = %d\n", i, server[i].fd);
        printf("server[%d].threads_avail = %d\n", i, server[i].threads_avail);
        printf("server[%d].limit.left = %g\n", i, server[i].limit.left);
        printf("server[%d].limit.right = %g\n", i, server[i].limit.right);
    }
    #endif
    fprintf(stderr, "All limits calculated. Starting transmission routine.\n");
    //info exchange
    int sync;
    int finished = 0;
    double temp_res = 0;
    double res = 0;
    while(1)
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
                int fd = ((server_struc_t *)events[j].data.ptr)->fd;
                limits_t limits = ((server_struc_t *)events[j].data.ptr)->limit;
                if(send(fd, &(limits), sizeof(limits_t), 0) < 0)
                {
                    printf("TCP client limits send: unsuccessful. Aborting...\n");
                    exit(EXIT_FAILURE);
                }

                //change poll target to EPOLLIN
                event.data.ptr = events[j].data.ptr;
                event.events = EPOLLIN;
                if(epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event) == -1)
                {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }

            }
            else if(events[j].events & EPOLLIN)
            {
                int fd = ((server_struc_t *)events[j].data.ptr)->fd;
                if(recv(fd, &(temp_res), sizeof(double), 0) <= 0)
                {
                    printf("TCP client results recv: unsuccessful. Aborting...\n");
                    exit(EXIT_FAILURE);            
                }
                res += temp_res;

                if(send(fd, &sync, sizeof(int), 0) < 0)
                {
                    printf("TCP client sync send: unsuccessful. Aborting...\n");
                    exit(EXIT_FAILURE);
                }

                //All desired operations were performed, unwatch this fd.
                if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event) == -1)
                {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }
                finished++;
            }
        }
        if(finished == servers_qty)
            break;
    }

    printf("RESULT :: %g\n", res);
    close(tcp_sock);

    return 0;
}