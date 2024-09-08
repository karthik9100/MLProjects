#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
struct arguments
{
    int *vector1;
    int *vector2;
    int start;
    int end;
    int *result;
};
#define MAXSIZE 200
int vector1size = 0;
int vector2size = 0;
int Readfile(FILE *ptr, int vector[])
{
    int num;
    int index=0;
    while(fscanf(ptr,"%d",&num)!=EOF)
    {
        vector[index++] = num;

    }
    return index;
}
// void addVectors(int vector1[],int vector2[], int start,int end, int result[])
void *mulVectors(void *data)
{
    struct arguments *args = (struct arguments*)data;
    //printf("%d\n",args[0].start);

    for(int i=args->start;i<args->end;i++)
    {
        
        args->result[i] = args->vector1[i]*args->vector2[i];
        // printf("%d * %d = %d\n",args->vector1[i],args->vector2[i],args->result[i]);
    }
    return NULL;
}
int main(int argc, char **argv)
{

    if(argc!=4 && argc!=3)
    {
        printf("please enter <file 1> <file 2> -<no_threads>\n");
        exit(1);
    }
    char *file1 = argv[1];
    char *file2 = argv[2];
    int n_threads = 0;
    if(argc==4)
    {

        for(int i=0;i<strlen(argv[3])-1;i++)
        {
            n_threads = n_threads*10+argv[3][i+1]-48;
        }
    }
    else
        n_threads=3;
    // printf("%d",n_threads);
    FILE *ptr1 = fopen(file1,"r");
    FILE *ptr2 = fopen(file2,"r");
    if(ptr1==NULL)
    {
        printf("Unable to open %s",file1);
    
    }
    if(ptr2==NULL)
    {
        
        printf("Unable to open %s",file2);
    }
    int vector1[MAXSIZE];
    int vector2[MAXSIZE];
    vector1size = Readfile(ptr1,vector1);
    vector2size = Readfile(ptr2,vector2);
    int result[vector1size];
    // int result2[vector2size];
    if(vector1size!=vector2size)
    {
        printf("both files should contain vectors of equal size... exiting program!!!\n");
        exit(1);
    }
    int opsize=0;
    int opsize1 = 0;
    
    //creating thread arguments
   
    struct arguments threadargs[n_threads];
    for(int i=0;i<n_threads;i++)
    {
        threadargs[i].vector1 = malloc(vector1size*sizeof(int));
        memcpy(threadargs[i].vector1, vector1, vector1size * sizeof(int));
        threadargs[i].vector2 = malloc(vector1size*sizeof(int));
        memcpy(threadargs[i].vector2, vector2, vector1size * sizeof(int));
        threadargs[i].result = malloc(vector1size*sizeof(int));
        memcpy(threadargs[i].result, result, vector1size * sizeof(int));
    }
    opsize = vector1size/n_threads;
    opsize1 = vector1size%n_threads;
    for(int i=0,k=0;i<vector1size && k<n_threads;)
    {
        if(opsize1>0)
        {
            threadargs[k].start = i;
            threadargs[k].end = i+opsize+1;
            i=i+opsize+1;
            opsize1--;
            
        }
        else
        {
            threadargs[k].start = i;
            threadargs[k].end = i+opsize;
            i=i+opsize;
        }
        k++;
    }
    if(n_threads>vector1size)
        n_threads = vector1size;
    
    

    // creating threads
    pthread_t thread[n_threads];
    for(int i=0;i<n_threads;i++)
    {
        if(pthread_create(&thread[i],NULL,mulVectors,(void *)&threadargs[i])==0)
        {

        }
        else
        {
            perror("creating thread");
        }
    }
    // addVectors(vector1,vector2, start,end, result1);
    for(int i=0;i<n_threads;i++)
    {
        pthread_join(thread[i],NULL);
    }
    for(int i=0;i<n_threads;i++)
    {
        // printf("%d  %d\n",threadargs[i].start,threadargs[i].end);
        for(int j=threadargs[i].start;j<threadargs[i].end;j++)
        result[j] = threadargs[i].result[j];
    }
    int sum=0;
    for(int i=0;i<vector1size;i++)
    {
        sum+=result[i];
    }
    printf("%d\n",sum);
    for (int i = 0; i < n_threads; i++) {
        free(threadargs[i].vector1) ;
        free(threadargs[i].vector2);
        free(threadargs[i].result);
    }
    fclose(ptr1);
    fclose(ptr2);
}