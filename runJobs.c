#include "list.h"
#include <csse2310a3.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

// flag indicates that all running process should be terminated if evaluated to
// be 1
int sighupFlag = 0;

// entrypoint function of this file
void run_jobs(ValidJobs *validJobs, Arguments *arguments) {
    PidAndOrder *pidAndOrder = malloc(sizeof(PidAndOrder));
    pidAndOrder->numOfProcces = 0;
    pidAndOrder->pIds = NULL;
    pidAndOrder->order = NULL;
    pidAndOrder->timeout = NULL;
    pidAndOrder->running = NULL;
    Pipes *pipes = malloc(sizeof(Pipes));
    pipes->numOfPipes = 0;
    pipes->fileDescriptors = NULL;
    pipes->pipes = NULL;

    verbose_mode(validJobs, arguments);
    get_all_pipes(validJobs, pipes);
    create_pipes(pipes);
    get_command_args(validJobs, pidAndOrder, pipes);
    close_parent_pipes(pipes);
    reap(pidAndOrder);
    free(pidAndOrder->pIds);
    free(pidAndOrder->order);
    free(pidAndOrder->running);
    free(pidAndOrder->timeout);
    free(pidAndOrder);
    for (int i = 0; i < pipes->numOfPipes; i++) {
        free(pipes->fileDescriptors[i]);
        free(pipes->pipes[i]);
    }
    free(pipes->fileDescriptors);
    free(pipes->pipes);
    free(pipes);
}

// get all valid pipes name and the number of thme
void get_all_pipes(ValidJobs *validJobs, Pipes *pipes) {
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        if (validJobs->jobCanRun[i] == 0) {
            continue;
        }
        char *line = strdup(validJobs->jobs[i]);
        char **processed = split_by_commas(line);
        int j = 0;
        while (processed[j]) {
            if (j == 1) {
                if (is_pipe(processed[j]) == 1) {
                    add_pipe(processed[j], pipes);
                }
            }
            if (j == 2) {
                if (is_pipe(processed[j]) == 1) {
                    add_pipe(processed[j], pipes);
                }
            }
            j++;
        }
        free(processed);
        free(line);
    }
}

// add a pipe
void add_pipe(char *pipe, Pipes *pipes) {
    int duplicate = 0;
    for (int i = 0; i < pipes->numOfPipes; i++) {
        if (strcmp(pipes->pipes[i], pipe) == 0) {
            duplicate = 1;
        }
    }
    if (duplicate == 0) {
        pipes->numOfPipes++;
        pipes->pipes =
                realloc(pipes->pipes, sizeof(char *) * pipes->numOfPipes);
        pipes->pipes[pipes->numOfPipes - 1] = strdup(pipe);
    }
}

// create all pipes
void create_pipes(Pipes *pipes) {
    pipes->fileDescriptors = realloc(
            pipes->fileDescriptors,
            sizeof(int *) *
            pipes->numOfPipes); // already know the number of all pipes here
    for (int i = 0; i < pipes->numOfPipes;
            i++) { // create as many pipes as indicated by pipes->numOfPipes
        pipes->fileDescriptors[i] = malloc(sizeof(int) * 2);
        pipe(pipes->fileDescriptors[i]);
    }
}

