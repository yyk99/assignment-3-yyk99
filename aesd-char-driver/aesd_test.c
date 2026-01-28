/*
  Test the aesd-char-driver implementation
 */

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define STR_1 "write1\nwrite2\nwrite3\nwrite4\nwrite5\nwrite6\nwrite7\nwrite8\nwrite9\nwrite10\nwrite11\nwrite12\n"
#define STR_2 "hello\nthere\n"

#define DEV_AESDCHAR "/dev/aesdchar"

char buffer[1024];

int main()
{
    /* step 1. Write */
    int fout, rc, fin;

    fout = open(DEV_AESDCHAR, O_WRONLY);
    if (fout < 0) {
        perror(DEV_AESDCHAR);
        return 1;
    }

    rc = write(fout, STR_1, strlen(STR_1));
    fprintf(stderr, "rc == %d\n", rc);
    close(fout);

    fin = open(DEV_AESDCHAR, O_RDONLY);
    if(fin < 0) {
        perror(DEV_AESDCHAR);
        return 1;
    }

    do {
        rc = read(fin, buffer, sizeof(buffer));
        if (rc < 0) {
            perror("read from " DEV_AESDCHAR);
            return 1;
        }
        fprintf(stderr, "read(...) = %d\n", rc);
        if (rc == 0)
            break;
        fwrite(buffer, 1, rc, stdout);
    } while(1);

    close(fin);

    return 0;
}

/*
Local Variables:
compile-command: "make aesd_test"
End:
*/
