#include <yaml.h>

typedef struct {
    char qemu_path[128];
    char work_dir[128];
    char covout_dir[128];
    char fuzzinput_dir[128];
    char xen_dir[128];
} path_config_t;


path_config_t * parse_config(char *path) {
    path_config_t *path_config;
    path_config = malloc(sizeof(path_config_t)); 
    yaml_parser_t parser;
    yaml_event_t event;   // Variables for parsing
    
    int level = 0;
    char level1_key[128] = {0};
    char level2_key[128] = {0};
    char *value;
    FILE *fh = fopen(path, "r");
    if (fh == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        return NULL;
    }
    // Initialize parser
    if(!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize parser!\n");
        fclose(fh);
        return NULL;
    }

    // Set input file
    yaml_parser_set_input_file(&parser, fh);

    // Start parsing events
    while(1) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "Parser error %d\n", parser.error);
            fclose(fh);
            yaml_parser_delete(&parser);
            return NULL;
        }


        switch(event.type) {
        case YAML_SCALAR_EVENT:
            if (level == 1) {
                strncpy(level1_key, (char *)event.data.scalar.value, sizeof(level1_key) - 1);
                level1_key[sizeof(level1_key) - 1] = '\0';
            } else if (level == 2) {
                if (strcmp(level1_key, "program") == 0 && strcmp(level2_key, "qemu") == 0) {
                    strncpy(path_config->qemu_path, (char *)event.data.scalar.value, sizeof(path_config->qemu_path) - 1);
                    path_config->qemu_path[sizeof(path_config->qemu_path) - 1] = '\0';
                } else if (strcmp(level1_key, "directories") == 0) {
                    if (strcmp(level2_key, "work_dir") == 0) {
                        strncpy(path_config->work_dir, (char *)event.data.scalar.value, sizeof(path_config->work_dir) - 1);
                        path_config->work_dir[sizeof(path_config->work_dir) - 1] = '\0';
                    } else if (strcmp(level2_key, "covout_dir") == 0) {
                        strncpy(path_config->covout_dir, (char *)event.data.scalar.value, sizeof(path_config->covout_dir) - 1);
                        path_config->covout_dir[sizeof(path_config->covout_dir) - 1] = '\0';
                    } else if (strcmp(level2_key, "fuzzinput_dir") == 0) {
                        strncpy(path_config->fuzzinput_dir, (char *)event.data.scalar.value, sizeof(path_config->fuzzinput_dir) - 1);
                        path_config->fuzzinput_dir[sizeof(path_config->fuzzinput_dir) - 1] = '\0';
                    } else if (strcmp(level2_key, "xen_dir") == 0) {
                        strncpy(path_config->xen_dir, (char *)event.data.scalar.value, sizeof(path_config->xen_dir) - 1);
                        path_config->xen_dir[sizeof(path_config->xen_dir) - 1] = '\0';
                    }
                }
                strncpy(level2_key, (char *)event.data.scalar.value, sizeof(level2_key) - 1);
                level2_key[sizeof(level2_key) - 1] = '\0';

            }
            break;
        case YAML_MAPPING_START_EVENT:
            level++;
            break;
        case YAML_MAPPING_END_EVENT:
            if (level == 2) {
                memset(level2_key, 0, sizeof(level2_key)); 
            } else if (level == 1) {
                memset(level1_key, 0, sizeof(level1_key)); 
            }
            level--;
            break;
        default:
            break;
        }

        if (event.type == YAML_STREAM_END_EVENT) {
            break;
        }
        yaml_event_delete(&event);
    }

    // Cleanup
    yaml_parser_delete(&parser);
    fclose(fh);
    return path_config;
}