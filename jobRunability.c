#include "list.h"
#include <csse2310a3.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// entrypoint funcion of this file
void check_job_runnability(Arguments *arguments) {
    ValidJobs *validJobs = malloc(sizeof(ValidJobs));
    validJobs->numOfJobs = 0;
    validJobs->filenames = malloc(sizeof(char *) * validJobs->numOfJobs);
    validJobs->jobs = malloc(sizeof(char *) * validJobs->numOfJobs);
    validJobs->jobCanRun = malloc(sizeof(int) * validJobs->numOfJobs);
    get_valid_jobs(arguments, validJobs);
    mark_invalid_stdio_file_job(validJobs);
    mark_invalid_pipe_job(validJobs);
    check_status4(validJobs, arguments);
    run_jobs(validJobs, arguments);

    for (int i = 0; i < validJobs->numOfJobs; i++) {
        free(validJobs->filenames[i]);
        free(validJobs->jobs[i]);
    }
    free(validJobs->filenames);
    free(validJobs->jobCanRun);
    free(validJobs->jobs);
    free(validJobs);
}

// get all valid
void get_valid_jobs(Arguments *arguments, ValidJobs *validJobs) {
    FILE *file;
    char *line;
    for (int i = 0; i < arguments->numOfJobfiles; i++) {
        file = fopen(arguments->jobfiles[i], "r");
        line = read_line(file);
        while (line) {
            if (is_job_line(line) == 0) {
                free(line);
                line = read_line(file);
                continue;
            }
            validJobs->numOfJobs++;
            validJobs->jobCanRun = realloc(validJobs->jobCanRun,
                    sizeof(int) * validJobs->numOfJobs);
            validJobs->jobCanRun[validJobs->numOfJobs - 1] = 1;
            validJobs->filenames = realloc(
                    validJobs->filenames, sizeof(char *)
                    * validJobs->numOfJobs);
            validJobs->jobs =
                    realloc(validJobs->jobs, sizeof(char *)
                    * validJobs->numOfJobs);
            validJobs->filenames[validJobs->numOfJobs - 1] =
                    strdup(arguments->jobfiles[i]);
            validJobs->jobs[validJobs->numOfJobs - 1] = strdup(line);
            free(line);
            line = read_line(file);
        }
        fclose(file);
    }
}

// return 1 if this is a job line
int is_job_line(char *line) {
    if (*line == '#') {
        return 0;
    }
    // test if line is empty
    if (strlen(line) == 0) {
        return 0;
    }
    // test if line only contains space and tab
    if (is_empty_line(line) == 1) {
        return 0;
    }
    return 1;
}

// set validJobs->jobCanRun[i] to 0 if it contains invalid stdio file
void mark_invalid_stdio_file_job(ValidJobs *validJobs) {
    int jobRunnable = 0;
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        char *line = strdup(validJobs->jobs[i]);
        char **processed = split_by_commas(line);
        int j = 0;
        while (processed[j]) {
            if (j == 1) { // check stdin file
                jobRunnable = can_open_stdin_file(processed[1]);
                if (jobRunnable == 0) {
                    validJobs->jobCanRun[i] = 0;
                }
            }
            if (j == 2) { // check stdout file
                jobRunnable = can_open_stdout_file(processed[2]);
                if (jobRunnable == 0) {
                    validJobs->jobCanRun[i] = 0;
                }
            }
            j++;
        }
        free(processed);
        free(line);
    }
}

// set validJobs->jobCanRun[i] to 0 if it contains invalid pipe
void mark_invalid_pipe_job(ValidJobs *validJobs) {
    char **readPipes = calloc(validJobs->numOfJobs, sizeof(char *));
    char **writePipes = calloc(validJobs->numOfJobs, sizeof(char *));
    InvalidPipes *invalidPipes = malloc(sizeof(InvalidPipes));
    invalidPipes->numOfPipes = 0;
    invalidPipes->pipes = malloc(sizeof(char *) * invalidPipes->numOfPipes);
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        char *line = strdup(validJobs->jobs[i]);
        char **processed = split_by_commas(line);
        int j = 0;
        while (processed[j]) {
            if (j == 1) { // check stdin PIPE
                if (is_pipe(processed[1]) == 1) {
                    readPipes[i] = strdup(processed[1]);
                }
            }
            if (j == 2) { // check stdout PIPE
                if (is_pipe(processed[2]) == 1) {
                    writePipes[i] = strdup(processed[2]);
                }
            }
            j++;
        }
        free(processed);
        free(line);
    }
    check_pipe(validJobs, readPipes, writePipes, invalidPipes);
    free(readPipes);
    free(writePipes);
    free(invalidPipes->pipes);
    free(invalidPipes);
}

