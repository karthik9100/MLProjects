//this is final shell code 

#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include<string.h>
void main()
{
    using_history();
    char *cmds[100];
    char *subinput;
    char *temp;
     // input may contain multiple commands which pipe separated so we need to prepared 
    //
    // printf("%s",input);
    char cmd[100];
    char *excfile;
    int is_ampersand=0;
    // char args[100][100];
    while(1)
    {

        int k=0;
        char input[200];
        input[0]='\0';

        // printf("shell>");
        int flag=1;
        int cont_read=0;
        
        while(flag)
        {
            // gets(subinput);
            if(cont_read)
                subinput = readline("");
            else
                subinput = readline("shell>");
            if(strlen(subinput)==0)
            {
                continue;
            }
            // printf("%c",subinput[0]);
            if(subinput[0]=='&')
            {
                flag=0;
                is_ampersand=1;
                continue;
            }
            // printf("%ld ",strlen(subinput));
            if(subinput[strlen(subinput)-1]=='\\')
            {
                subinput[strlen(subinput)-1]=' ';
                strcat(input,subinput);
                cont_read = 1;
            }
            else
            {
                flag=0;
                cont_read=0;
                strcat(input,subinput);
            }
        }
        // printf("\n%s\n",input);
        if(is_ampersand)
        {
            is_ampersand=0;
            continue;
        }
        if (strlen(input) > 0)
        {
            add_history(input);
        }
        //preprocess the input and splits input into multiple commands based the | symbol
        int cmds_i=0;
        temp = strtok(input, "|");
        cmds[cmds_i]=temp;
        cmds_i++;
        temp=strtok(NULL,"|");
        // printf("%s",temp);
        while(temp!=NULL)
        {
            cmds[cmds_i]=temp;
            temp=strtok(NULL,"|");
            cmds_i++;
        }
        // cmds[cmds_i]=NULL;
        // for(int k=0;k<cmds_i;k++)
        // printf("%s\n",cmds[k]);


       
        char *args[100];
        int index=0;
        // creating pipes each for each command
        int p[cmds_i-1][2];
        pid_t childid[cmds_i];
        for(int i=0;i<cmds_i-1;i++)
        {
            if(pipe(p[i])<0)
            {
                perror("error in creating a pipe\n");
                exit(1);
            }
        }
        
        for(int cid=0;cid<cmds_i;cid++)
        {
            
            // printf("..%d..",cid);    
            int i=0;

            temp = strtok(cmds[cid], " ");
            
            args[i]=temp;
            
            excfile=temp;
            if(strcmp(excfile,"addvec")==0 || strcmp(excfile,"subvec")==0 || strcmp(excfile,"dotprod")==0 || strcmp(excfile,"ncurses")==0)
            {
                    char c[100] = "./";
                    strcat(c,excfile);
                    excfile = c;
                    // printf("%s",excfile  );
            }
            args[i]=excfile;
            i++;
            // printf("%s ...",excfile);
            // printf("dsf");
            temp = strtok(NULL, " ");
            while (temp!= NULL) {
                
                args[i]=temp;
                i++;
                temp = strtok(NULL, " ");
            }
            args[i]=NULL;

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
        
            childid[cid] = fork();
            if(childid[cid]==-1)
            {
                perror("fork failed");
                exit(1);
            }
            
            if(childid[cid]==0)
            {
                //  printf("In child");
                 
                 if(cid>0)
                 {
                    close(p[cid-1][1]);
                    dup2(p[cid-1][0],0);
                    close(p[cid-1][0]);
                 }
                 if(cid<cmds_i-1)
                 {
                    close(p[cid][0]);
                    dup2(p[cid][1],1);
                    close(p[cid][1]);
                 }
                
                if(execvp(excfile,args)==-1)
                {
                    perror("execvp failed");
                    exit(1);
                }
                
            }
            
        }
        for(int i=0;i<cmds_i-1;i++)
        {
            close(p[i][0]);
            close(p[i][1]);
        }
         for(int i=0;i<cmds_i;i++)
        {
            waitpid(childid[i],NULL,0);
        }
                
    }
    
}
