# include "kernel/types.h"
# include "kernel/stat.h"
# include "user/user.h"

int main(int agrc, char *argv[])
{
    int p1[2], p2[2];
    char buf[1];
    pipe(p1);
    pipe(p2);

    if (fork() == 0)
    {
        close(p1[1]);
        read(p1[0], buf, 1);
        close(p1[0]);
        printf("%d: received ping\n", getpid());
        // printf("%s\n", buf);
        close(p2[0]);
        write(p2[1], buf, 1);
        close(p2[1]);
        exit(0);
    }
    else
    {
        buf[0] = 'a';
        close(p1[0]);
        write(p1[1], buf, 1);
        close(p1[1]);
        wait(0);
        close(p2[1]);
        read(p2[0], buf, 1);
        printf("%d: received pong\n", getpid());
        close(p2[0]);
        exit(0);
    }
    exit(0);
}