#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "helper.h"

int main(int argc, char **argv) {
    struct timeval starttime, endtime;
    double timediff;

    if((gettimeofday(&starttime, NULL)) == -1) {
    	perror("gettimeofday");
    	exit(1);
    }
    
    extern char *optarg;
    char ch;
    char *input_file = NULL, *output_file = NULL;
    int i, j, n;
    int rec_size = sizeof(struct rec), file_size;
    int pid;
    
    /* Check for the correct number of arguments. */
    if (argc != 7) {
	fprintf(stderr, "Usage: psort -n <number of processes> "
		"-f <input file name> -o <output file name>\n");
	exit(1);
    }

    /* Read in arguments. */
    while ((ch = getopt(argc, argv, "n:f:o:")) != -1) {
        switch(ch) {
	case 'n':
	    n = atoi(optarg);
	    break;	    
        case 'f':
	    input_file = optarg;
	    break;
	case 'o':
	    output_file = optarg;
	    break;
        default:
            fprintf(stderr, "Usage: psort -n <number of processses> "
		"-f <input file name> -o <output file name>\n");
	    exit(1);
	}
    }

    if (n == 0) {
    	fprintf(stderr, "Usage: psort -n <number of processses> "
        "-f <input file name> -o <output file name>\n");
        exit(1);
    } else if (input_file == NULL || output_file == NULL) {
    	fprintf(stderr, "Usage: psort -n <number of processes> "
		"-f <input file name> -o <output file name>\n");
	exit(1);
    }

    /* Check if file size is a multiple of the struct rec size. */
    if ((file_size = get_file_size(input_file)) % rec_size != 0) {
    	fprintf(stderr, "%s: invalid input file\n", input_file); 
    	exit(1);
    }

    int num_recs = file_size / rec_size;
    int status, fd[n][2];

    for (i = 0; i < n; i++) {
    	if (pipe(fd[i]) == -1) {
	    perror("pipe");
	    exit(1);
	}
	if ((pid = fork()) == -1) {
	    perror("fork");
	    exit(1);
	} else if (pid == 0) { /* child */
	    /* Close read and write ends of all unused pipes. */
	    for (j = 0; j < i; j++) {
		Close(fd[j][0]);
		Close(fd[j][1]); 
            }

	    /* Close read end of the used pipe. */
            close(fd[i][0]);
		
	    int recs_per_process;
	    FILE *fp;
	    
	    if (i < num_recs % n) {
                recs_per_process = (num_recs / n) + 1;
            } else {
                recs_per_process = (num_recs / n);
            }

            struct rec *records = (struct rec *)malloc(rec_size * recs_per_process);

	    if ((fp = fopen(input_file, "r")) == NULL) {
                perror("fopen");
		exit(1);
	    } 
	    if (fseek(fp, i * recs_per_process * rec_size, SEEK_SET) != 0) {
		perror("fseek");
		exit(1);
	    }
            if (fread(records, sizeof(struct rec), recs_per_process, fp) == 0) {
		perror("fread");
		exit(1);
	    }
		
            qsort(records, recs_per_process, rec_size, compare_freq);

	    for (j = 0; j < recs_per_process; j++) {
		if(write(fd[i][1], &records[j], rec_size) == -1) {
			perror("write");
			exit(1);
		}
	    }
 
	    Close(fd[i][1]);
	    
	    if (fclose(fp) != 0) {
		perror("fclose");
		exit(1);
            }		
	    
	    free(records);
            exit(0);
	} else { /* parent */	    
            if (waitpid(pid, &status, WNOHANG) == -1) { /* child crashed. */
		perror("wait");
		exit(1);
	    }
	    
	    if (WIFEXITED(status)) { /* child failed. */
	    	if (WEXITSTATUS(status) != 0) {
		    fprintf(stderr, "Child process %d terminated prematurely\n", pid);
		}
	    }
        }
    }

    /* Close writing end of all pipes. */
    for (i = 0; i < n; i++) {
	Close(fd[i][1]);
    }

    int smallest_index, bytes_read, empty_pipes = 0;
    FILE *fp;
    struct rec merge_array[n];

    if((fp = fopen(output_file, "w")) == NULL) {
    	perror("fopen");
	exit(1);
    }

    for (i = 0; i < n; i++) {
    	if((bytes_read = read(fd[i][0], &merge_array[i], rec_size)) == -1) {
            perror("read");
    	    exit(1);
    	} else if (bytes_read == 0) {
    	    merge_array[i].freq = -1;
	    empty_pipes++;	
	}
    }

    while (empty_pipes < n) {
	smallest_index = get_smallest(merge_array, n);
	if (fwrite(&merge_array[smallest_index], rec_size, 1, fp) == 0) {
	    perror("fwrite");
	    exit(1);
	}
	if ((bytes_read = read(fd[smallest_index][0], &merge_array[smallest_index], rec_size)) == -1) {
            perror("read");
            exit(1);
        } else if (bytes_read == 0) {
            merge_array[smallest_index].freq = -1;
	    empty_pipes++;
        }
    }

    /* Close reading end of all pipes. */
    for (i = 0; i < n; i++) {
	Close(fd[i][0]);
    }

    if(fclose(fp) != 0) {
	perror("fclose");
	exit(1);
    }

    if((gettimeofday(&endtime, NULL)) == -1) {
    	perror("gettimeofday");
        exit(1);
    }

    timediff = (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec) / 1000000.0;
    fprintf(stdout, "%.4f\n", timediff);
    return 0;
}