// spawn new child here
void create_child(char **commandArgs, int order, PidAndOrder *pidAndOrder,
        char *stdinFile, char *stdoutFile, Pipes *pipes, int timeout) {
    int pid;
    if ((pid = fork()) == 0) { // child
        if (is_pipe(stdinFile) == 0 &&
                strcmp(stdinFile, "-") != 0) { // if stdin field is a file
            open_file_stdin(stdinFile);
        }
        if (is_pipe(stdoutFile) == 0 &&
                strcmp(stdoutFile, "-") != 0) { // if stdout field is a file
            open_file_stdout(stdoutFile);
        }
        if (is_pipe(stdinFile) == 1 &&
                is_pipe(stdoutFile) == 0) { // if only stdin filed is a pipe
            dup2_only_stdin(stdinFile, pipes);
        }
        if (is_pipe(stdinFile) == 0 &&
                is_pipe(stdoutFile) == 1) { // if only stdout filed is a pipe
            dup2_only_stdout(stdoutFile, pipes);
        }
        if (is_pipe(stdinFile) == 1 &&
                is_pipe(stdoutFile) ==
                1) { // if both stdin and stdout filed are pipes
            dup2_stdin_and_stdout(stdinFile, stdoutFile, pipes);
        }
        redirect_to_null_and_execvp(
                commandArgs); // redirect child stderr to null and exec
    } else if (pid > 0) { // parent
        set_job_order(pid, order, pidAndOrder, timeout);
    }
}

// dup2 stdin pipe
void dup2_only_stdin(char *stdinField, Pipes *pipes) {
    for (int i = 0; i < pipes->numOfPipes;
            i++) { // close the write end and dup2 read end and stdin
        if (strcmp(stdinField, pipes->pipes[i]) == 0) {
            close(pipes->fileDescriptors[i][PIPE_WRITE]);
            dup2(pipes->fileDescriptors[i][PIPE_READ], 0);
            close(pipes->fileDescriptors[i][PIPE_READ]);
        } else { // close all other pipe ends
            close(pipes->fileDescriptors[i][PIPE_READ]);
            close(pipes->fileDescriptors[i][PIPE_WRITE]);
        }
    }
}

// dup2 stdout pipe
void dup2_only_stdout(char *stdoutField, Pipes *pipes) {
    for (int i = 0; i < pipes->numOfPipes;
            i++) { // close the write end and dup2 read end and stdin
        if (strcmp(stdoutField, pipes->pipes[i]) == 0) {
            close(pipes->fileDescriptors[i][PIPE_READ]);
            dup2(pipes->fileDescriptors[i][PIPE_WRITE], 1);
            close(pipes->fileDescriptors[i][PIPE_WRITE]);
        } else { // close all other pipe ends
            close(pipes->fileDescriptors[i][PIPE_READ]);
            close(pipes->fileDescriptors[i][PIPE_WRITE]);
        }
    }
}

// dup2 both stdin and stdout pipes
void dup2_stdin_and_stdout(char *stdinField, char *stdoutField, Pipes *pipes) {
    int stdinPipe;
    int stdoutPipe;
    for (int i = 0; i < pipes->numOfPipes;
            i++) { // get read end and write end file descriptors
        if (strcmp(stdinField, pipes->pipes[i]) == 0) {
            stdinPipe = pipes->fileDescriptors[i][PIPE_READ];
        }
        if (strcmp(stdoutField, pipes->pipes[i]) == 0) {
            stdoutPipe = pipes->fileDescriptors[i][PIPE_WRITE];
        }
    }
    for (int i = 0; i < pipes->numOfPipes; i++) { // close all unneeded ends
        if (pipes->fileDescriptors[i][PIPE_READ] != stdinPipe) {
            close(pipes->fileDescriptors[i][PIPE_READ]);
        }
        if (pipes->fileDescriptors[i][PIPE_WRITE] != stdoutPipe) {
            close(pipes->fileDescriptors[i][PIPE_WRITE]);
        }
    }
    dup2(stdinPipe, 0);
    close(stdinPipe);
    dup2(stdoutPipe, 1);
    close(stdoutPipe);
}

// close all pipes of the parent process
void close_parent_pipes(Pipes *pipes) {
    for (int i = 0; i < pipes->numOfPipes; i++) { // close all pipe end on
                                                  // parent
        close(pipes->fileDescriptors[i][PIPE_READ]);
        close(pipes->fileDescriptors[i][PIPE_WRITE]);
    }
}

