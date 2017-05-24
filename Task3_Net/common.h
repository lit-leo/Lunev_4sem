#ifndef INET_INTEGRAL
#define INET_INTEGRAL
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#define f(x) x * x * x * 4 + x * x
#define simps(a, b) ((b - a) / 6 * (f(a) + 4 * f((a + b) / 2) + f(b)))

#endif