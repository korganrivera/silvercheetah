/* Thu Feb 21 15:39:49 CST 2019
 * Wahoo .csv file processing code.
 * For more details, read README.md.
 * either use make, or compile with: gcc silvercheetah.c -o silvercheetah -lm
 *
 * New order of operations to fix my FTP numbers:
 * for each file:
 *   get timestamp
 *   get time in secs
 *   calculate NP
 *   calculate file's FTP (This is different from table's FTP)
 * with table:
 *   calculate table's FTP
 *   calculate IF
 *   calculate TSS
 *   ATL, CTL, TSB
 * write to tss.log:
 *   timestamp, NP, duration, FTP, IF, TSS
 *
 *   There's an issue with the timestamps.
 *   All timestamps are in UTC and don't include any info about daylight savings time.
 *   So, for example, these two timestamps:
 *                   my time zone              UTC
 *   1552752597    3-16-19 11:09:57    3-16-19 16:09:57 UTC
 *   1552875565    3-17-19 21:19:25    3-18-19 02:19:25 UTC
 *
 *   So according to my time zone, these two timestamps occur on days that
 *   follow each other.  But according to UTC, they are two days apart.
 *
 *   It actually makes more sense to use UTC since it is consistent, but it
 *   will seem confusing when the tss.log says there is a 2-day difference
 *   between workouts when there looks like only one in your time zone.
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
    return 1;
}

int main(int argc, char **argv){
    FILE *fp;

    char c, folder_location[4098];
    char rm_log_command[4134] = "rm ";
    char find_command[4134] = "find ";

    unsigned filecount;
    unsigned array_size;
    unsigned i, j, k;
    unsigned *duration, *new_duration;

    long long unsigned *ts, *new_ts;

    double *np, *file_ftp, *ftp, *ifact, *tss, *atl, *ctl;
    double *tsb, *new_np, *new_file_ftp, *mps, *ra;

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

    // build the find command and run it.
    strcat(find_command, folder_location);
    strcat(find_command, " -maxdepth 1 -type f > /home/korgan/code/silvercheetah/filelist");
    system(find_command);

    // Count lines in filelist.
    if((fp = fopen("/home/korgan/code/silvercheetah/filelist","r")) == NULL){
        puts("can't open filelist");
        exit(1);
    }
    filecount = 0;
    while((c = fgetc(fp)) != EOF)
        if(c == '\n')
            filecount++;
    rewind(fp);

    // if no files found, clean up and exit.
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

    // allocate space for all the things.
    array_size = filecount;
    if((ts       = malloc(array_size * sizeof(long long unsigned))) == NULL){ puts("malloc failed."); exit(1); }
    if((np       = malloc(array_size * sizeof(double))) == NULL){   puts("malloc failed."); exit(1); }
    if((duration = malloc(array_size * sizeof(unsigned))) == NULL){ puts("malloc failed."); exit(1); }
    if((file_ftp = malloc(array_size * sizeof(double))) == NULL){   puts("malloc failed."); exit(1); }

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
        duration[i] = 0;
        while((c = fgetc(wahoo_file)) != EOF) if(c == '\n') duration[i]++;
        duration[i]--;
        rewind(wahoo_file);

        // Skip header line.
        while((c = fgetc(wahoo_file)) != '\n' && c != EOF);

        // malloc space for mps values.
        if((mps = malloc(duration[i] * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }

        // Grab the first timestamp while I'm here.
        fscanf(wahoo_file,"%llu", &ts[i]);
        ts[i] /= 1000;

        // Read the mps values from wahoo file into mps[].
        for( j = 0; j < duration[i]; j++){
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
        for(j = 0; j < duration[i]; j++){

            // convert mps to mph.
            mps[j] *= 2.23694;

            // convert to watts using Kinetic's formula.
            mps[j] = mps[j] * (5.24482  + 0.01968 * mps[j] * mps[j]);
        }

        // malloc space for rolling average values.
        if((ra = malloc(duration[i] * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }

        // Calculate 30-sec rolling average for NP.
        rolling_average(mps, ra, duration[i], 30);

        // Raise each value to 4th power and calc average.
        np[i] = 0.0;
        for(j = 0; j < duration[i]; j++){
            ra[j] = ra[j] * ra[j] * ra[j] * ra[j];
            np[i] += ra[j] / duration[i];
        }
        // Find 4th root of this number. This is NP.
        np[i] = sqrt(sqrt(np[i]));

        // Calculate file_FTP: maximum value from 20-min rolling average.
        rolling_average(mps, ra, duration[i], 1200);
        free(mps);
        file_ftp[i] = 0.0;
        for(j = 0; j < duration[i]; j++){
            if(ra[j] > file_ftp[i])
                file_ftp[i] = ra[j];
        }
        free(ra);
        file_ftp[i] *= 0.95;
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
                long long unsigned u_temp;
                u_temp = ts[i];
                ts[i] = ts[lowest];
                ts[lowest] = u_temp;

                double d_temp;
                d_temp = np[i];
                np[i] = np[lowest];
                np[lowest] = d_temp;

                u_temp = duration[i];
                duration[i] = duration[lowest];
                duration[lowest] = u_temp;

                d_temp = file_ftp[i];
                file_ftp[i] = file_ftp[lowest];
                file_ftp[lowest] = d_temp;
            }
        }
    }

    // calculate how many interpolations I need.
    unsigned interpolation_count = 0;
    for(i = 1; i < array_size; i++){
        unsigned day_diff = (ts[i] / 86400) - (ts[i - 1] / 86400);
        if(day_diff > 1){
            interpolation_count += day_diff - 1;
        }
    }

    // create an interpolated array.
    if(interpolation_count > 0){
        if((new_ts = malloc((array_size + interpolation_count) * sizeof(long long unsigned))) == NULL){ puts("malloc failed."); exit(1); }
        if((new_np = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }
        if((new_duration = malloc((array_size + interpolation_count) * sizeof(unsigned))) == NULL){ puts("malloc failed."); exit(1); }
        if((new_file_ftp = malloc((array_size + interpolation_count) * sizeof(double))) == NULL){ puts("malloc failed.n"); exit(1); }

        // Copy first value over because that one is fine.
        new_ts[0]       = ts[0];
        new_np[0]       = np[0];
        new_duration[0] = duration[0];
        new_file_ftp[0] = file_ftp[0];

        // Interpolate gaps between timestamps.
        for(i = 1, j = 1; i < array_size; i++, j++){
            unsigned day_diff = (ts[i] / 86400) - (ts[i - 1] / 86400);
            if(day_diff > 1){
                for(k = 0; k < day_diff - 1; j++, k++){
                    new_ts[j] = new_ts[j - 1] / 86400 * 86400 + 86400;
                    new_file_ftp[j] = 0.0;
                    new_np[j] = 0.0;
                    new_duration[j] = 0;
                }
            }
            new_ts[j] = ts[i];
            new_file_ftp[j] = file_ftp[i];
            new_np[j] = np[i];
            new_duration[j] = duration[i];
        }

        // interpolated array is complete. swap pointers for new and old arrays, and free the old ones.
        long long unsigned *llu_temp = ts;
        ts = new_ts;
        free(llu_temp);

        unsigned *u_temp;
        u_temp = duration;
        duration = new_duration;
        free(u_temp);

        double *d_temp;
        d_temp = np;
        np = new_np;
        free(d_temp);

        d_temp = file_ftp;
        file_ftp = new_file_ftp;
        free(d_temp);

        array_size += interpolation_count;
    }

    // calculate how many appendage entries I need.
    long long unsigned current_time = time(NULL) / 86400;
    long long unsigned last_time = ts[array_size - 1] / 86400;
    unsigned appendage = current_time - last_time - 1;

    // Create yet another array to hold the appendage entries.
    if(appendage > 0){
        if((new_ts = malloc((array_size + appendage) * sizeof(long long unsigned))) == NULL){ puts("malloc failed."); exit(1); }
        if((new_np       = malloc((array_size + appendage) * sizeof(double))) == NULL){ puts("malloc failed.n"); exit(1); }
        if((new_duration = malloc((array_size + appendage) * sizeof(unsigned))) == NULL){ puts("malloc failed.n"); exit(1); }
        if((new_file_ftp = malloc((array_size + appendage) * sizeof(double))) == NULL){ puts("malloc failed.n"); exit(1); }

        // copy array over.
        for(i = 0; i < array_size; i++){
            new_ts[i]       = ts[i];
            new_np[i]       = np[i];
            new_duration[i] = duration[i];
            new_file_ftp[i] = file_ftp[i];
        }

        // write appendages.
        for(i = array_size; i < array_size + appendage; i++){
            new_ts[i] = new_ts[i - 1] / 86400 * 86400 + 86400;
            new_np[i] = 0.0;
            new_duration[i] = 0;
            new_file_ftp[i] = 0.0;
        }

        // again, switch array pointers and free mem.
        long long unsigned *llu_temp;
        llu_temp = ts;
        ts = new_ts;
        free(llu_temp);

        unsigned *u_temp;
        u_temp = duration;
        duration = new_duration;
        free(u_temp);

        double *d_temp;
        d_temp = np;
        np = new_np;
        free(d_temp);

        d_temp = file_ftp;
        file_ftp = new_file_ftp;
        free(d_temp);

        array_size += appendage;
    }

    // malloc space for next set of calculations.
    if((ftp      = malloc(array_size * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }
    if((ifact    = malloc(array_size * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }
    if((tss      = malloc(array_size * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }
    if((ctl      = malloc(array_size * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }
    if((atl      = malloc(array_size * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }
    if((tsb      = malloc(array_size * sizeof(double))) == NULL){ puts("malloc failed."); exit(1); }

    // calculate table_ftp: current ftp will be highest ftp before current.
    ftp[0] = file_ftp[0];
    for(i = 1; i < array_size; i++){
        if(file_ftp[i] > ftp[i - 1])
            ftp[i] = file_ftp[i];
        else
            ftp[i] = ftp[i - 1];
    }

    // improve FTP estimate by linear-interpolating the high values.
    unsigned left = 0;
    for(i = 1; i < array_size; i++){
        if(ftp[i] > ftp[left]){
            // interpolate between ftp[left] and ftp[i].
            unsigned gap = i - left;
            double change = (ftp[i] - ftp[left]) / gap;
            for(left = left + 1; left < i; left++){
                ftp[left] = ftp[left - 1] + change;
            }
        }
    }

    // calculate IF.
    for(i = 0; i < array_size; i++){
        ifact[i] = np[i] / ftp[i];
    }

    // calculate tss.
    for(i = 0; i < array_size; i++)
        tss[i] = ifact[i] * ifact[i] * 100.0 * (duration[i] / 3600.0);

    // calculate ctl and atl.
    rolling_average(tss, ctl, array_size, 42);
    rolling_average(tss, atl, array_size, 7);

    // calculate tsb.
    for(i = 0; i < array_size; i++)
        tsb[i] = ctl[i] - atl[i];

    // write arrays to tss.log
    if((fp = fopen(TSS_LOG_PATH,"w")) == NULL){ puts("Can't open tss.log"); exit(1); }
    fprintf(fp, "TIMESTAMP |  NP   | secs |  FTP  | IF  |  TSS  |  CTL  |  ATL  |  TSB\n");
    for(i = 0; i < array_size; i++){
    fprintf(fp, "%-10llu %7.3lf %6u %7.3lf %5.3lf %7.3lf %7.3lf %7.3lf %6.3lf", ts[i], np[i], duration[i], ftp[i], ifact[i], tss[i], ctl[i], atl[i], tsb[i]);
        if(i < array_size - 1)
            fputc('\n', fp);
    }

    // close tss.log
    fclose(fp);

    puts("tss.log updated :)");

    // free a bunch of memory even though the OS would probably do it for me.
    free(ts);
    free(np);
    free(duration);
    free(file_ftp);
    free(ftp);
    free(ifact);
    free(tss);
    free(ctl);
    free(atl);
    free(tsb);
}
