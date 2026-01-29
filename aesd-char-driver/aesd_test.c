/*
  Test the aesd-char-driver implementation
 */

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "aesd_ioctl.h"

#define STR_1 "write1\nwrite2\nwrite3\nwrite4\nwrite5\nwrite6\nwrite7\nwrite8\nwrite9\nwrite10\nwrite11\nwrite12\n"
#define STR_2 "hello\nthere\n"

#define DEV_AESDCHAR "/dev/aesdchar"

char buffer[1024];

int test_8()
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

int test_9()
{
    int fout, rc, fin;
    struct aesd_seekto pos;

    fout = open(DEV_AESDCHAR, O_WRONLY);
    if (fout < 0) {
        perror(DEV_AESDCHAR);
        return 1;
    }

    rc = ioctl(fout, AESDCHAR_IOCSEEKTO, 0);
    if (rc < 0) { /* expected error */
        perror("Case 1: " DEV_AESDCHAR);
        /* close(fout); */
        /* return 1; */
    }

    memset(&pos, 0, sizeof(pos));
    pos.write_cmd = 1;
    pos.write_cmd_offset = 123;

    rc = ioctl(fout, AESDCHAR_IOCSEEKTO, &pos);
    if (rc < 0) {
        perror("Case 2: " DEV_AESDCHAR);
        close(fout);
        return 1;
    }
    printf("Case 2: rc == %d\n", rc);

    pos.write_cmd = 2; // END
    pos.write_cmd_offset = 0;

    rc = ioctl(fout, AESDCHAR_IOCSEEKTO, &pos);
    if (rc < 0) {
        perror("case 3: " DEV_AESDCHAR);
        close(fout);
        return 1;
    }
    printf("Case 3: rc == %d\n", rc);

    rc = write(fout, STR_1, strlen(STR_1));

    pos.write_cmd = 2; // END
    pos.write_cmd_offset = 0;

    rc = ioctl(fout, AESDCHAR_IOCSEEKTO, &pos);
    if (rc < 0) {
        perror("case 4: " DEV_AESDCHAR);
        close(fout);
        return 1;
    }
    printf("Case 4: rc == %d\n", rc);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        if(strcmp("test_8", argv[1]) == 0)
            return test_8();
        if(strcmp("test_9", argv[1]) == 0)
            return test_9();
    }
    fprintf(stderr, "Usage: %s {test_8|test_9}\n", argv[0]);
    return 0;
}

/*
Local Variables:
compile-command: "make aesd_test"
End:
*/
