#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>
void main()
{
    char cmd[100];
    char *excfile;
    char *temp;
    // char args[100][100];
    while(1)
    {
        int k=0;

        printf("shell>");
        // fgets(cmd,100,stdin);
        // printf("%s\n",cmd);
        char *args[100];
        
        int i=0,index=0;
        gets(cmd);

        temp = strtok(cmd, " ");
        args[i]=temp;
        i++;
        excfile=temp;
        // printf("%s ",excfile);
        // printf("dsf");
        temp = strtok(NULL, " ");
        while (temp!= NULL) {
            
            args[i]=temp;
            i++;
            temp = strtok(NULL, " ");
        }
        args[i]=NULL;
        // if(i==0)
        // args[0]=NULL;
        // printf("%d",i);
        // k=0;
        // while(args[k])
        // {
        //     printf("%s  ",args[k]);
        //     k++;
        // }
        if(strcmp("exit",excfile)==0)
        {
            printf("exiting\n");
            exit(1);
        }
        
        if(strcmp("cd",excfile)==0)
        {
            if(i>1)
            {
                if (chdir(args[1]) != 0) {
                perror("chdir");
                exit(EXIT_FAILURE);
                }
            }
            else
            {
                printf("please provide path\n");
            }
            excfile = "pwd";
        }
       
        int childid = fork();
        if(childid==-1)
        {
            perror("fork failed");
            exit(1);
        }
        if(childid==0)
        {
            if(execvp(excfile,args)==-1)
            {
                perror("execvp failed");
                exit(1);
            }
        }
        else
        {
            int status;
            wait(&status);
        }
    
    }
}
