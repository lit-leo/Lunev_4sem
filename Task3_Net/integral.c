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
//#define INTEGRAL_DBG

typedef struct calc_structure
{
    double low_limit;
    double high_limit;
    double step;
    double result;
    //int blank_space[2048];

} calc_structure_t;

typedef struct core_cpus
{
    int cpu[2];
} core_cpus_t;

void *integrate(void *this)
{
    double a = ((calc_structure_t*)this)->low_limit;
    double b = ((calc_structure_t*)this)->high_limit;
    double step = ((calc_structure_t*)this)->step;
    double result = 0;
    double left = a;
    double right = a;
    while(right < b)
    {
        right += step;
        result += simps(left, right);
        left += step;
    }
    ((calc_structure_t*)this)->result = result;

    return NULL;
}

void *idle()
{
    while(1);
    return NULL;
}

int fillCoreStruct(int cpu_no, core_cpus_t *table)
{
    char path[64];
    char path_beginning[28] = "/sys/devices/system/cpu/cpu";
    char path_ending[18] = "/topology/core_id";
    sprintf(path, "%s%d%s", path_beginning, cpu_no, path_ending);
    
    #ifdef INTEGRAL_DBG
    printf("%s - %s\n", __FUNCTION__, path);
    #endif
    
    FILE* core_f = NULL;
    core_f = fopen(path, "r");
    if(core_f == NULL)
    {
        perror(__FUNCTION__);
        exit(-1);
    }

    int core_no;
    fscanf(core_f, "%d", &core_no);
    if(table[core_no].cpu[0] == -1)
        table[core_no].cpu[0] = cpu_no;
    else
        table[core_no].cpu[1] = cpu_no;
    fclose(core_f);


    return 0;
}

int nextFirstLineCpu(unsigned int *counter, core_cpus_t *tbl, int cpus_conf)
{
    /*pick vacant first-line cpu*/
    (*counter)++;
    while(tbl[*counter % cpus_conf].cpu[0] == -1)
        (*counter)++;
    return tbl[*counter % cpus_conf].cpu[0];
}

int nextSecondLineCpu(unsigned int *counter, core_cpus_t *tbl, int cpus_conf)
{
    /*pick vacant second-line cpu*/
    (*counter)++;
    while(tbl[*counter % cpus_conf].cpu[1] == -1)
        (*counter)++;
    return tbl[*counter % cpus_conf].cpu[1];
}

