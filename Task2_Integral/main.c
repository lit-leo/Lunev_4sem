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
#define f(x) x * x/*a * a * a * a + 2 * a * a * a - a*/
#define rect(a, b) (f(a) * (b - a))
#define simps(a, b) ((b - a) / 6 * (f(a) + 4 * f((a + b) / 2) + f(b)))
typedef struct calc_structure
{
    double low_limit;
    double high_limit;

} calc_structure_t;

void integrate(calc_structure_t *this);


void *idle()
{
    printf("AXAXAXAXAX\n");
    while(1);
    return NULL;
}

int getCoreId(int cpu_no, int* cpus_array)
{
    char path[64];
    char path_beginning[28] = "/sys/devices/system/cpu/cpu";
    char path_ending[18] = "/topology/core_id";
    sprintf(path, "%s%d%s", path_beginning, cpu_no, path_ending);
    printf("%s - %s\n", __FUNCTION__, path);

    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: ./command <quantity_of_threads>\n");
        return -1;
    }
    
    char* endptr = NULL;
    errno = 0;  
    long threads_requested = strtol(argv[1], &endptr, 10);
    if (errno != 0)
    {
        perror("Strtol:");
        return errno;
    }
    if (*endptr != '\0')
    {
        printf("2nd arg is not a number\n");
        return -1;
    }

    long cores_online = 0;
    long cpus_online = sysconf(_SC_NPROCESSORS_ONLN);
    long cpu_core_id[16];
    for (int i = 0; i < 16; ++i)
    {
        cpu_core_id[i] = -1;
    }
    /*!!!!! Need to list online cpus, need to parse
     * system directory
     */
    FILE* online = NULL;
    online = fopen("/sys/devices/system/cpu/online", "r");
    if(online == NULL)
    {
        perror("fopen online");
        return -1;
    }
    int first, second;
    char buf;
    while(fscanf(online, "%d%c", &first, &buf) > 0)
        switch(buf)
        {
            case ',' :
                getCoreId(first, (int*)cpu_core_id);
                break;

            case '-' :
                fscanf(online, "%d%c", &second, &buf);
                for (int i = first; i <= second; ++i)
                    getCoreId(i, (int*)cpu_core_id);
                break;

            case '\n' :
                getCoreId(first, (int*)cpu_core_id);
                break;

            default :
                printf("Unknown symbol met\n");
        }
        /*if(buf == ',')
            getCoreId(first, (int*)cpu_core_id);
        else
        {
            if(buf == '-')
                fscanf(online, "%d%c", &second, &buf);
            for (int i = first; i <= second; ++i)
                getCoreId(i, (int*)cpu_core_id);
        }
        
    }*/
    /*fscanf(online, "%d%c", &first, &buf);
    printf("%d||%c\n", first, buf);
    fscanf(online, "%d%c", &first, &buf);
    printf("%d||%c\n", first, buf);
    printf("\n%d\n", fscanf(online, "%c", &buf));
    fclose(online);*/
    for(int i = 0; i < cpus_online; i++)
    {
        char path[64];
        char path_beginning[28] = "/sys/devices/system/cpu/cpu";
        char path_ending[18] = "/topology/core_id";
        sprintf(path, "%s%d%s", path_beginning, i, path_ending);
        printf("%s\n", path);
        int cpufd = open(path, O_RDONLY);
        /*!!!!! Security improv!*/
        long id = 0;
        if(read(cpufd, &id, sizeof(char)) <= 0)
        {
            return -1;
        }
        if(id > cores_online)
            cores_online = id - '0';
        close(cpufd);
    }
    cores_online++;
    printf("%ld\n", cores_online);
    long threads_involved;
    if(threads_requested > cores_online)
        threads_involved = threads_requested;
    else
        threads_involved = cores_online;

    pthread_t threads[threads_involved];
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu);
    if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu) != 0)
    {
        perror("pthread_setaffinity_np");
        return -1;
    }
    
    for (int i = 0; i < 7; ++i)
    {
        CPU_ZERO(&cpu);
        CPU_SET(i + 1, &cpu);

        pthread_create(&(threads[i]), NULL, idle, NULL);
        pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpu);        
    }

    double res = 0;
    double res2 = 0;
    double a = 0;
    double b = 12;
    double left = a;
    double right = a;
    double step = (b - a)/3000000000;
    //for(int i = a; i <= b; i += (b - a)/10)
    while(right < b)
    {
        right += step;
        res += rect(left, right);
        res2 += simps(left, right);
        left += step;
    }
    printf("%g\n", res);
    printf("%g\n", res2);

    return 0;
}