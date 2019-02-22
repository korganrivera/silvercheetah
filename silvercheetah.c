/* Thu Feb 21 15:39:49 CST 2019
 * Wahoo .csv file processing code.
 * For more details, read README.md.
 * compile with: gcc silvercheetah.c -o silvercheetah -lm
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TSS_LOG_PATH "/home/korgan/code/silvercheetah/tss.log"
#define CONFIG_PATH "/home/korgan/code/silvercheetah/config"

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

int main(int argc, char **argv){
    FILE *fp;
    char c, folder_location[4098];
    char rm_log_command[4134] = "rm ";
    char find_command[4134] = "find ";
    unsigned filecount;
    unsigned array_size, i, j, k;
    long long unsigned *ts, *new_ts;
    double *ftp, *tss, *atl, *ctl, *tsb;
    double *new_ftp, *new_tss, *new_atl, *new_ctl, *new_tsb;
    double *mps, *ra;
    double NP, FTP, IF, TSS;

    // open config file.
    if((fp = fopen(CONFIG_PATH,"r")) == NULL){
        printf("can't find config file. creating...");
        if((fp = fopen(CONFIG_PATH,"w")) == NULL){
            printf("failed.\n");
            exit(1);
        }

        // set ./wahoo_csv_files as default location of your csv files.
        fprintf(fp, "./whatever_folder_path_goes_here_no_trailing_slash");
        fclose(fp);
        puts("done.\nPut your folder path in the config file and then run silvercheetah again.");
        exit(0);
    }

    // get folder location from config file.
    fgets(folder_location, 4098, fp);
    fclose(fp);
    folder_location[strcspn(folder_location, "\n")] = 0;

    printf("folder location: %s\n", folder_location);

    // build the find command.
    strcat(find_command, folder_location);
    strcat(find_command, " -maxdepth 1 -type f > /home/korgan/code/silvercheetah/filelist");

    // Make list of current files in the target folder.
    //printf("running this command: %s", find_command);
    system(find_command);

    // Count lines in filelist.
    if((fp = fopen("/home/korgan/code/silvercheetah/filelist","r")) == NULL){
        printf("can't open filelist\n");
        exit(1);
    }
    filecount = 0;
    while((c = fgetc(fp)) != EOF)
        if(c == '\n')
            filecount++;
    rewind(fp);

    if(filecount == 0){
        fclose(fp);
        puts("no files, nothing to do. cleaning up.");
        system("rm /home/korgan/code/silvercheetah/filelist");
        if((fp = fopen(TSS_LOG_PATH, "r")) != NULL){
            fclose(fp);
            strcat(rm_log_command, TSS_LOG_PATH);
            system(rm_log_command);
        }
        exit(0);
    }
    printf("%u files found\n", filecount);

    array_size = filecount;
    // allocate space for all the things.
    if((ts  = malloc(array_size * sizeof(long long unsigned))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((ftp = malloc(array_size * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((tss = malloc(array_size * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((ctl = malloc(array_size * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((atl = malloc(array_size * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }
    if((tsb = malloc(array_size * sizeof(double))) == NULL){    fprintf(stderr, "malloc failed.\n"); exit(1); }

    // process files in newlist, put results into arrays.
    for(i = 0; i < array_size; i++){
        char filename[4098];
        fgets(filename, 4098, fp);
        filename[strcspn(filename, "\n")] = 0;
        FILE *wahoo_file;
        if((wahoo_file = fopen(filename,"r")) == NULL){
            printf("can't open %s. :(", filename);
            exit(1);
        }

        // Count lines in the file.
        unsigned wahoo_linecount = 0;
        while((c = fgetc(wahoo_file)) != EOF) if(c == '\n') wahoo_linecount++;
        wahoo_linecount--;
        rewind(wahoo_file);

        // Skip header line.
        while((c = fgetc(wahoo_file)) != '\n' && c != EOF);

        // malloc space for mps values.
        if((mps = malloc(wahoo_linecount * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }

        // Grab the first timestamp while I'm here.
        long long unsigned timestamp;
        fscanf(wahoo_file,"%llu", &timestamp);
        timestamp /= 1000;

        // Read the mps values from wahoo file into mps[].
        for( j = 0; j < wahoo_linecount; j++){
            // skip the first 6 commas.
            unsigned comma = 0;
            while(comma < 6){
                c = fgetc(wahoo_file);
                if(c == ',')
                    comma++;
            }
            mps[j] = 0.0;
            fscanf(wahoo_file, "%lf", &mps[j]);

            // skip to the next line.
            while((c = fgetc(wahoo_file)) != '\n' && c != EOF);
            if(feof(wahoo_file))
                break;
        }
        fclose(wahoo_file);

        // convert mps to watts.
        for( j = 0; j < wahoo_linecount; j++){

            // convert mps to mph.
            mps[j] *= 2.23694;

            // convert to watts using Kinetic's formula.
            mps[j] = mps[j] * (5.24482  + 0.01968 * mps[j] * mps[j]);
        }

        // malloc space for rolling average values.
        if((ra = malloc(wahoo_linecount * sizeof(double))) == NULL){ fprintf(stderr, "malloc failed.\n"); exit(1); }

        // Calculate 30-sec rolling average for NP.
        rolling_average(mps, ra, wahoo_linecount, 30);

        // Raise each value to 4th power and calc average.
        NP = 0.0;
        for(j = 0; j < wahoo_linecount; j++){
            ra[j] = ra[j] * ra[j] * ra[j] * ra[j];
            NP += ra[j] / wahoo_linecount;
        }
        // Find 4th root of this number. This is NP.
        NP = sqrt(sqrt(NP));

        // Calculate FTP: maximum value from 20-min rolling average.
        rolling_average(mps, ra, wahoo_linecount, 1200);
        free(mps);
        FTP = 0.0;
        for(j = 0; j < wahoo_linecount; j++){
            if(ra[j] > FTP)
                FTP = ra[j];
        }
        free(ra);
        FTP *= 0.95;

        // Calc IF.
        IF = NP / FTP;

        // Calc TSS.
        TSS = IF * IF * 100.0 * (wahoo_linecount / 3600.0);

        // Add all of these values to the end of the array.
        ts[i] = timestamp;
        ftp[i] = FTP;
        tss[i] = TSS;
    }
    fclose(fp);
    system("rm /home/korgan/code/silvercheetah/filelist");

    // selection sort the list, by timestamp.
    if(array_size > 1){
        for(i = 0; i < array_size; i++){
            unsigned lowest = i;
            for(j = i + 1; j < array_size; j++){
                if(ts[j] < ts[lowest])
                    lowest = j;
            }
            if(lowest != i){
                long long unsigned u_temp = ts[i];
                ts[i] = ts[lowest];
                ts[lowest] = u_temp;

                    double temp;

                    temp = ftp[i];
                    ftp[i] = ftp[lowest];
                    ftp[lowest] = temp;

                    temp = tss[i];
                    tss[i] = tss[lowest];
                    tss[lowest] = temp;
            }
        }
    }

    // calculate how many interpolations I need.
    unsigned interpolation_count = 0;
    for(i = 1; i < array_size; i++){
        unsigned day_diff = ts[i] / 86400 - ts[i - 1] / 86400;
        if(day_diff > 1)
            interpolation_count += day_diff - 1;
    }

    // create an interpolated array.
    if(interpolation_count > 0){
        if((new_ts  = malloc((array_size + interpolation_count) * sizeof(long long unsigned))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_ftp = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_tss = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_ctl = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_atl = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_tsb = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }

        // Copy first value over because that one is fine.
        new_ts[0] = ts[0];
        new_ftp[0] = ftp[0];
        new_tss[0] = tss[0];
        new_ctl[0] = ctl[0];
        new_atl[0] = atl[0];
        new_tsb[0] = tsb[0];

        for(i = 1,  j = 1; i < array_size; i++, j++){
            unsigned day_diff = ts[i] / 86400 - ts[i - 1] / 86400;
            if(day_diff > 1){
                for(k = 0; k < day_diff - 1; j++, k++){
                    new_ts[j] = new_ts[j - 1] / 86400 * 86400 + 86400;
                    new_ftp[j] = 0.0;
                    new_tss[j] = 0.0;
                    new_ctl[j] = 0.0;
                    new_atl[j] = 0.0;
                    new_tsb[j] = 0.0;
                }
            }
            new_ts[j] = ts[i];
            new_ftp[j] = ftp[i];
            new_tss[j] = tss[i];
            new_ctl[j] = ctl[i];
            new_atl[j] = atl[i];
            new_tsb[j] = tsb[i];
        }

        // interpolated array is complete. swap pointers for new and old arrays, and free the old ones.
        long long unsigned *u_temp = ts;
        ts = new_ts;
        free(u_temp);

        double *temp;
        temp = ftp;
        ftp = new_ftp;
        free(temp);

        temp = tss;
        tss = new_tss;
        free(temp);

        temp = ctl;
        ctl = new_ctl;
        free(temp);

        temp = atl;
        atl = new_atl;
        free(temp);

        temp = tsb;
        tsb = new_tsb;
        free(temp);

        array_size += interpolation_count;
    }

    // calculate how many appendage entries I need.
    long long unsigned current_time = time(NULL);
    long long unsigned last_time = ts[array_size - 1];
    current_time /= 86400;
    last_time /= 86400;
    unsigned difference = current_time - last_time;
    unsigned appendage = 0;
    if(difference > 1)
        appendage = difference - 1;

    // Create yet another array to hold the appendage entries.
    if(appendage > 0){
        if((new_ts  = malloc((array_size + appendage) * sizeof(long long unsigned))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_ftp = malloc((array_size + appendage) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_tss = malloc((array_size + appendage) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_ctl = malloc((array_size + appendage) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_atl = malloc((array_size + appendage) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }
        if((new_tsb = malloc((array_size + appendage) * sizeof(double))) == NULL){ printf("malloc failed.\n"); exit(1); }

        // copy array over.
        for(i = 0; i < array_size; i++){
            new_ts[i] = ts[i];
            new_ftp[i] = ftp[i];
            new_tss[i] = tss[i];
            new_atl[i] = atl[i];
            new_ctl[i] = ctl[i];
            new_tsb[i] = tsb[i];
        }

        // write appendages.
        for(i = array_size; i < array_size + appendage; i++){
            new_ts[i] = new_ts[i - 1] / 86400 * 86400 + 86400;
            new_ftp[i] = 0.0;
            new_tss[i] = 0.0;
        }

        // again, switch array pointers and free mem.
        long long unsigned *u_temp = ts;
        ts = new_ts;
        free(u_temp);

        double *temp;
        temp = ftp;
        ftp = new_ftp;
        free(temp);

        temp = tss;
        tss = new_tss;
        free(temp);

        temp = ctl;
        ctl = new_ctl;
        free(temp);

        temp = atl;
        atl = new_atl;
        free(temp);

        temp = tsb;
        tsb = new_tsb;
        free(temp);

        array_size += appendage;
    }

    // calculate ctl and atl.
    rolling_average(tss, ctl, array_size, 42);
    rolling_average(tss, atl, array_size, 7);

    // calculate tsb.
    for(i = 0; i < array_size; i++)
        tsb[i] = ctl[i] - atl[i];

    // write arrays to tss.log
    if((fp = fopen(TSS_LOG_PATH,"w")) == NULL){ fprintf(stderr, "Can't open tss.log"); exit(1); }
    for(i = 0; i < array_size; i++){
        fprintf(fp, "%llu,%lf,%lf,%lf,%lf,%lf", ts[i], ftp[i], tss[i], ctl[i], atl[i], tsb[i]);
        if(i < array_size - 1)
            fputc('\n', fp);
    }
    // close tss.log
    fclose(fp);

    // free a bunch of memory even though the OS would probably do it for me.
    free(ts);
    free(ftp);
    free(tss);
    free(ctl);
    free(atl);
    free(tsb);
    puts("tss.log updated :)");
}