double calculate_integral(int thrds_qty, double left, double right)
{
    long threads_req = thrds_qty;

    double res = 0;
    double a = left;
    double b = right;
    double step = (double)20 / (double)9000000000;//(b - a)/3000000000; //1000000000
    double range = (b - a) / threads_req;

    calc_structure_t calc_struc[threads_req];
    for (int i = 0; i < threads_req; ++i)
    {
        calc_struc[i].low_limit = a + range * i;
        calc_struc[i].high_limit = calc_struc[i].low_limit + range;
        calc_struc[i].step = step;
        calc_struc[i].result = 0;
    }

    long cpus_conf = sysconf(_SC_NPROCESSORS_CONF);
    #ifdef INTEGRAL_DBG
    printf("cpus_conf = %ld\n", cpus_conf);
    #endif

    core_cpus_t cpus_tbl[cpus_conf];
    unsigned int line1_cur = -1;
    unsigned int line2_cur = -1;
    int line1_total = 0;
    int line2_total = 0;
    for (int i = 0; i < cpus_conf; ++i)
    {
        cpus_tbl[i].cpu[0] = -1;
        cpus_tbl[i].cpu[1] = -1;
    }


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
                fillCoreStruct(first, cpus_tbl);
                break;

            case '-' :
                fscanf(online, "%d%c", &second, &buf);
                for (int i = first; i <= second; ++i)
                    fillCoreStruct(i, cpus_tbl);
                break;

            case '\n' :
                fillCoreStruct(first, cpus_tbl);
                break;

            default :
                printf("Unknown symbol met\n");
        }

    int threads_tb = 0; //anti-turbo-boost threads
    for (int i = 0; i < cpus_conf; ++i)
    {    
        if(cpus_tbl[i].cpu[0] != -1)
        {
            threads_tb++;
            line1_total++;
        }
        if(cpus_tbl[i].cpu[1] != -1)
            line2_total++;
    }

    #ifdef INTEGRAL_DBG
    for (int i = 0; i < cpus_conf; ++i)
        printf("Core %d: %d, %d\n", i, cpus_tbl[i].cpu[0], cpus_tbl[i].cpu[1]);
    printf("\n");
    printf("threads_tb = %d\n", threads_tb);    
    #endif

    int threads_involved;
    if(threads_req > threads_tb)
        threads_involved = threads_req;
    else
        threads_involved = threads_tb;

    pthread_t threads[threads_involved];
    threads[0] = pthread_self();

    cpu_set_t cpu;
    int cpu_tb = nextFirstLineCpu(&line1_cur, cpus_tbl, cpus_conf);
    CPU_ZERO(&cpu);
    CPU_SET(cpu_tb, &cpu);
    if(pthread_setaffinity_np(threads[0], sizeof(cpu_set_t), &cpu) != 0)
    {
        perror("main thread pthread_setaffinity_np:");
        return -1;
    }
    #ifdef INTEGRAL_DBG
    printf("Obtained %d thread on %d core\n", cpu_tb, line1_cur % (int)cpus_conf);
    #endif

    //anti-turboo-boost distribution
    int i = 1;
    for (; i < threads_tb; ++i)
    {
        void *thread_data;
        if(i < threads_req)
            thread_data = (void*)&calc_struc[i];
        else
            thread_data = (void*)&calc_struc[0];

        if(pthread_create(&(threads[i]), NULL, integrate, thread_data))
        {
            perror("anti-turbo-boost thread creation");
            exit(-1);
        }

        int cpu_tb = nextFirstLineCpu(&line1_cur, cpus_tbl, cpus_conf);
        CPU_ZERO(&cpu);
        CPU_SET(cpu_tb, &cpu);
        if(pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpu))
        {
            perror("anti-turbo-boost thread setting affinity");
            exit(-1);
        }
        #ifdef INTEGRAL_DBG
        printf("Obtained %d thread on %d core\n", cpu_tb, line1_cur % (int)cpus_conf);
        #endif
    }

    while(i < threads_req)
    {
        for (int j = 0; j < line2_total && i < threads_req; ++j)
        {
            void *thread_data = (void*)&calc_struc[i];
            int cpu_calc = nextSecondLineCpu(&line2_cur, cpus_tbl, cpus_conf);
            CPU_ZERO(&cpu);
            CPU_SET(cpu_calc, &cpu);

            if(pthread_create(&(threads[i]), NULL, integrate, thread_data))
            {
                perror("calculating thread creation");
                exit(-1);
            }
            if(pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpu))
            {
                perror("calculating thread setting affinity");
                exit(-1);
            }
            i++;
            #ifdef INTEGRAL_DBG
            printf("Obtained %d thread on %d core\n", cpu_calc, line2_cur % (int)cpus_conf);
            #endif
        }
        for (int j = 0; j < line1_total && i < threads_req; ++j)
        {
            void *thread_data = (void*)&calc_struc[i];
            int cpu_calc = nextFirstLineCpu(&line1_cur, cpus_tbl, cpus_conf);
            CPU_ZERO(&cpu);
            CPU_SET(cpu_calc, &cpu);

            if(pthread_create(&(threads[i]), NULL, integrate, thread_data))
            {
                perror("calculating thread creation");
                exit(-1);
            }
            if(pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpu))
            {
                perror("calculating thread setting affinity");
                exit(-1);
            }
            i++;
            #ifdef INTEGRAL_DBG
            printf("Obtained %d thread on %d core\n", cpu_calc, line1_cur % (int)cpus_conf);
            #endif
        }
    }

    #ifdef INTEGRAL_DBG
    printf("Obtaining: DONE\n");
    #endif
    //calculations in main thread

    integrate((void*)&calc_struc[0]);
    res+=calc_struc[0].result;

    i = 1;
    for (; i < threads_req; ++i)
    {
        if(pthread_join(threads[i], NULL))
        {
            perror("pthread_join");
            exit(-1);
        }
        res += calc_struc[i].result;
    }

    for (; i < threads_tb; ++i)
        pthread_cancel(threads[i]);

    return res;
}