# include "kernel/types.h"
# include "kernel/stat.h"
# include "user/user.h"

void primes(int *lp)
{
    close(lp[1]);

    int temp;
    if (read(lp[0], &temp, 4) == 0)
    {
        close(lp[0]);
        exit(0);
    }
    else
    {
        int p[2];
        int buf;
        pipe(p);
        if (fork() == 0)
        {
            primes(p);
        }
        else
        {
            close(p[0]);
            printf("prime %d\n", temp);
            while (read(lp[0], &buf, 4) > 0)
            {
                if (buf % temp != 0)
                {
                    write(p[1], &buf, 4);
                }
            }
            close(lp[0]);
            close(p[1]);
            wait(0);
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    if (fork() == 0)
        primes(p);
    else
    {
        close(p[0]);

        for (int i = 2; i <= 35; i++)
        {
            write(p[1], &i, 4);
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    exit(0);
}