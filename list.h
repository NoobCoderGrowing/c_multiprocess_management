#ifndef _LIST_H
#define _LIST_H
#include <stdio.h>

typedef enum { 
    NONE, VERBOSE 
} Mode;

typedef struct {
    Mode mode;         // Verbose mode or normal
    char **jobfiles;   // Paths of jobfiles
    int numOfJobfiles; // Number of jobfiles
} Arguments;

typedef struct {
    char **filenames; // filename of the job
    char **jobs;      // valid jobs
    int numOfJobs;    // Number of jobs
    int *jobCanRun;   // 1 if job can wrong and 0 the otherwise
} ValidJobs;

typedef struct {
    char **pipes;   // invalid pipe names
    int numOfPipes; // Number of pipes
} InvalidPipes;

typedef struct {
    int *pIds;        // process IDs
    int *order;       // order of the process
    int numOfProcces; // number of Processes
    int *timeout;
    int *running;
} PidAndOrder;

typedef struct {
    int numOfPipes;        // number of pipes
    char **pipes;          // pipe names
    int **fileDescriptors; // an (numOfPipes * 2) 2d array to hold all file
                           // descripters
} Pipes;

// functions in usage.c
void is_vflag_valid(int argc, char **argv, Arguments *arguments);
void can_open_jobfile(int argc, char **argv, Arguments *arguments);

// functions in jobfileParsing.c
void jobfile_parsing(Arguments *arguments);
int is_empty_line(char *line);
void is_job_syntactically_ok(int lineNum, char *line, char *filename,
        Arguments *arguments, FILE *file);
void is_timeout_field_ok(char *field, int lineNum, char *filename,
        Arguments *arguments, char *line, char **processed,
        FILE *file);

// functions in jobRunability.c
void check_job_runnability(Arguments *arguments);
void get_valid_jobs(Arguments *arguments, ValidJobs *validJobs);
int is_job_line(char *line);
void mark_invalid_stdio_file_job(ValidJobs *validJobs);
int can_open_stdin_file(char *stdinFile);
int can_open_stdout_file(char *stdoutFile);
void mark_invalid_pipe_job(ValidJobs *validJobs);
int is_pipe(char *field);
void check_pipe(ValidJobs *validJobs, char **readPipes, char **writePipes,
        InvalidPipes *invalidPipes);
void is_pipe_valid(int index, char *pipe, char **readPipes, char **writePipes,
        ValidJobs *validJobs, InvalidPipes *invalidPipes);
int does_invalid_pipe_exist(char *pipe, InvalidPipes *invalidPipes);
void add_invalid_pipe(char *pipe, InvalidPipes *invalidPipes);
void check_status4(ValidJobs *validJobs, Arguments *arguments);

// function in runJobs.c
void run_jobs(ValidJobs *validJobs, Arguments *arguments);
void get_command_args(ValidJobs *validJobs, PidAndOrder *pidAndOrder,
        Pipes *pipes);
void reap(PidAndOrder *pidAndOrder);
void create_child(char **commandArgs, int order, PidAndOrder *pidAndOrder,
        char *stdinFile, char *stdoutFile, Pipes *pipes, int timeout);
void set_job_order(int pid, int order, PidAndOrder *pidAndOrder, int timeout);
void redirect_to_null_and_execvp(char **commandArgs);
void open_file_stdin(char *stdinFile);
void open_file_stdout(char *stdoutFile);

// functions to impelement piping feature
void get_all_pipes(ValidJobs *validJobs, Pipes *pipes);
void create_pipes(Pipes *pipes);
void add_pipe(char *pipe, Pipes *pipes);
void close_parent_pipes(Pipes *pipes);
void dup2_only_stdin(char *stdinField, Pipes *pipes);
void dup2_only_stdout(char *stdoutField, Pipes *pipes);
void dup2_stdin_and_stdout(char *stdinField, char *stdoutField, Pipes *pipes);

// timeout related function
void sighup_handler();
int get_timeout(char *timeout);

// verbose mode related function
void verbose_mode(ValidJobs *validJobs, Arguments *arguments);
char *get_timeout_str(char *timeout);
int get_line_number_str_length(int lineNumber);

// reaping realted functions
void remove_finished_process(PidAndOrder *pidAndOrder, int returnPid);

#endif