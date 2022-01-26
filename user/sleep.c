# include "kernel/types.h"
# include "kernel/stat.h"
# include "user/user.h"

int main(int argc, char *agrv[])
{
    int tick;
    int flag;

    if (argc < 2)
    {
        fprintf(2, "Usage: sleep arg\n");
        exit(1);
    }
    
    tick = atoi(agrv[1]);
    flag = sleep(tick);
    exit(flag);
}