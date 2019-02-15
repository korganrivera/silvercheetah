/*
 * Thu Feb 14 11:26:31 CST 2019
 * freshcheetah.c will read my tss.log file and calculate fitness, fatique, and
 * freshness values before overwriting tss.log.
 * */

// open tss.log. if it doesn't exist, we're done.
// count how many lines are in it. subtract one because of the header comment.
// make space to store timestamp, ftp, tss, ctl, atl, tsb. need array of markers too.
// for each line, read timestamp, ftp, tss.
// if the next value is a comma, then that line needs to be calculated. mark it. skip to next line.
// else, read ctl, atl, tsb.
// close file.
// for each marked line, calculate ctl, atl, tsb.
// write all the things back to the log file.
// I somehow need to interpolate days with no activity.
// also need to make the calculations more efficient so it will use previously calculated values from the log file
// instead of recalculating all of it.
// need to tie silvercheetah to freshcheetah with scripts, or combine everything.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int rolling_average(double *array, double *target, unsigned n, unsigned interval);

int main(int argc, char **argv){
    FILE *log;
    char c;
    unsigned n, i;
    long long *timestamp;
    double *ftp, *tss, *ctl, *atl, *tsb;

    if(argc != 2){
        printf("USAGE: %s <tss.log>\n", argv[0]);
        exit(1);
    }

    // open tss.log
    if((log = fopen("tss.log","r")) == NULL){ fprintf(stderr, "Can't open tss.log"); exit(1); }

    // count lines in file. subtract one to account for header comment.
    n = 0;
    while((c = fgetc(log)) != EOF) if(c == '\n') n++;
    n--;
    rewind(log);

    printf("There are %u lines to process.\n", n);
    // skip over the header comment.
    while((c = fgetc(log)) != '\n' && c != EOF);

    // make storage for all the things.
    if((timestamp = malloc(n * sizeof(long long))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((ftp = malloc(n * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((tss = malloc(n * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((ctl = malloc(n * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((atl = malloc(n * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((tsb = malloc(n * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }

    // read file into arrays.
    for(i = 0; i < n; i++){
        fscanf(log, "%lld,%lf,%lf,", &timestamp[i], &ftp[i], &tss[i]);

        // check what the next character is.
        FILE *origin = log;
        c = fgetc(log);
        if(c == ','){
            while((c = fgetc(log)) != '\n' && c != EOF);
        }
        else{
            log = origin;
            fscanf(log, "%lf,%lf,%lf\n", &ctl[i], &atl[i], &tsb[i]);
        }
    }

    fclose(log);

    // Check what I have so far.
    printf("%-3s%-14s%-11s%-10s%-10s%-10s%-10s\n", " i","timestamp","ftp","tss","ctl","atl","tsb");
    for(i = 0; i < n; i++){
        printf("%2u %lld %lf %lf %lf %lf %lf\n", i, timestamp[i], ftp[i], tss[i], ctl[i], atl[i], tsb[i]);
    }

    // calculate ctl and atl.
    rolling_average(tss, ctl, n, 42);
    rolling_average(tss, atl, n, 7);

    // calculate tsb.
    for(i = 0; i < n; i++)
        tsb[i] = ctl[i] - atl[i];

    // Check again.
    printf("%-3s%-14s%-11s%-10s%-10s%-10s%-10s\n", " i","timestamp","ftp","tss","ctl","atl","tsb");
    for(i = 0; i < n; i++){
        printf("%2u %lld %lf %lf %lf %lf %lf\n", i, timestamp[i], ftp[i], tss[i], ctl[i], atl[i], tsb[i]);
    }

    // overwrite tss.log with new data.
    if((log = fopen("tss.log","w")) == NULL){ fprintf(stderr, "Can't open tss.log"); exit(1); }

    // write header comment.
    fprintf(log, "# timestamp,FTP,TSS,CTL(fitness),ATL(fatigue),TSB(freshness)");

    // write data.
    for(i = 0; i < n; i++){
        fprintf(log, "\n%lld,%lf,%lf,%lf,%lf,%lf", timestamp[i], ftp[i], tss[i], ctl[i], atl[i], tsb[i]);
    }

    // close tss.log
    fclose(log);
}

int rolling_average(double* array, double* target, unsigned n, unsigned interval){
    for(unsigned i = 0; i < n; i++){
        if(i < interval){
            if(i == 0)
                    target[i] = array[i];
            else
                target[i] = (target[i - 1] * i + array[i]) / (i + 1);
        }
        else
            target[i] = (target[i - 1] * interval + array[i] - array[i - interval]) / interval;
    }
}
