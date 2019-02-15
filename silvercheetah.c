/* Fri Feb 15 17:36:08 CST 2019
 * This is a tool that processes my Wahoo Fitness files.
 * Monitors a folder for new csv files, processes them:
 * Strips speed values from wahoo csv files and calculates ftp and tss.
 * writes these to tss.log.
 * Then processes tss.log to calculate atl, ctl, and tsb.
 * Uses some shell commands like system, find, sort, uniq, rm, mv, notify-send
 * so make sure those work.
 * currently assumes that the folder being monitored is one of my Dropbox folders.
 * I need to fix that so that anyone can set their own folder.
 * Also need to refactor and clean up the code. Make it prettier.
 * to compile: gcc silvercheetah.c -o silvercheetah -lm
 * usage: ./silvercheetah
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int rolling_average(double *array, double *target, unsigned n, unsigned interval);

int main(int argc, char **argv){
    FILE *fp, *wahoo_file, *new_files, *log;
    char c;
    unsigned i, j, filecount, linecount;
    double *mps;
    double *ra;
    double AP, NP, FTP, IF, TSS;
    double *ftp, *tss, *ctl, *atl, *tsb;
    long long timestamp, *ts;
    // Make list of current files in my dropbox folder.
    system("find ~/Dropbox/cycling_files/csv_files -maxdepth 1 -type f > filelist");
    rebuild:
    // If old list doesn't exist, create it.
    if((fp = fopen(".files_old","r")) == NULL){
        if((fp = fopen(".files_old","w")) == NULL){
            puts("Can't create .files_old :(");
            exit(1);
        }
    }
    fclose(fp);
    // Make a list of the new files.
    system("sort .files_old filelist | uniq -u > newfiles");
    // open newfiles.
    if((fp = fopen("newfiles","r")) == NULL){ fprintf(stderr, "\ncan't open newfiles"); exit(1); }

    //count filenames.
    filecount = 0;
    while((c = fgetc(fp)) != EOF)
        if(c == '\n')
            filecount++;
    rewind(fp);
    if(filecount)
        system("notify-send \"silvercheetah: updating\"");
    else
        puts("tss.log is up to date");
    // process each file.
    for(i = 0; i < filecount; i++){
        char filename[4098];
        fgets(filename, 4098, fp);
        // remove fget's newline if there is one.
        filename[strcspn(filename, "\n")] = 0;
        // Open the Wahoo Fitness csv file.
        if((wahoo_file = fopen(filename,"r")) == NULL){
            // If files were removed, rebuild newfiles and tss.log from scratch.
            puts("Files were removed. Rebuilding tss.log");
            system("rm .files_old");
            system("rm tss.log");
            fclose(fp);
            goto rebuild;
        }
        printf("adding file \"%s\"...", filename);
        // Count lines in the file.
        linecount = 0;
        while((c = fgetc(wahoo_file)) != EOF) if(c == '\n') linecount++;
        rewind(wahoo_file);
        // malloc space for mps values.
        if((mps = malloc(linecount * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
        // malloc space for NP values.
        if((ra = malloc(linecount * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
        // Skip header line.
        while((c = fgetc(wahoo_file)) != '\n' && c != EOF);
        // Grab the first timestamp while I'm here.
        fscanf(wahoo_file,"%lld", &timestamp);
        // Read just the mps values into the array.
        unsigned comma = j = 0;
        while((c = fgetc(wahoo_file)) != EOF){
            // Skip first 6 fields.
            while(comma < 6){
                c = fgetc(wahoo_file);
                if(c == ',')
                    comma++;
            }
            // Read the float in the 7th field.
            fscanf(wahoo_file, "%lf", &mps[j++]);
            // skip to the next line.
            while((c = fgetc(wahoo_file)) != '\n' && c != EOF);
            comma = 0;
        }
        fclose(wahoo_file);

        // calculate AP.
        AP = 0.0;
        for(j = 0; j < linecount; j++){
            // convert mps to mph.
            mps[j] *= 2.23694;
            // convert to watts using Kinetic's formula.
            mps[j] = mps[j] * (5.24482  + 0.01968 * mps[j] * mps[j]);
            // calculate average power.
            AP += mps[j] / (double)linecount;
        }
        // Calculate 30-sec rolling average for NP.
        rolling_average(mps, ra, linecount, 30);
        // Raise each value to 4th power and calc average.
        NP = 0.0;
        for(j = 0; j < linecount; j++){
            ra[j] = ra[j] * ra[j] * ra[j] * ra[j];
            NP += ra[j] / (double)linecount;
        }
        // Find 4th root of this number.
        NP = sqrt(sqrt(NP));
        // Calculate FTP: maximum value from 20-min rolling average.
        rolling_average(mps, ra, linecount, 1200);
        free(mps);
        FTP = 0.0;
        for(j = 0; j < linecount; j++){
            if(ra[j] > FTP)
                FTP = ra[j];
        }
        free(ra);
        FTP *= 0.95;
        // Calc IF.
        IF = NP / FTP;
        // Calc TSS.
        TSS = IF * IF * 100.0 * ((double)linecount / 3600.0);
        // Open tss.log. if it doesn't exist, create it and add a header comment to it.
        if((log = fopen("tss.log","r")) == NULL){
            if((log = fopen("tss.log","a")) == NULL){
                fprintf(stderr, "can't create tss.log.");
                exit(1);
            }
            fprintf(log, "# timestamp,FTP,TSS,CTL(fitness),ATL(fatigue),TSB(freshness)");
        }
        else{
            fclose(log);
            if((log = fopen("tss.log","a")) == NULL){
                fprintf(stderr, "can't open tss.log.");
                exit(1);
            }
        }
        // write timestamp, ftp and tss to log.
        fprintf(log, "\n%lld,%lf,%lf,,,", timestamp, FTP, TSS);
        fclose(log);
        printf("done\n");
    }
    fclose(fp);
    system("rm newfiles");
    // make new .files_old file.
    system("mv filelist .files_old");

    // if no files were added to tss.log, then we're done.
    if(filecount == 0)
        exit(0);
    // otherwise, process tss.log freshcheetah-style.
    // open tss.log
    if((log = fopen("tss.log","r")) == NULL){ fprintf(stderr, "Can't open tss.log"); exit(1); }
    // count lines in file.
    linecount = 0;
    while((c = fgetc(log)) != EOF) if(c == '\n') linecount++;
    rewind(log);
    // skip over the header comment.
    while((c = fgetc(log)) != '\n' && c != EOF);
    // make storage for all the things.
    if((ts  = malloc(linecount * sizeof(long long))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((ftp = malloc(linecount * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((tss = malloc(linecount * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((ctl = malloc(linecount * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((atl = malloc(linecount * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((tsb = malloc(linecount * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }

    // read tss.log into arrays.
    for(i = 0; i < linecount; i++){
        fscanf(log, "%lld,%lf,%lf,", &ts[i], &ftp[i], &tss[i]);
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

    // calculate ctl and atl.
    rolling_average(tss, ctl, linecount, 42);
    rolling_average(tss, atl, linecount, 7);

    // calculate tsb.
    for(i = 0; i < linecount; i++)
        tsb[i] = ctl[i] - atl[i];

    // overwrite tss.log with new data.
    if((log = fopen("tss.log","w")) == NULL){ fprintf(stderr, "Can't open tss.log"); exit(1); }
    // write header comment.
    fprintf(log, "# timestamp,FTP,TSS,CTL(fitness),ATL(fatigue),TSB(freshness)");
    // write data.
    for(i = 0; i < linecount; i++)
        fprintf(log, "\n%lld,%lf,%lf,%lf,%lf,%lf", ts[i], ftp[i], tss[i], ctl[i], atl[i], tsb[i]);
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
