#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> // lstat
#include <sys/types.h> // lstat
#include <unistd.h> // readlink
#include <grp.h> // getgrgid
#include <pwd.h> // getpwuid
#include <time.h> // strftime
#include <dirent.h> // readdir
#include <getopt.h> // getopt

int option_i = 0;
int option_l = 0;
int option_R = 0;
int option_count = 0; // 1 for ./myls

#define PATH_MAX_LENGTH 1024

void print_option_i(struct stat *file_buffer) {
    printf("%ld ", file_buffer->st_ino);
}

void print_option_l(struct stat *file_buffer) {
    struct group *grp = getgrgid(file_buffer->st_gid);
    struct passwd *pw = getpwuid(file_buffer->st_uid);

    if (S_ISLNK(file_buffer->st_mode) == 1) {
        printf("l");
    }
    else {
        printf((S_ISDIR(file_buffer->st_mode)) ? "d":"-");
    }

    printf((file_buffer->st_mode & S_IRUSR) ? "r" : "-");
    printf((file_buffer->st_mode & S_IWUSR) ? "w" : "-");
    printf((file_buffer->st_mode & S_IXUSR) ? "x" : "-");
    printf((file_buffer->st_mode & S_IRGRP) ? "r" : "-");
    printf((file_buffer->st_mode & S_IWGRP) ? "w" : "-");
    printf((file_buffer->st_mode & S_IXGRP) ? "x" : "-");
    printf((file_buffer->st_mode & S_IROTH) ? "r" : "-");
    printf((file_buffer->st_mode & S_IWOTH) ? "w" : "-");
    printf((file_buffer->st_mode & S_IXOTH) ? "x" : "-");
    printf(" ");

    // print time in the format of "MMM DD YY HH:MM"
    struct tm time;
    time_t t = file_buffer->st_mtime;
    char str_time[256];
    localtime_r(&t,&time);
    strftime(str_time, sizeof str_time, "%b %d %Y %H:%M", &time);

    printf("%-d %-s %-s %-8ld %s ", file_buffer->st_nlink, pw->pw_name, grp->gr_name, file_buffer->st_size, str_time);
}

int compareEntries(const void *a, const void *b) {
    struct dirent *entry_a = *(struct dirent **)a;
    struct dirent *entry_b = *(struct dirent **)b;
    return strcmp(entry_a->d_name, entry_b->d_name);
}

void list_dir(const char *path, int is_only_option, int is_first_dir_arg) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_buffer;
    char full_path_R[PATH_MAX_LENGTH][PATH_MAX_LENGTH];
    int full_path_R_count = 0;
    
    dir = opendir(path);
    if (dir == NULL) {
        printf("Error: Nonexistent files or directories\n");
        exit(-1);
    }

    if (!is_first_dir_arg) {
        printf("\n");
    }

    if (!is_only_option) {
        printf("%s:\n", path);
    }

    // sort
    int num_entries = 0;
    struct dirent **entries = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { // store the entries in the array
            continue;
        } else {
            num_entries++;
            entries = realloc(entries, sizeof(struct dirent *) * num_entries);
            entries[num_entries - 1] = entry;
        }
    }
    qsort(entries, num_entries, sizeof(struct dirent *), compareEntries); // qsort the entries

    for (int i = 0; i < num_entries; i++) {
        entry = entries[i];

        if (option_i == 0 && option_l == 0 && option_R == 0) { // just a ./myls
            printf ("%s\n", entry->d_name);
        }

        else {
            char full_path[PATH_MAX_LENGTH];
            snprintf(full_path, sizeof full_path, "%s/%s", path, entry->d_name);

            if (lstat(full_path, &file_buffer) != 0) {
                printf("Error: Nonexistent files or directories\n");
                exit(-1);
            }

            // if the option i
            if (option_i) {
                print_option_i(&file_buffer);
            }

            // if the option l
            if (option_l) {
                print_option_l(&file_buffer);
            }

            // if the option R: list subdirectories recursively
            if (option_R) {
                if (S_ISDIR(file_buffer.st_mode) == 1) {
                    strcpy(full_path_R[full_path_R_count], full_path);
                    full_path_R_count++;
                }
            }

            // if the entry is a symbolic link
            if (S_ISLNK(file_buffer.st_mode)) {
                char link_path[PATH_MAX_LENGTH];
                int link_path_length = readlink(full_path, link_path, sizeof link_path);
                link_path[link_path_length] = '\0';
                printf("%s -> %s\n", entry->d_name, link_path);
            }
            
            else {
                printf("%s\n", entry->d_name);
            }
        }
    }

    // if the option R: list subdirectories recursively
    if (option_R == 1) {
        for (int i = 0; i < full_path_R_count; i++) {
            list_dir(full_path_R[i], is_only_option, 0);
        }
    }

    closedir(dir);
    free(entries);
}

