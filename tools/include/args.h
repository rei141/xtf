#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#define DEFAULT_SHM "ivmshm"
#define DEFAULT_BITMAP "afl_bitmap"
#define DEFAULT_AFL_INPUT "afl_input"
#define DEFAULT_YAML_CONFIG "config.yaml"
#define DEFAULT_SRCDIR "./"

typedef struct {
    char shm_name[128];
    char bitmap_name[128];
    char afl_input_name[128];
    char yaml_config_name[128];
    char srcdir_name[128];
} config_t;

static void print_usage() {
    printf("Usage: program [-s shm_name] [-b bitmap_name] [-i afl_input_name] [-c config_file]\n");
    printf("  -s, --shm      shared memory name\n");
    printf("  -b, --bitmap   bitmap name\n");
    printf("  -i, --input    afl input file name\n");
    printf("  -c, --config   yaml config file name\n");
    printf("  -d, --srcdir   src directry name name\n");
    printf("  -h, --help     show this help message\n");
}

config_t* create_config(int argc, char **argv) {
    int c;
    config_t *config;
    config = malloc(sizeof(config_t)); 
    if (config == NULL) {
        fprintf(stderr, "malloc failed\n");
        return NULL;
    }
    strcpy(config->shm_name, DEFAULT_SHM);
    strcpy(config->bitmap_name, DEFAULT_BITMAP);
    strcpy(config->afl_input_name, DEFAULT_AFL_INPUT);
    strcpy(config->yaml_config_name, DEFAULT_YAML_CONFIG);
    strcpy(config->srcdir_name, DEFAULT_SRCDIR);
    
    
    static struct option long_options[] = {
        {"shm",      required_argument, 0, 's'},
        {"bitmap",   required_argument, 0, 'b'},
        {"input",    required_argument, 0, 'i'},
        {"config",   required_argument, 0, 'c'},
        {"srcdir",   required_argument, 0, 'c'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while ((c = getopt_long(argc, argv, "s:b:i:c:d:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 's':
                strncpy(config->shm_name, optarg, sizeof(config->shm_name) - 1);
                config->shm_name[sizeof(config->shm_name) - 1] = '\0';
                break;
            case 'b':
                strncpy(config->bitmap_name, optarg, sizeof(config->bitmap_name) - 1);
                config->bitmap_name[sizeof(config->bitmap_name) - 1] = '\0';
                break;
            case 'i':
                strncpy(config->afl_input_name, optarg, sizeof(config->afl_input_name) - 1);
                config->afl_input_name[sizeof(config->afl_input_name) - 1] = '\0';
                break;
            case 'c':
                strncpy(config->yaml_config_name, optarg, sizeof(config->yaml_config_name) - 1);
                config->yaml_config_name[sizeof(config->yaml_config_name) - 1] = '\0';
                break;
            case 'd':
                strncpy(config->srcdir_name, optarg, sizeof(config->srcdir_name) - 1);
                config->srcdir_name[sizeof(config->srcdir_name) - 1] = '\0';
                break;
            case 'h':
                print_usage();
                exit(0);
            case '?':
                print_usage();
                exit(1);
            default:
                abort();
        }
    }

    return config;
}