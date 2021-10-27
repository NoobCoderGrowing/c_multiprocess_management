#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// check if -v flag is valid
void is_vflag_valid(int argc, char **argv, Arguments *arguments) {
    if (argc == 1) {
        fprintf(stderr, "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
        free(arguments->jobfiles);
        free(arguments);
        exit(1);
    }
    int numOfV = 0;
    for (int i = 1; i < argc; i++) {
        // if arguments contains more than 1 vflag
        if (strcmp(argv[i], "-v") == 0) {
            numOfV++;
            // if contains -v, but -v is not the first argument
            if (strcmp(argv[1], "-v") != 0) {
                fprintf(stderr,
                        "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
                free(arguments->jobfiles);
                free(arguments);
                exit(1);
            } else {
                // if no jobfiles specified
                if (argc == 2) {
                    fprintf(stderr,
                            "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
                    free(arguments->jobfiles);
                    free(arguments);
                    exit(1);
                }
                arguments->mode = VERBOSE;
            }
        }
    }
    if (numOfV > 1) { // if there is more than 1 -v tag
        fprintf(stderr, "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
        free(arguments->jobfiles);
        free(arguments);
        exit(1);
    }
}

// check if the jobfile can be opened
void can_open_jobfile(int argc, char **argv, Arguments *arguments) {
    int i = 1;
    // if mode is verbose, set the third argument as the first job file
    if (arguments->mode == VERBOSE) {
        i = 2;
    }
    FILE *file;
    for (; i < argc; i++) {
        file = fopen(argv[i], "r");
        if (file != NULL) {
            fclose(file);
            arguments->jobfiles =
                    realloc(arguments->jobfiles,
                    sizeof(char *) * (++arguments->numOfJobfiles));
            arguments->jobfiles[arguments->numOfJobfiles - 1] = argv[i];
        } else { // if there is a file that can not be opened, exit with 2
            fprintf(stderr, "jobrunner: file \"%s\" can not be opened\n",
                    argv[i]);
            free(arguments->jobfiles);
            free(arguments);
            exit(2);
        }
    }
}