int main(int argc, char **argv) {
    if (strcmp(argv[0], "./myls") != 0) {
        printf("Error: Unsupported Option\n");
        exit(-1);
    }
    option_count = 1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            option_count++;
            for (int j = 1; argv[i][j] != '\0'; j++) {
                char option = argv[i][j];
                switch (option) {
                    case 'i':
                        option_i = 1;
                        break;
                    case 'l':
                        option_l = 1;
                        break;
                    case 'R':
                        option_R = 1;
                        break;
                    default:
                        printf("Error: Unsupported Option\n");
                        exit(-1);
                }
            }
        }
    }

    struct stat file_buffer;
    char file_path[PATH_MAX_LENGTH];

    int is_first_dir_arg = 0;
    int is_first_dir_arg_count = 0;
    int is_only_option = 0;
   
    // Only options
    if (argc == option_count) {
        is_only_option = 1;
        is_first_dir_arg = 1;
        list_dir(".", is_only_option, is_first_dir_arg);
    }

    // Also has filelist
    else {
        for (int i = option_count; i < argc; i++) {
            memset(&file_buffer, 0, sizeof file_buffer);
            strcpy(file_path, argv[i]);

            if(lstat(file_path, &file_buffer) !=0) {
                printf("\nError: Nonexistent files or directories\n");
                exit(-1); 
            }

            // if the entry is a file or a symbolic link
            if (S_ISREG(file_buffer.st_mode) || S_ISLNK(file_buffer.st_mode)) {
                if (option_i) {
                    print_option_i(&file_buffer);
                }

                if (option_l) {
                    print_option_l(&file_buffer);
                }

                if (S_ISLNK(file_buffer.st_mode)) {
                    char link_path[PATH_MAX_LENGTH];
                    int link_path_length = readlink(file_path, link_path, sizeof link_path);
                    link_path[link_path_length] = '\0';
                    
                    // remove './' in the beginning of the path
                    if (file_path[0] == '.' && file_path[1] == '/') {
                        memmove(file_path, file_path + 2, strlen(file_path) - 1);
                    }
                    printf("%s -> %s\n", file_path, link_path);       
                }

                else {
                    // remove './' in the beginning of the path
                    if (file_path[0] == '.' && file_path[1] == '/') {
                        memmove(file_path, file_path + 2, strlen(file_path) - 1);
                    }
                    printf("%s\n", file_path);
                }
            }

            // if the entry is a directory
            if (S_ISDIR(file_buffer.st_mode)) {
                if (is_first_dir_arg_count == 0) { // if it is the first file argument
                    is_first_dir_arg = 1;
                } else {
                    is_first_dir_arg = 0;
                }
                is_first_dir_arg_count++;

                if (file_path[0] == '.' && file_path[1] == '\0') {
                    is_only_option = 1;
                }

                list_dir(file_path, is_only_option, is_first_dir_arg);
            }
        }
    }
    
    return 0;
}
