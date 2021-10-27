#include "list.h"
#include <csse2310a3.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// entrypoint function of this file
void jobfile_parsing(Arguments *arguments) {
    FILE *file;
    char *line;
    int lineNum;
    for (int j = 0; j < arguments->numOfJobfiles; j++) {
        file = fopen(arguments->jobfiles[j], "r");
        line = read_line(file);
        lineNum = 0;
        while (line) {
            lineNum++;
            // test if line start with '#'
            if (*line == '#') {
                free(line);
                line = read_line(file);
                continue;
            }
            // test if line is empty
            if (strlen(line) == 0) {
                free(line);
                line = read_line(file);
                continue;
            }
            // test if line only contains space and tab
            if (is_empty_line(line) == 1) {
                free(line);
                line = read_line(file);
                continue;
            }
            // check if timeout valid
            is_job_syntactically_ok(lineNum, line, arguments->jobfiles[j],
                    arguments, file);
            free(line);
            line = read_line(file);
        }
        fclose(file);
    }
}

// return 1 if the line is empty
int is_empty_line(char *line) {
    for (int i = 0; i < strlen(line); i++) {
        if (isspace(*(line + i)) == 0) {
            return 0;
        }
    }
    return 1;
}

// check if the job has less than 3 field and if any of them is empty
void is_job_syntactically_ok(int lineNum, char *line, char *filename,
        Arguments *arguments, FILE *file) {
    char **processed = split_by_commas(line);
    int i = 0;
    while (processed[i] && i < 4) {
        // test if first three fields are empty;
        if (i == 0 || i == 1 || i == 2) {
            if (strlen(processed[i]) == 0) {
                fprintf(stderr,
                        "jobrunner: invalid job specification on line %d of "
                        "\"%s\"\n",
                        lineNum, filename);
                free(processed);
                free(line);
                fclose(file);
                free(arguments->jobfiles);
                free(arguments);
                exit(3);
            }
        }
        if (i == 3) { // check if timeout field is valid
            is_timeout_field_ok(processed[3], lineNum, filename, arguments,
                    line, processed, file);
        }
        i++;
    }
    // if the fileds number is less than 3
    if (i < 3) {
        fprintf(stderr,
                "jobrunner: invalid job specification on line %d of \"%s\"\n",
                lineNum, filename);
        free(processed);
        free(line);
        fclose(file);
        free(arguments->jobfiles);
        free(arguments);
        exit(3);
    }
    free(processed);
}

// check if the timeout filed is valid
void is_timeout_field_ok(char *field, int lineNum, char *filename,
        Arguments *arguments, char *line, char **proccessed,
        FILE *file) {
    float result;
    char *rest;
    for (int i = 0; i < strlen(field); i++) { // check if contains space
        if (isspace(*(field + i))) {
            fprintf(stderr,
                    "jobrunner: invalid job specification on line %d\t"
                    " of \"%s\"\n",lineNum, filename);
            free(proccessed);
            free(line);
            fclose(file);
            free(arguments->jobfiles);
            free(arguments);
            exit(3);
        }
        if (*(field + i) == '.') { // check if contains '.'
            fprintf(stderr,
                    "jobrunner: invalid job specification on line %d\t"
                    " of \"%s\"\n", lineNum, filename);
            free(proccessed);
            free(line);
            fclose(file);
            free(arguments->jobfiles);
            free(arguments);
            exit(3);
        }
    }
    result = strtof(field, &rest);
    if (result > (int)result || result < 0) { // check if it's float or < 0
        fprintf(stderr,
                "jobrunner: invalid job specification on line %d of \"%s\"\n",
                lineNum, filename);
        free(proccessed);
        free(line);
        fclose(file);
        free(arguments->jobfiles);
        free(arguments);
        exit(3);
    }
    if (strlen(rest) > 0) { // check if contains other than integer
        fprintf(stderr,
                "jobrunner: invalid job specification on line %d of \"%s\"\n",
                lineNum, filename);
        free(proccessed);
        free(line);
        fclose(file);
        free(arguments->jobfiles);
        free(arguments);
        exit(3);
    }
}
