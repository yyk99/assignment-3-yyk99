/*
- One difference from the write.sh instructions in Assignment 1:  
  You do not need to make your "writer" utility create directories 
  which do not exist.  You can assume the directory is created by the caller.

- Setup syslog logging for your utility using the LOG_USER facility.

- Use the syslog capability to write a message “Writing <string> to <file>” 
  where <string> is the text string written to file (second argument) and 
  <file> is the file created by the script.  This should be written with LOG_DEBUG level.

- Use the syslog capability to log any unexpected errors with LOG_ERR level.

*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

void usage(const char *me)
{
  fprintf(stderr, "Usage: %s writefile writestr\n", me);
}

int main(int argc, char **argv)
{
  if (argc != 3){
    usage(argv[0]);
    return 1;
  }

  openlog("writer", LOG_PID, LOG_USER); 
  
  FILE *fo = fopen(argv[1], "w");
  if (!fo) {
    syslog(LOG_ERR, "%s: %s(%d)", argv[1], strerror(errno), errno);
    return 1;	   
  }

  syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]); 
  int rc = fputs(argv[2], fo);
  if (rc < 0) {
    syslog(LOG_ERR, "write to file failed: %s(%d)", strerror(errno), errno);
    fclose(fo);
    return 1;
  }
  fclose(fo);
  
  return 0;
}
