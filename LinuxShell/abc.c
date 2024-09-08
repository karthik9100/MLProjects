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
    char* line;
    while (1) 
    {
        line = readline( "shell> ");
        if (strlen(line) > 0)
        {
            add_history(line);
        }

        printf("[%s]\n", line);
        // free(buf);
    }
}