#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <sys/mman.h>


#include <sys/types.h>
#include <unistd.h>

// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  unsigned int magic;  // must equal ELF_MAGIC
  unsigned char elf[12];
  unsigned short type;
  unsigned short machine;
  unsigned int version;
  unsigned int entry;
  unsigned int phoff;
  unsigned int shoff;
  unsigned int flags;
  unsigned short ehsize;
  unsigned short phentsize;
  unsigned short phnum;
  unsigned short shentsize;
  unsigned short shnum;
  unsigned short shstrndx;
};

// Program section header
struct proghdr {
  unsigned int type;
  unsigned int off;
  unsigned int vaddr;
  unsigned int paddr;
  unsigned int filesz;
  unsigned int memsz;
  unsigned int flags;
  unsigned int align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4


int main(int argc, char* argv[]) {
    struct elfhdr elf;
    struct proghdr ph;
    int (*sum)(int a, int b);
    void *entry = NULL;
    int ret; 

    /* Add your ELF loading code here */
    char cwd[4096];  //path_max
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/");
    strcat(cwd, argv[1]);


    FILE* file = fopen(cwd, "rb");
    fread(&elf, sizeof(elf),1, file);

    fseek(file, elf.phoff, SEEK_SET);
    int i;
    for(i=0; i<elf.phnum;i++){
      fread(&ph, 1, sizeof(ph), file);
      if (ph.type==ELF_PROG_LOAD){
        break;
      }
      fseek(file, elf.phentsize, SEEK_CUR);
    }

    fclose(file);
     
    entry = mmap(NULL, ph.memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
              MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    int f = open(cwd, O_RDWR);
    lseek(f, elf.entry + ph.off, SEEK_CUR);
    read(f, entry, ph.memsz);
    close(f);

    
    if (entry != NULL) {
        sum = entry;
        ret = sum(1, 2);
        printf("sum:%d\n", ret); 
    };

    munmap(entry, ph.memsz);

    return 0; //just for the warnings

}


