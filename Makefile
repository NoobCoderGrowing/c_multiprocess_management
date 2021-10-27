CC = gcc
CFLAGS = -I/local/courses/csse2310/include -Wall -pedantic -std=gnu99
DEBUG = -g
TARGETS = jobrunner

# Mark the default target to run (otherwise make will select the first target in the file)
.DEFAULT: all
# Mark targets as not generating output files (ensure the targets will always run)
.PHONY: all debug clean

all: $(TARGETS)

# A debug target to update flags before cleaning and compiling all targets
debug: CFLAGS += $(DEBUG) 
debug: clean $(TARGETS)

jobrunner: main.o usage.o jobfileParsing.o jobRunability.o runJobs.o
	$(CC) $(CFLAGS) -L/local/courses/csse2310/lib -lcsse2310a3 $^ -o $@

main.o: main.c list.h

usage.o: usage.c list.h

jobfileParsing.o: jobfileParsing.c list.h

jobRunability.o: jobRunability.c runJobs.c list.h

runJobs.o: runJobs.c jobRunability.c list.h


# Clean up our directory - remove objects and binaries
clean:
	rm -f $(TARGETS) *.o