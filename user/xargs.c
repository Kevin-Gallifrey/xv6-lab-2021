# include "kernel/types.h"
# include "user/user.h"
# include "kernel/param.h"

int main(int argc, char *argv[])
{
    if (argc > MAXARG)
    {
        fprintf(2, "xarg: too many arguments.\n");
        exit(1);
    }

    char *xargv[MAXARG];
    for (int i = 0; i < argc - 1; i++)
        xargv[i] = argv[i+1];

    int xargc = argc - 1;
    int wordBegin = 0, wordEnd = 0;
    char line[512];
    char *p = line;
    while (read(0, p, sizeof(char)) > 0)
    {
        if (*p == ' ')
        {
            *p = 0;
            xargv[xargc] = &line[wordBegin];
            wordBegin = wordEnd + 1;
            xargc++;
            if (xargc > MAXARG)
            {
                fprintf(2, "xarg: too many arguments.\n");
                exit(1);
            }
            p++;
            wordEnd++;
        }
        // finish one line and exec
        else if (*p == '\n')
        {
            *p = 0;
            xargv[xargc] = &line[wordBegin];
            if (fork() == 0)
                exec(argv[1], xargv);
            else
            {
                wait(0);
                xargc = argc - 1;
                p = line;
                wordBegin = 0;
                wordEnd = 0;
            }
        }
        else
        {
            p++;
            wordEnd++;
        }
    }

    exit(0);
}