// parse line and get the arguments for each job
void get_command_args(ValidJobs *validJobs, PidAndOrder *pidAndOrder,
        Pipes *pipes) {
    for (int i = 0; i < validJobs->numOfJobs; i++) {
        char *stdinFile;
        char *stdoutFile;
        int timeout = 0;
        if (validJobs->jobCanRun[i] == 0) {
            continue;
        }
        char *line = strdup(validJobs->jobs[i]);
        char **processed = split_by_commas(line);
        int numOfArgs = 1;
        char **commandArgs = malloc(sizeof(char *)); // execvp argument array
        int j = 0;
        while (processed[j]) {
            if (j == 0) { // set execvp command
                commandArgs[0] = strdup(processed[0]);
            }
            if (j == 1) { // set stdin
                stdinFile = processed[1];
            }
            if (j == 2) { // set stdout
                stdoutFile = processed[2];
            }
            if (j == 3) {
                timeout = get_timeout(processed[3]);
            }
            if (j > 3) { // set execvp command args
                numOfArgs++;
                commandArgs = realloc(commandArgs, sizeof(char *) * numOfArgs);
                // char* arg=strdup(processed[j]);
                commandArgs[numOfArgs - 1] = strdup(processed[j]);
            }
            j++;
        }
        commandArgs[numOfArgs] = NULL;
        create_child(commandArgs, i, pidAndOrder, stdinFile, stdoutFile, pipes,
                timeout);
        for (int z = 0; z < numOfArgs; z++) {
            free(commandArgs[z]);
        }
        free(commandArgs);
        free(processed);
        free(line);
    }
}

// get the int value of timeout field
int get_timeout(char *timeout) {
    int result;
    if (strlen(timeout) == 0) {
        return 0;
    }
    result = atoi(timeout);
    return result;
}

// get the string value of the timeout field
char *get_timeout_str(char *timeout) {
    if (strlen(timeout) == 0) {
        return "0";
    }
    return strdup(timeout);
}

// SIGHUP signal handler
void sighup_handler() { 
    sighupFlag = 1; 
}

// reap evevry terminated process here
void reap(PidAndOrder *pidAndOrder) {
    signal(SIGHUP, sighup_handler);
    int finishedProcess = 0;
    int time = 0;
    while (
            finishedProcess <
            pidAndOrder->numOfProcces) { 
                // loop if there still are running process
        for (int i = 0; i < pidAndOrder->numOfProcces; i++) {
            if (pidAndOrder->running[i] ==
                    1) {   // only running process can be waited
                if (sighupFlag == 1) { 
        // if SIGHUP is received, send SIGKILL to all running processes
                    kill(pidAndOrder->pIds[i], SIGKILL);
                }
                int status;
                int returnPid;
                if (pidAndOrder->timeout[i] !=
                        0) { // if has timeout field specified
                    if (time ==
                            pidAndOrder->timeout[i]) { 
                                // if timeout, send SIGABRT
                        kill(pidAndOrder->pIds[i], SIGABRT);
                    } else if (time >
                            pidAndOrder->timeout[i]) { 
            // if not killed afte receiving SIGABRT, send SIGKILL
                        kill(pidAndOrder->pIds[i], SIGKILL);
                    }
                }
                returnPid =
                        waitpid(pidAndOrder->pIds[i], &status,
                        WNOHANG); // if process is not finished, query next
                if (returnPid > 0) {  
            // if the process is finished, print it, return status to stderr
                    if (WIFSIGNALED(status)) { // if a signal is received
                        fprintf(stderr, "Job %d terminated with signal %d\n",
                                pidAndOrder->order[i] + 1, WTERMSIG(status));
                    } else { // if the job eixts with exit()
                        fprintf(stderr, "Job %d exited with status %d\n",
                                pidAndOrder->order[i] + 1, 
                                WEXITSTATUS(status));
                    }
                    remove_finished_process(pidAndOrder, returnPid);
                    finishedProcess++;
                }
            }
        }
        sleep(1);
        time++;
    }
}

