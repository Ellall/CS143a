 // Shell.
#include "types.h"
#include "user.h"
#include "stat.h"

int main(void){
    char * memory_pos = sbrk(16);
    wrprotect(memory_pos, 16);
    *memory_pos = 0x10;
    printf(1, "Undoing the wrprotect");
    exit();
}