#include "list.h" //Definition of all enums and structs
#include <stdlib.h>

int main(int argc, char **argv) {
    //Command Line Processing
    Arguments *arguments = malloc(sizeof(Arguments));
    arguments->numOfJobfiles = 0;
    arguments->mode = NONE;
    arguments->jobfiles = malloc(sizeof(char *) * arguments->numOfJobfiles);
    is_vflag_valid(argc, argv, arguments);
    can_open_jobfile(argc, argv, arguments);
    jobfile_parsing(arguments);
    check_job_runnability(arguments);
    free(arguments->jobfiles);
    free(arguments);
}
