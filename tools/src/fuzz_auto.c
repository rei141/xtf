#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdint.h>
#include <semaphore.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <signal.h>
// #include <process.h>
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>
#include "fuzz.h"
#include "my_yaml.h"
#include "args.h"

#define BINC_HEADER "binc.h"
#define BINC_SOURCE "binc.c"

uint8_t *kvm_arch_coverage, *kvm_coverage;
path_config_t *path_config;

int create_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0) {
        } else {
            fprintf(stderr, "mkdir failed\n");
            return 1;
        }
    }

    return 0;
}

int save_input(uint8_t * ivmshm) {
    struct timeval tv;
    struct tm *tm;
    char d_name[192] = {0};
    char f_name[256] = {0};
    struct stat st;
    FILE * fp;

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);

    sprintf(d_name,"%s/%02d_%02d_%02d",path_config->fuzzinput_dir, tm->tm_mon+1, tm->tm_mday,tm->tm_hour);
    if (create_directory(d_name))
        return 1;

    sprintf(f_name,"%s/input_%02d_%02d_%02d_%02d_%02d", d_name, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    fp = fopen(f_name,"w");
    if (fp == NULL) {
        fprintf(stderr, "fopen failed\n");
        return 1;
    }
    fwrite(ivmshm,sizeof(uint8_t),4096,fp);
    fclose(fp);
    return 0;
}

