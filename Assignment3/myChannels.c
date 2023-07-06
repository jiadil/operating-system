#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 512

pthread_mutex_t global_checkpointing;

char* metadata_file_path = "inputs/metadata.txt";
char* output_file_path = "output.txt";
int buffer_size;
int num_threads;
int lock_config;
// int global_checkpointing;

int global_lock = 0;
int entry_lock = 0;

FILE* output_file;
FILE** input_files;
int num_input_files;
int files_per_thread;
float* alpha_list;
float* beta_list;

// remove newline from string
char* remove_newline(char* str) {
    int i = 0;
    int j = 0;
    while(str[i]) {
        if (str[i] != '\n') {
            str[j] = str[i];
            j++;
        }
        i++;
    }
    str[j]='\0';
    return str;
}

// compare and swap
int compare_and_swap(int *value, int expected, int new_value) {
    int temp = *value;
    if (temp == expected) {
        *value = new_value;
    }
    return temp;
}

// find max value in array
int max(int arr[], int n) {
    int max = arr[0];
    for (int i = 1; i < n; i++) {
        if (max < arr[i]) {
            max = arr[i];
        }
    }
    return max;
}

// write to output file
void write_to_output_file(int buffer_float_list_length, float beta_cal_result[files_per_thread][MAX_LINE_LENGTH]) {
    char output_buffer[MAX_LINE_LENGTH] = "";
    char *output_buffer_char = output_buffer;
    size_t output_buffer_size = MAX_LINE_LENGTH;
    int output_value_to_read;
    int output_value_to_write;

    if (lock_config == 1) {
        // waiting global lock
        while (global_lock == 1);
        global_lock = 1;

        for (int i = 0; i < files_per_thread; i++) {
            // set file pointer to the beginning of the file
            rewind(output_file);

            for (int j = 0; j < buffer_float_list_length; j++) {
                // get line from current output file
                if (getline(&output_buffer_char, &output_buffer_size, output_file) != EOF) {
                    fseek(output_file, -strlen(output_buffer), SEEK_CUR);
                }

                if (output_buffer[0] == '\0') {
                    output_value_to_read = 0;
                } else {
                    output_value_to_read = strtof(output_buffer, NULL);
                }

                // store the value to write
                // output_value_to_read = strtof(output_buffer, NULL);
                output_value_to_write = output_value_to_read + beta_cal_result[i][j]; // convert to float
                if (output_value_to_write < output_value_to_read + beta_cal_result[i][j]) { // check overflow
                    output_value_to_write = output_value_to_write + 1;
                }

                // write to output file
                fprintf(output_file, "%d\n", output_value_to_write);

                // flush the buffer
                for (int m = 0; m < MAX_LINE_LENGTH; m++) {
                    output_buffer[m] = '\0';
                }
            }
        }

        global_lock = 0;
    } 
    
    // entry lock
    else if (lock_config == 2) {
        for (int i = 0; i < files_per_thread; i++) {
            // set file pointer to the beginning of the file
            rewind(output_file);

            for (int j = 0; j < buffer_float_list_length; j++) {
                // wait for entry lock
                while (entry_lock == 1);
                entry_lock = 1;

                // get line from current output file
                if (getline(&output_buffer_char, &output_buffer_size, output_file) != EOF) {
                    fseek(output_file, -strlen(output_buffer), SEEK_CUR);
                }

                // store the value to write
                output_value_to_read = strtof(output_buffer, NULL);
                output_value_to_write = output_value_to_read + beta_cal_result[i][j]; // convert to float
                if (output_value_to_write < output_value_to_read + beta_cal_result[i][j]) { // check overflow
                    output_value_to_write = output_value_to_write + 1;
                }

                // write to output file
                fprintf(output_file, "%d\n", output_value_to_write);

                // flush the buffer
                for (int m = 0; m < MAX_LINE_LENGTH; m++) {
                    output_buffer[m] = '\0';
                }

                entry_lock = 0;
            }
        }
    }

    // compare and swap
    else if (lock_config == 3) {
        for (int i = 0; i < files_per_thread; i++) {
            // set file pointer to the beginning of the file
            rewind(output_file);

            for (int j = 0; j < buffer_float_list_length; j++) {
                // compare and swap
                while (compare_and_swap(&global_lock, 0, 1) != 0);

                // get line from current output file
                if (getline(&output_buffer_char, &output_buffer_size, output_file) != EOF) {
                    fseek(output_file, -strlen(output_buffer), SEEK_CUR);
                }

                // store the value to write
                output_value_to_read = strtof(output_buffer, NULL);
                output_value_to_write = output_value_to_read + beta_cal_result[i][j]; // convert to float
                if (output_value_to_write < output_value_to_read + beta_cal_result[i][j]) { // check overflow
                    output_value_to_write = output_value_to_write + 1;
                }

                // write to output file
                fprintf(output_file, "%d\n", output_value_to_write);

                // flush the buffer
                for (int m = 0; m < MAX_LINE_LENGTH; m++) {
                    output_buffer[m] = '\0';
                }

                global_lock = 0;
            }
        }
    }
}

