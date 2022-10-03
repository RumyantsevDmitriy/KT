#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char** StringParcer (char* str, size_t str_size, char** cmd, size_t* my_cmd_size)
{
    size_t cmd_size = 1;
    cmd = (char**) calloc (cmd_size, sizeof(char**));

    cmd[0] = str;
    for (size_t i = 0, j = 1; i < str_size; i++)
    {
        if (str[i] == '|' && i != (str_size - 1))
        {
            if(str[i + 1] != ' ')
            {
                str[i] = '\0';
                cmd_size++;
                cmd = (char**) realloc (cmd, cmd_size*sizeof(char**));
                cmd[j] = str + i + 1;
                j++;
            }
            else
            {
                str[i] = '\0';
                cmd_size++;
                cmd = (char**) realloc (cmd, cmd_size*sizeof(char**));
                cmd[j] = str + i + 2;
                j++;
            }
        }
        else if (str[i] == '|' && i == (str_size - 1))
            str[i] = '\0';
    }
    if (str[str_size - 1] == '\n')
        str[str_size - 1] = '\0';

    *my_cmd_size = cmd_size;

    return cmd;
}

char** CommandParcer (char** cmd, size_t cmd_size, char** pcmd, size_t* pcmd_size, char* str)
{
    size_t str_size = 0;
    int k = 0;
    char c = 0;
    int flag = 0;

    for (size_t i = 0; i < cmd_size; i++)
    {
        flag = 0;
        c = cmd[i][0];
        for (size_t j = 0; c != '\0'; )
        {
            str_size++;
            str = (char*) realloc (str, str_size*sizeof(char));
            if (c == ' ')
            {   
                str[k] = '|';
                k++;
                flag = 1;
                break;
            }
            str[k] = c;
            k++;
            j++;
            c = cmd[i][j];
        }
        if (flag == 0)
        {
            str[k] = '|';
            k++;
            str_size++;
        }
    }

    pcmd = StringParcer(str, str_size, pcmd, pcmd_size);

    return pcmd;
}

void seq_pipe(char **cmd, size_t cmd_size, char **pcmd)
{
    int   p[2];
    pid_t pid;
    int   fd_in = 0;

    for (size_t i = 0; i < cmd_size; ) 
    {
        pipe(p);
        printf("New used pipe descriptors: %d %d\n",p[0],p[1]);
        printf("Input descriptor for current child process: %d\n", fd_in);
        if ((pid = fork()) == -1) 
        {
            printf("Error");
            exit(1);
        } 
        else if (pid == 0) 
        {
            if (i > 0)
            dup2(fd_in, 0);
            if (i != (cmd_size - 1))
            dup2(p[1], 1);
            close(p[0]);
            (void) execlp(pcmd[i], cmd[i], NULL);
            printf("Error in child's exec\n");
            exit(2);
        }
        else 
        {
            wait(NULL);
            close(p[1]);
            if (i > 0)
                close(fd_in);
            fd_in = p[0];
            i++;
        }
    }
}

int main()
{
    char   *str      = NULL;
    char   *st       = NULL;
    char   **cmd     = NULL;
    char   **pcmd    = NULL;
    char   c         = 0;
    int    str_size  = 0;
    size_t cmd_size  = 0;
    size_t pcmd_size = 0;
    size_t i         = 0;

    str = (char*) calloc (4096, sizeof(char));
    str_size = sizeof(str);

    while (1)
    {
        printf ("Enter commands: ");
        for (i = 0; (c = getchar ()) != '\n'; i++)
        {
            if (i < str_size)
                str[i] = c;
            else
            {
                str_size++;
                str = (char*) realloc (str, (str_size * sizeof(char)));
                str[i] = c;
            }
        }
        if (i > str_size)
        {   
            str_size++;
            str = (char*) realloc (str, (str_size * sizeof(char)));
            str[i] = '\0';
        }
        str_size = strlen(str);
 
        cmd = StringParcer(str, strlen(str), cmd, &cmd_size);
        assert(cmd != NULL);
        pcmd = CommandParcer(cmd, cmd_size, pcmd, &pcmd_size, st);
        assert(pcmd != NULL);

        seq_pipe(cmd, cmd_size, pcmd);
    }

    return 0;
}