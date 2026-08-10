/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief This program reads from /dev/log/ and saves the raw data
 * to file. This file can then be used to test idd-logreader.
 *
 * @author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>


#define LOGGER_ENTRY_MAX_LEN		(4*1024)
static unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));

bool keepRunning = true;

void read_and_write(char* pInputFile, char* pOutputFile)
{
  int in;
  int out = -1;

  in = open(pInputFile, O_RDONLY);

  if (in == -1)
    {
      fprintf(stderr, "Error: failed to open input file %s: %s\n", pInputFile, strerror(errno) );
    }
  else{
    out = open(pOutputFile, O_WRONLY|O_CREAT);

    if (out == -1)
      {
        fprintf(stderr, "Error: failed to open output file %s: %s\n", pOutputFile, strerror(errno) );
      }
  }

  if (in != -1 && out != -1)
    {
      while(keepRunning)
        {
          fd_set rfds;
          int status;
          FD_ZERO(&rfds);
          FD_SET(in, &rfds);


          status = select(in + 1, &rfds, NULL, NULL, NULL);

          if (keepRunning && status > 0)
            {
              int bytesRead = read(in, buf, LOGGER_ENTRY_MAX_LEN);

              if (bytesRead > 0)
                {
                  write(out, buf, bytesRead);
                }
            }
        }

      close(in);
      close(out);
    }
}


static void sigh_exit(int nr)
{
  (void)nr;
  keepRunning = false;
}

int main(int argc, char** argv)
{
  if (argc != 3){
    printf("Usage generate_testdata /dev/log/(main|radio|events) outputfile\n");
  }
  else {
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_handler = sigh_exit;

    if (sigaction(SIGINT, &act, NULL) < 0)
      {
        perror("Failed to install SIGINT handler");
      }

    if (sigaction(SIGTERM, &act, NULL) < 0)
      {
        perror("Failed to install SIGTERM handler");
      }

    read_and_write(argv[1], argv[2]);
  }

  return 0;
}
