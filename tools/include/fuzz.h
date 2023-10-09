#include <libelf.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <gelf.h>
#include <unistd.h>

uint64_t MAX_KVM_ARCH;
uint64_t MAX_KVM;

#define INPUT_READY 8000
#define EXEC_DONE 8001
#define KILL_QEMU 8002
#define QEMU_READY 8004
#define VMCS_READY 8005

static uint64_t check_text_size(char *filepath) {
    Elf         *elf;
    Elf_Scn     *scn = NULL;
    GElf_Shdr   shdr;
    int         fd;
    size_t      shstrndx;  // Section header string table index

    // Open the file
    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (elf_version(EV_CURRENT) == EV_NONE) {
        // library out of date
        exit(1);
    }

    elf = elf_begin(fd, ELF_C_READ, NULL);
    
    // Retrieve the section header string table index
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        perror("elf_getshdrstrndx");
        exit(1);
    }

    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            // error
            exit(1);
        }

        if (shdr.sh_type == SHT_PROGBITS) {
            char *name;
            name = elf_strptr(elf, shstrndx, shdr.sh_name);  // Use shstrndx
            if (name && strcmp(name, ".text") == 0) {
                break;
            }
        }
    }

    elf_end(elf);
    close(fd);

    return (uint64_t)shdr.sh_size;
}

void check_cpu_vendor(void) {
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    char buffer[255];
    char vendor[16];
    struct utsname utbuffer;
    char filepath[128];

    if (cpuinfo == NULL) {
        perror("fopen");
        return;
    }

    if (uname(&utbuffer) != 0) {
        perror("uname");
        return;
    }

    snprintf(filepath, 128, "/usr/lib/modules/%s/kernel/arch/x86/kvm/kvm.ko", utbuffer.release);

    MAX_KVM = check_text_size(filepath);

    while (fgets(buffer, 255, cpuinfo)) {
        if (strncmp(buffer, "vendor_id", 9) == 0) {
            sscanf(buffer, "vendor_id : %s", vendor);

            if (strcmp(vendor, "GenuineIntel") == 0) {
                snprintf(filepath, 128, "/usr/lib/modules/%s/kernel/arch/x86/kvm/kvm-intel.ko", utbuffer.release);
                MAX_KVM_ARCH = check_text_size(filepath);
            } else if (strcmp(vendor, "AuthenticAMD") == 0) {
                snprintf(filepath, 128, "/usr/lib/modules/%s/kernel/arch/x86/kvm/kvm-amd.ko", utbuffer.release);
                MAX_KVM_ARCH = check_text_size(filepath);
            } else {
                printf("This is a CPU from another vendor: %s\n", vendor);
                // default value or another value
                MAX_KVM_ARCH = 0;
            }

            break;
        }
    }

    fclose(cpuinfo);
}