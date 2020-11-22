#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h> 
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

void func(char* id, int n, ...){
    va_list nums;
    int acc = 0;
    va_start(nums, n);
    for(int i = 0 ; i < n ; ++i){
        acc += va_arg(nums, int);
    }
    va_end(nums);
    fprintf(stderr, "ID: %s, acc = %d", id, acc);
}

int main(){
    char buf[1024];
    int a;
    int b;
    
    // func("Mike", 5, 1, 2, 3, 4, 5);

    // int fds[2];
    // pipe2(fds, FD_CLOEXEC);
    // fprintf(stderr, "%d %d\n", fds[0], fds[1]);

    fgets(buf, sizeof(buf), stdin);
    sscanf(buf, "%d", &a);
    sscanf(buf, "%d", &b);
    fprintf(stderr, "a: %d b: %d", a, b);

    return 0;
}
