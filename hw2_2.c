#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct pPipe Pipe;
typedef struct op_table Ops;
size_t FileSize (const char* file_name);
Pipe ctorPipe();
void p_snd (Pipe *self);
void p_rcv (Pipe *self);
void p_rd (Pipe *self, const char *file_name);
void p_wr (Pipe *self, const char *file_name);
void p_check (Pipe *self);

typedef struct op_table  
{
    void (*rcv)(Pipe *self); 
    void (*snd)(Pipe *self); 
    void (*rd)(Pipe *self, const char *file_name);
    void (*wr)(Pipe *self, const char *file_name);
    void (*check)(Pipe *self);
} Ops;

typedef struct pPipe 
{
    char *buf;
    int fd_direct[2];
    //int fd_back[2]; 
    size_t len;
    Ops actions;
} Pipe;

void p_snd (Pipe *self)
{
    size_t file_size = 0;

    close(self->fd_direct[0]);
    file_size = write(self->fd_direct[1], self->buf, self->len);
    if(file_size != self->len)
    {
        printf("Can't write all string\n");
        exit(-1);
    }
    close(self->fd_direct[1]);
}

void p_rcv (Pipe *self)
{
    size_t file_size = 0;

    close(self->fd_direct[1]);
    self->buf = (char*) realloc(self->buf, (self->len * sizeof(char)));
    file_size = read(self->fd_direct[0], self->buf, self->len);
    if(file_size < 0)
    {
        printf("Can't read string\n");
        exit(-1);
    }
    close(self->fd_direct[0]);
}

void p_rd (Pipe *self, const char *file_name)
{
    int fd   = 0;
    int flag = 0;

    umask(0);
    if((fd = open(file_name, O_RDONLY)) < 0)
    {
        printf("Can't open file \"%s\"\n", file_name);
        exit(-1);
    }  
    self->len = FileSize(file_name); 
    self->buf = (char*) realloc(self->buf, (self->len * sizeof(char)));
    flag = read(fd, self->buf, self->len);
    if(flag < 0)
    {
        printf("Can't read file \"%s\"\n", file_name);
        exit(-1);
    }
    if(close(fd) < 0)
    {
        printf("Can't close file \"%s\"\n", file_name);
        exit(-1);
    }   
}

void p_wr (Pipe *self, const char *file_name)
{
    int    fd        = 0;
    size_t file_size = 0;

    umask(0);
    if((fd = open(file_name, O_WRONLY | O_CREAT, 0777)) < 0)
    {
        printf("Can't open file \"%s\"\n", file_name);
        exit(-1);
    }
    file_size = write(fd, self->buf, self->len);
    if(file_size != self->len)
    {
        printf("Can't write all string in file \"%s\"\n", file_name);
        exit(-1);
    }
    if(close(fd) < 0)
    {
        printf("Can't close file \"%s\"", file_name);
        exit(-1);
    }
}

void p_check (Pipe *self)
{
    if(pipe(self->fd_direct) < 0)
    {
        printf("Can't create pipe\n");
        exit(-1);
    }
}

size_t FileSize (const char* file_name)
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

Pipe ctorPipe ()
{
    Pipe pipe;
    pipe.actions.rcv = p_rcv;
    pipe.actions.snd = p_snd;
    pipe.actions.rd = p_rd;
    pipe.actions.wr = p_wr;
    pipe.actions.check = p_check;

    return pipe;
}

int main()
{
    int fork_status;
    char out_file_name[] = "data_file.txt", in_file_name[] = "data_file1.txt";
    Pipe myPipe1 = ctorPipe();
    Pipe myPipe2 = ctorPipe();

    myPipe1.len = FileSize(out_file_name);
    myPipe2.len = FileSize(out_file_name);
    myPipe1.actions.check(&myPipe1);
    myPipe2.actions.check(&myPipe2);
    fork_status = fork();
    if(fork_status < 0)
    {
        printf("Can't make fork\n");
        exit(-1);
    }
    else if(fork_status == 0)
    {
        printf("I am a child\n");
        myPipe1.actions.rcv(&myPipe1);
        myPipe2.buf = myPipe1.buf;
        myPipe2.actions.snd(&myPipe2);
        printf("Child exit\n");
    }
    else
    {
        printf("I am a parent\n");
        myPipe1.actions.rd(&myPipe1, out_file_name);
        myPipe1.actions.snd(&myPipe1);
        myPipe2.actions.rcv(&myPipe2);
        myPipe2.actions.wr(&myPipe2, in_file_name);
        printf("Parent exit\n");
    }

    return 0;
}