// return 1 if input is a pipe
int is_pipe(char *field) {
    if (*field == '@') {
        return 1;
    }
    return 0;
}

// set validJobs->jobCanRun[i] to 0 if it contains invalid pipe
void check_pipe(ValidJobs *validJobs, char **readPipes, char **writePipes,
        InvalidPipes *invalidPipes) {
    for (int i = 0; i < validJobs->numOfJobs;
            i++) { // check if each read pipe is valid
        if (readPipes[i] != NULL) {
            // check if invalid pipe already printed to stderr
            if (does_invalid_pipe_exist(readPipes[i], invalidPipes) == 0) {
                is_pipe_valid(i, readPipes[i], readPipes, writePipes, 
                        validJobs, invalidPipes);
            } else {
                validJobs->jobCanRun[i] = 0;
            }
        }

        if (writePipes[i] != NULL) {
            // check if invalid pipe already printed to stderr
            if (does_invalid_pipe_exist(writePipes[i], invalidPipes) == 0) {
                is_pipe_valid(i, writePipes[i], readPipes, writePipes,
                        validJobs, invalidPipes);
            } else {
                validJobs->jobCanRun[i] = 0;
            }
        }
    }
}

// check if the job that has the invalid pipe hasn't been printed to stderr
int does_invalid_pipe_exist(char *pipe, InvalidPipes *invalidPipes) {
    for (int i = 0; i < invalidPipes->numOfPipes; i++) {
        if (strcmp(invalidPipes->pipes[i], pipe) == 0) {
            return 1;
        }
    }
    return 0;
}

// if the job that has the invalid pipe hasn't been printed to stderr, print it
// to stderr and add it to invalidPipes
void is_pipe_valid(int index, char *pipe, char **readPipes, char **writePipes,
        ValidJobs *validJobs, InvalidPipes *invalidPipes) {
    int duplicate = 0;
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        if (readPipes[i] == NULL) {
            continue;
        } else {
            if (strcmp(readPipes[i], pipe) == 0) {
                duplicate++;
            }
        }
    }
    if (duplicate != 1) {
        validJobs->jobCanRun[index] = 0;
        add_invalid_pipe(pipe, invalidPipes);
        fprintf(stderr, "Invalid pipe usage \"%s\"\n", (pipe + 1));
        return;
    }
    duplicate = 0;
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        if (writePipes[i] == NULL) {
            continue;
        } else {
            if (strcmp(writePipes[i], pipe) == 0) {
                duplicate++;
            }
        }
    }
    if (duplicate != 1) {
        validJobs->jobCanRun[index] = 0;
        add_invalid_pipe(pipe, invalidPipes);
        fprintf(stderr, "Invalid pipe usage \"%s\"\n", (pipe + 1));
    }
}

// add the invalid pipe to invalidPipes
void add_invalid_pipe(char *pipe, InvalidPipes *invalidPipes) {
    if (does_invalid_pipe_exist(pipe, invalidPipes) == 0) {
        invalidPipes->numOfPipes++;
        invalidPipes->pipes = realloc(
                invalidPipes->pipes, sizeof(char *) *
                invalidPipes->numOfPipes);
        invalidPipes->pipes[invalidPipes->numOfPipes - 1] = pipe;
    }
}

// check if there is any runnable job after stdio file and pipe check
void check_status4(ValidJobs *validJobs, Arguments *arguments) {
    int result = 0;
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        result += validJobs->jobCanRun[i];
    }
    if (result == 0) { // if no job is runnable
        fprintf(stderr, "jobrunner: no runnable jobs\n");
        free(validJobs);
        free(arguments->jobfiles);
        free(arguments);
        exit(4);
    }
}

// return 1 if the stdin file can be opened
int can_open_stdin_file(char *stdinFile) {
    if (strcmp(stdinFile, "-") == 0) { // check if stdin filed is '-'
        return 1;
    }
    if (*stdinFile == '@') { // check if it's a pipe
        return 1;
    }
    int fileDescriptor = open(stdinFile, O_RDONLY); // try open this stdin file
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        return 1;
    }
    fprintf(stderr, "Unable to open \"%s\" for reading\n", stdinFile);
    return 0;
}

// return 1 if the stdout file can be opened
int can_open_stdout_file(char *stdoutFile) {
    if (strcmp(stdoutFile, "-") == 0) { // check if stdin filed is '-'
        return 1;
    }
    if (*stdoutFile == '@') { // check if it's a pipe
        return 1;
    }
    int fileDescriptor =
            open(stdoutFile, O_WRONLY | O_CREAT,
            S_IRWXU | S_IRWXG | S_IRWXO); // try open this stdout file
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        return 1;
    }
    fprintf(stderr, "Unable to open \"%s\" for writing\n", stdoutFile);
    return 0;
}