// remove the finished process from all
void remove_finished_process(PidAndOrder *pidAndOrder, int returnPid) {
    for (int i = 0; i < pidAndOrder->numOfProcces; i++) {
        if (pidAndOrder->pIds[i] == returnPid) {
            pidAndOrder->running[i] = 0;
            break;
        }
    }
}

// give each job a order which is used to manage the reaping order
void set_job_order(int pid, int order, PidAndOrder *pidAndOrder, int timeout) {
    pidAndOrder->numOfProcces++; 
    // following code is mainly to manage the order of the job and
    // reap them in order
    pidAndOrder->pIds =
            realloc(pidAndOrder->pIds, sizeof(int) *
            pidAndOrder->numOfProcces);
    pidAndOrder->pIds[pidAndOrder->numOfProcces - 1] = pid;
    pidAndOrder->order =
            realloc(pidAndOrder->order, sizeof(int) *
            pidAndOrder->numOfProcces);
    pidAndOrder->order[pidAndOrder->numOfProcces - 1] = order;
    pidAndOrder->timeout =
            realloc(pidAndOrder->timeout, sizeof(int) * 
            pidAndOrder->numOfProcces);
    pidAndOrder->timeout[pidAndOrder->numOfProcces - 1] = timeout;
    pidAndOrder->running =
            realloc(pidAndOrder->running, sizeof(int) *
            pidAndOrder->numOfProcces);
    pidAndOrder->running[pidAndOrder->numOfProcces - 1] = 1;
}

// redirect stderr of a child process to null
void redirect_to_null_and_execvp(char **commandArgs) {
    int devNull = open("/dev/null", O_WRONLY);
    dup2(devNull, 2);
    close(devNull);
    int i = 0;
    while (commandArgs[i]) {
        i++;
    }
    commandArgs[i] = NULL;
    if (execvp(commandArgs[0], commandArgs) == -1) {
        exit(255);
    }
}

// get the stdin file descriptor
void open_file_stdin(char *stdinFile) {
    int fileDescriptor = open(stdinFile, O_RDONLY);
    dup2(fileDescriptor, 0);
    close(fileDescriptor);
}

// get the stdout file descriptoer
void open_file_stdout(char *stdoutFile) {
    int fileDescriptor = open(stdoutFile, O_WRONLY | O_TRUNC);
    dup2(fileDescriptor, 1);
    close(fileDescriptor);
}

// print all runnable job if it is in verbose mode
void verbose_mode(ValidJobs *validJobs, Arguments *arguments) {
    if (arguments->mode == VERBOSE) { // enter verbose mode
        char *line;
        char **processed;
        char *newLine;
        for (int i = 0; i < validJobs->numOfJobs; i++) {
            if (validJobs->jobCanRun[i] == 1) {
                line = strdup(validJobs->jobs[i]);
                processed = split_by_commas(line);
                int z = 0;
                while (processed[z]) {
                    z++;
                }
                newLine =
                        malloc(sizeof(char) * 
                        get_line_number_str_length(i + 1));
                sprintf(newLine, "%d", i + 1);
                for (int j = 0; j < z; j++) {
                    if (j == 3) {
                        strcat(newLine, ":");
                        strcat(newLine, get_timeout_str(processed[j]));
                    } else {
                        strcat(newLine, ":");
                        strcat(newLine, strdup(processed[j]));
                    }
                }
                if (z == 3) {
                    strcat(newLine, ":");
                    char timeoutStr2[2];
                    sprintf(timeoutStr2, "%d", 0);
                    strcat(newLine, timeoutStr2);
                }
                fprintf(stderr, "%s\n", newLine);
            } else {
                continue;
            }
            free(newLine);
            free(processed);
            free(line);
        }
    } else {
        return;
    }
}

// determine the string length for the timeout field
int get_line_number_str_length(int lineNumber) {
    int length = 2;
    while (lineNumber > 10) {
        lineNumber = lineNumber / 10;
        length++;
    }
    return length;
}
