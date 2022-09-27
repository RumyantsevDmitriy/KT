#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct pPipe Pipe;
typedef struct op_table Ops;

typedef struct op_table  
{
    size_t (*rcv)(Pipe *self); 
    size_t (*snd)(Pipe *self); 
} Ops;

typedef struct pPipe 
{
        char* data;
        int fd_direct[2];
        int fd_back[2]; 
        size_t len;
        Ops actions;
} Pipe;

size_t FileSize (char* file_name)
{
    int fd;
    struct stat statbuf;
    umask(0);
    if((fd = open(file_name, O_RDONLY)) < 0)
    {
        printf("Can't open file \"%s\"\n", file_name);
        exit(-1);
    }
    if(fstat(fd, &statbuf) < 0)
    {
        printf("Problem with file \"%s\"\n", file_name);
        exit(-1);
    }
    if(close(fd) < 0)
    {
        printf("Can't close file \"%s\"", file_name);
        exit(-1);
    }

    return statbuf.st_size;
}

int main()
{
    int fork_status;
    int flag;
    char *buf;
    buf = (char*) calloc(0, sizeof(char));
    int fd;
    size_t file_size;
    char out_file_name[] = "data_file.txt", in_file_name[] = "data_file1.txt";
    Pipe myPipe = {};
    myPipe.len = FileSize(out_file_name);

    if(pipe(myPipe.fd_direct) < 0)
    {
        printf("Can't create parent pipe\n");
        exit(-1);
    }
    if(pipe(myPipe.fd_back) < 0)
    {
        printf("Can't create child pipe\n");
        exit(-1);
    }
    fork_status = fork();
    if(fork_status < 0)
    {
        printf("Can't make fork\n");
        exit(-1);
    }
    else if(fork_status == 0)
    {
        printf("I am a child\n");
        close(myPipe.fd_direct[1]);
        buf = (char*) realloc(buf, myPipe.len*sizeof(char));
        myPipe.data = buf;
        flag = read(myPipe.fd_direct[0], myPipe.data, myPipe.len);
        if(flag < 0)
        {
            printf("Can't read string\n");
            exit(-1);
        }
        close(myPipe.fd_direct[0]);
        
        close(myPipe.fd_back[0]);
        file_size = write(myPipe.fd_back[1], myPipe.data, myPipe.len);
        if(file_size != myPipe.len)
        {
            printf("Can't write all string\n");
            exit(-1);
        }
        close(myPipe.fd_back[1]);
        free(buf);
        printf("Child exit\n");
    }
    else
    {
        printf("I am a parent\n");
        umask(0);
        if((fd = open(out_file_name, O_RDONLY)) < 0)
        {
            printf("Can't open file \"%s\"\n", out_file_name);
            exit(-1);
        }       
        buf = (char*) realloc(buf, myPipe.len*sizeof(char));
        flag = read(fd, buf, myPipe.len);
        if(flag < 0)
        {
            printf("Can't read file \"%s\"\n", out_file_name);
            exit(-1);
        }
        if(close(fd) < 0)
        {
            printf("Can't close file \"%s\"\n", out_file_name);
            exit(-1);
        }   
        myPipe.data = buf;

        close(myPipe.fd_direct[0]);
        file_size= write(myPipe.fd_direct[1], myPipe.data, myPipe.len);
        if(file_size != myPipe.len)
        {
            printf("Can't write all string\n");
            exit(-1);
        }
        close(myPipe.fd_direct[1]);
        free(buf);

        close(myPipe.fd_back[1]);
        buf = (char*) realloc(buf, myPipe.len*sizeof(char));
        myPipe.data = buf;
        file_size = read(myPipe.fd_back[0], myPipe.data, myPipe.len);
        if(file_size < 0)
        {
            printf("Can't read string\n");
            exit(-1);
        }
        close(myPipe.fd_back[0]);

        umask(0);
        if((fd = open(in_file_name, O_WRONLY | O_CREAT, 0777)) < 0)
        {
            printf("Can't open file \"%s\"\n", in_file_name);
            exit(-1);
        }
        file_size = write(fd, myPipe.data, myPipe.len);
        if(file_size != myPipe.len)
        {
            printf("Can't write all string in file \"%s\"\n", in_file_name);
            exit(-1);
        }
        if(close(fd) < 0)
        {
            printf("Can't close file \"%s\"", in_file_name);
            exit(-1);
        }
        free(buf);
        printf("Parent exit\n");
    }

    return 0;
}