int create_binc(char *input_file, char *srcdir) {
    FILE *f = fopen(input_file, "rb");
    if (!f) {
        perror("Error opening input file");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *data = malloc(size);
    if (!data) {
        perror("Error allocating memory");
        fclose(f);
        return 1;
    }

    fread(data, 1, size, f);
    fclose(f);

    char *header_file = malloc(strlen(srcdir) + strlen(BINC_HEADER));
    if (!header_file) {
        perror("Error allocating memory");
        return 1;
    }
    strcpy(header_file, srcdir);
    strcat(header_file, BINC_HEADER);

    // Write header file
    f = fopen(header_file, "w");
    if (!f) {
        perror("Error opening header file");
        free(data);
        return 1;
    }

#include <stdint.h>
    fprintf(f, "#pragma once\n");
    fprintf(f, "#include <stdint.h>\n\n");
    fprintf(f, "#ifndef BINARY_DATA_H\n");
    fprintf(f, "#define BINARY_DATA_H\n\n");
    fprintf(f, "#define BINARY_DATA_SIZE %ld\n\n", size);
    fprintf(f, "extern uint8_t binary_data[BINARY_DATA_SIZE];\n\n");
    fprintf(f, "#endif // BINARY_DATA_H\n");

    fclose(f);
    printf("create %s\n", header_file);
    // Write source file
    char *source_file = malloc(strlen(srcdir) + strlen(BINC_SOURCE));
    if (!source_file) {
        perror("Error allocating memory");
        return 1;
    }
    strcpy(source_file, srcdir);
    strcat(source_file, BINC_SOURCE);
    
    f = fopen(source_file, "w");
    if (!f) {
        perror("Error opening source file");
        free(data);
        return 1;
    }

    fprintf(f, "#include \"binc.h\"\n\n");
    fprintf(f, "uint8_t binary_data[] = {\n");

    for (long i = 0; i < size; i++) {
        fprintf(f, "0x%02x", data[i]);
        if (i < size - 1) fprintf(f, ",");
    }

    fprintf(f, "\n};\n");
    fclose(f);
    printf("create %s\n", source_file);
    free(data);
    free(header_file);
    free(source_file);
    return 0;
}
void execute_command(const char *command) {
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to execute command: %s\n", command);
        // exit(EXIT_FAILURE);
    }
}
void execute_command_with_fallback(const char *main_command, const char *fallback_command) {
    int result = system(main_command);
    if (result != 0) {
        fprintf(stderr, "Failed to execute main command: %s\n", main_command);
        execute_command(fallback_command);
    }
}
void execute_command_with_extra(const char *main_command, const char *fallback_command) {
    int result = system(main_command);
    if (result == 0) {
        execute_command(fallback_command);
    }
}
char xencov_name[256];
char gcov_name[256];
int get_xencov() {
    struct timeval tv;
    struct tm *tm;
    char d_name[192] = {0};
    char command[256] = {0};
    struct stat st;

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);

    sprintf(d_name,"%s/%02d_%02d_%02d",path_config->covout_dir, tm->tm_mon+1, tm->tm_mday,tm->tm_hour);
    if (create_directory(d_name))
        return 1;

    sprintf(xencov_name,"%s/xencov_%02d_%02d_%02d_%02d_%02d", d_name, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    sprintf(gcov_name,"%s/gcov_%02d_%02d_%02d_%02d_%02d", d_name, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    sprintf(command,"sudo xencov read > %s/xencov_%02d_%02d_%02d_%02d_%02d", d_name, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    execute_command(command);
    return 0;
}

int ind;
#define BITMAP_SIZE 65536 // 64kB

int process_gcov_file(const char *filename, uint8_t *bitmap) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return 0;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *colon_ptr = strchr(line, ':');
        if (!colon_ptr) continue; // ":"がない場合、この行を無視
        if(colon_ptr[-1] == '-'){
            continue;
        }

        // ":"より左の部分を抽出
        char left_part[256];
        strncpy(left_part, line, colon_ptr - line);
        left_part[colon_ptr - line] = '\0';
        // 左部分が"#####"かどうかをチェック
        if (strstr(left_part, "#####")) {
            // printf("%d Line not covered\n",ind);
            ind += 1;
            continue;
        }

        char *ptr = left_part;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++; // 空白をスキップ
        if (*ptr) {
            char *end_ptr;
            long count = strtol(ptr, &end_ptr, 10); // 数字を解析
            if (count >= 128)
                bitmap[ind] |= 0x80;
            else if (count >= 32)
                bitmap[ind] |= 0x40;
            else if (count >= 16)
                bitmap[ind] |= 0x20;
            else if (count >= 8)
                bitmap[ind] |= 0x10;
            else if (count >= 4)
                bitmap[ind] |= 0x08;
            else if (count >= 3)
                bitmap[ind] |= 0x04;
            else if (count >= 2)
                bitmap[ind] |= 0x02;
            else if (count >= 1)
                bitmap[ind] |= 0x01;
            // printf("byte #%d = 0x%x\n", ind, bitmap[ind]);
            ind += 1;
        }
        if (ind >= BITMAP_SIZE){
            printf("index reached BITMAP_SIZE with %s", filename);
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int main(int argc, char** argv) {
    FILE *input_fp;
    int shm_fd, bitmap_fd, kvm_arch_fd, kvm_fd, err;
    uint8_t *afl_bitmap;
    config_t *config;
    uint8_t buf[4096];
    int afl_shm_id;
    const char *afl_shm_id_str = getenv("__AFL_SHM_ID");
    uint8_t *afl_area_ptr = NULL;
    if (afl_shm_id_str != NULL) {
        afl_shm_id = atoi(afl_shm_id_str);
        afl_area_ptr = shmat(afl_shm_id, NULL, 0);
    }
    
    // check_cpu_vendor();
    config = create_config(argc, argv);
    if(config == NULL) 
        return 1;
    path_config = parse_config(config->yaml_config_name);
    if (path_config != NULL) {
        if (create_directory(path_config->covout_dir))
            return 1;
        if (create_directory(path_config->fuzzinput_dir))
            return 1;
    }

    // bitmap_fd = shm_open(config->bitmap_name, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
    // if (bitmap_fd == -1) {
    //     fprintf(stderr, "shm_open failed\n");
    //     return 1;
    // }
    // err = ftruncate(bitmap_fd, 65536);
    // if(err == -1){
    //     fprintf(stderr, "ftruncate failed\n");
    //     return 1;
    // }

    // afl_bitmap = (uint8_t *)mmap(NULL, 65536,
    //                                 PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    // if ((void *)afl_bitmap == MAP_FAILED) {
    //     fprintf(stderr, "mmap failed\n");
    //     return 1;
    // }

    input_fp = fopen(config->afl_input_name, "rb");
    if (input_fp == NULL) {
        fprintf(stderr, "fopen failed\n");
        return 1;
    }
    fread(buf, sizeof(uint8_t), 4096, input_fp);
    if (save_input(buf))
        fprintf(stderr, "save_input() failed\n");

    int ret = create_binc(config->afl_input_name, config->srcdir_name);
    
    execute_command("make necofuzz");
    execute_command("sudo xencov reset");
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) { // 子プロセス
        close(pipefd[0]); // 読み込み用の端を閉じる
        dup2(pipefd[1], STDOUT_FILENO); // 標準出力をパイプにリダイレクト
        close(pipefd[1]); // 書き込み用の端を閉じる

        execlp("sudo", "sudo", "xl", "create", "tests/necofuzz/test-hvm64-necofuzz.cfg", "-c", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    close(pipefd[1]); // 書き込み用の端を閉じる
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

    char buffer[4096];
    int no_output_seconds = 0;
    int total_seconds = 0;
    int vmx_bench_found = 0;


    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(pipefd[0], &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0; // 1秒
        timeout.tv_usec = 0.1*1000*1000;
        int ret = select(pipefd[0] + 1, &readfds, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("select");
            break;
        }

        if (ret == 0) { // タイムアウト
            no_output_seconds++;
            total_seconds++;

            if (!vmx_bench_found && total_seconds >= 20) {
                printf("(!vmx_bench_found && total_seconds >= 2) \n");
                break;
            }

            if (vmx_bench_found && no_output_seconds >= 10) {
                printf("(vmx_bench_found && no_output_seconds >= 1) \n");
                break;
            }

            continue;
        }

        ssize_t len = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // ノンブロッキングモードでデータがない場合
            }
            perror("read");
            break;
        }

        if (len == 0) {
            printf("len = 0\n");
            break; // EOF
        }

        buffer[len] = '\0';
        printf("%s", buffer); // バッファの内容を出力

        if (!vmx_bench_found && strstr(buffer, "necofuzz start")) {
            printf("necofuzz found!\n");
            vmx_bench_found = 1;
        }

        no_output_seconds = 0; // カウンタをリセット
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0); // 子プロセスの終了を待つ
    char command[312] = {0};
    struct timeval tv;
    struct tm *tm;
    char total_cov_path[256] = {0};
    char f_name[256] = {0};
    FILE * total_cov_file;
    FILE * fp;
        gettimeofday(&tv, NULL);
        tm = localtime(&tv.tv_sec);
        
    get_xencov();
    
    sprintf(command,"xencov_split %s --output-dir=/ > /dev/null", xencov_name);
    execute_command(command);

    char tmpcov_name[] = "/tmp/tmp.gcov";
    execute_command("rm /tmp/tmp.gcov -f");
    sprintf(command,"cd %s && find . -name \"*.gcda\" -exec gcov-12 -t {} + > %s", path_config->xen_dir, tmpcov_name);
    execute_command(command);
    // printf("%s\n",command);
    sprintf(command,"mv %s %s", tmpcov_name, gcov_name);
    execute_command(command);

    if(afl_area_ptr) {
        process_gcov_file(gcov_name, afl_area_ptr);
    }
    else {
        uint8_t *bitmap = malloc(BITMAP_SIZE);
        process_gcov_file(gcov_name, bitmap);
    }

    if (afl_shm_id_str != NULL) {
        shmdt(afl_area_ptr);
    }

    close(shm_fd);
    free(config);
    free(path_config);
    execute_command_with_extra("sudo xl list | grep necofuzz | grep r", "sudo xl destroy test-hvm64-necofuzz");
    execute_command("while sudo xl list | grep -q necofuzz; do sleep 0.1; done");
    return 0;
}