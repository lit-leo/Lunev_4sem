#define _GNU_SOURCE

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#define f(a) a * a * a * a + 2 * a * a * a - a
#define rect(a, b) (f(a) * (b - a))
#define simps(a, b) ((b - a) / 6 * (f(a) + 4 * f((a + b) / 2) + f(b)))

typedef struct calc_structure
{

} calc_structure_t;

void integrate(calc_structure_t *this);


void *idle()
{
    printf("AXAXAXAXAX\n");
    while(1);
    return NULL;
}
int main(int argc, char **argv)
{
    pthread_t threads[7];
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu);
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu);
    pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu);
    for (int i = 0; i < CPU_COUNT(&cpu); ++i)
    {
        printf("CPU%d - %d\n", i, CPU_ISSET(i, &cpu));
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
    double b = 3;
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