// thread function
void* thread_function (void* thread_num) {
    pthread_mutex_lock(&global_checkpointing);

    int first_thread_file = (intptr_t) thread_num * files_per_thread;
    char buffer[buffer_size]; // buffer for each time byte reading
    float buffer_float_list[files_per_thread][MAX_LINE_LENGTH]; // converted buffer (float) list
    int buffer_float_list_length = 0;
    int thread_file_buffer_length[files_per_thread]; // each thread file buffer length

    float alpha_cal_result[files_per_thread][MAX_LINE_LENGTH];
    float beta_cal_result[files_per_thread][MAX_LINE_LENGTH];

    int continue_to_read = 1;
    int complete_reading_thread_file[files_per_thread];
    int complete_reading_all = 0;

    int local_checkpointing = 0; // local lock

    // store all input files
    while (continue_to_read == 1) {    
        for (int i = 0; i < files_per_thread; i++) {
            // local lock
            while (local_checkpointing == 1);
            local_checkpointing = 1;

            int current_thread_file = i + first_thread_file;

            // read buffer from the input file
            fread(buffer, buffer_size, 1, input_files[current_thread_file]);

            // if reach the end of the input file
            if (feof(input_files[current_thread_file])) {
                complete_reading_thread_file[i] = 1;
                if (buffer[0] == '\0') { // when the line reaches end and is empty
                    buffer_float_list[i][buffer_float_list_length] = 0;
                } else {
                    if (buffer[0] != '\n') {
                        thread_file_buffer_length[i]++;
                    }
                    remove_newline(buffer);
                    buffer_float_list[i][buffer_float_list_length] = strtof(buffer, NULL);
                }                
            }

            // store the buffer to the buffer (float) list
            if (complete_reading_thread_file[i] != 1) {
                remove_newline(buffer);
                buffer_float_list[i][buffer_float_list_length] = strtof(buffer, NULL);
                thread_file_buffer_length[i]++;
                
            }

            // flush buffer
            for (int j = 0; j < buffer_size; j++) {
                buffer[j] = '\0';
            }
            
            local_checkpointing = 0;
        }

        buffer_float_list_length++;
        
        // check if all files are read
        complete_reading_all = 1;
        for (int i = 0; i < files_per_thread; i++) {
            if (complete_reading_thread_file[i] != 1) {
                complete_reading_all = 0;
            }
        }

        // stop reading
        if (complete_reading_all == 1) {
            continue_to_read = 0;
            buffer_float_list_length = max(thread_file_buffer_length, files_per_thread);
        }
    }

    // calculate alpha and beta
    for (int i = 0; i < files_per_thread; i++) {
        int current_thread_file = i + first_thread_file;
        alpha_cal_result[i][0] = buffer_float_list[i][0];
        for (int m = 1; m < buffer_float_list_length; m++) {
            if (m < thread_file_buffer_length[i]) {
                alpha_cal_result[i][m] = alpha_list[current_thread_file] * buffer_float_list[i][m] + (1 - alpha_list[current_thread_file]) * alpha_cal_result[i][m-1];
            } else {
                alpha_cal_result[i][m] = 0;
            }
        }
        for (int n = 0; n < buffer_float_list_length; n++) {
            if (n < thread_file_buffer_length[i]) {
                beta_cal_result[i][n] = alpha_cal_result[i][n] * beta_list[current_thread_file];
            } else {
                beta_cal_result[i][n] = 0;
            }
        }
    }

    // write to output file (3 lock config cases)
    write_to_output_file(buffer_float_list_length, beta_cal_result);

    pthread_mutex_unlock(&global_checkpointing);

    pthread_exit(NULL);
}

void skip_bom(FILE *fp) {
    // char buffer[4];
    // fread(buffer, 1, 3, fp);
    // buffer[3] = '\0';
    // if (strcmp(buffer, "\xEF\xBB\xBF") != 0) {
    //     rewind(fp);
    // }
}

int main(int argc, char **argv) {
    // get user input
    printf("Enter buffer size: ");
    scanf("%d", &buffer_size);
    printf("Enter number of threads: ");
    scanf("%d", &num_threads);
    printf("Enter lock config (1: global lock, 2: entry lock, 3: compare and swap): ");
    scanf("%d", &lock_config);

    if (lock_config != 1 && lock_config != 2 && lock_config != 3) {
        printf("Invalid lock config!\n");
        exit(1);
    }

    pthread_t thread[num_threads];
    
    FILE *metadataFile = fopen(metadata_file_path, "r");
    if (metadataFile == NULL) {
        printf("Error opening metadata file!\n");
        exit(1);
    }
    
    // skip BOM if exists
    // skip_bom(metadataFile);

    // read metadata file
    fscanf(metadataFile, "%d/r/n", &num_input_files);
    if (num_input_files % num_threads != 0) {
        printf("file is not in multiple of threads\n");
        fclose(metadataFile);
        exit(1);
    }

    char input_file_path[num_input_files][MAX_LINE_LENGTH];
    alpha_list = malloc(sizeof(float) * num_input_files);
    beta_list = malloc(sizeof(float) * num_input_files);
    input_files = malloc(sizeof(FILE*) * num_input_files);

    // store input file path, alpha and beta values
    files_per_thread = num_input_files/num_threads; // number of files per thread
    for (int i = 0; i < num_input_files; i++) {
        fscanf(metadataFile, "%s/r/n", input_file_path[i]); // store input file path
        input_files[i] = fopen(input_file_path[i], "r"); // open input file
        fscanf(metadataFile, "%f/r/n", &alpha_list[i]); // store alpha value
        fscanf(metadataFile, "%f/r/n", &beta_list[i]); // store beta value
    }

    output_file = fopen(output_file_path, "w+");

    pthread_mutex_init(&global_checkpointing, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&thread[i], NULL, thread_function, (void*)(intptr_t)i);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread[i], NULL);
    }

    pthread_mutex_destroy(&global_checkpointing);

    printf("All threads finished\n");

    // free memory
    fclose(metadataFile);
    fclose(output_file);
    for (int i = 0; i < num_input_files; i++) {
        fclose(input_files[i]);
    }
    free(alpha_list);
    free(beta_list);
    free(input_files);

    return 0;
}
