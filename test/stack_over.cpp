#include <pthread.h>
#include <sys/mman.h>
#include <cstdio>

void recursive_stack(long long* m_, int i) {
    if (i >= 100) {
        return;
    }

    long long m[512];
    printf("-----%p\n", &m);
    i++;
    recursive_stack(m, i);
}

static int offset = 1 << 13;

void* f(void *stack) {
    long long i = 1;
    i = ((long long)(&i) - offset) & (~(offset-1));

    recursive_stack(nullptr, 0);
    madvise(stack, (i - (long long)stack)/sizeof(char), MADV_DONTNEED);

    recursive_stack(nullptr, 0);
    madvise(stack, (i - (long long)stack)/sizeof(char), MADV_DONTNEED);

    recursive_stack(nullptr, 0);
    madvise(stack, (i - (long long)stack)/sizeof(char), MADV_DONTNEED);

    recursive_stack(nullptr, 0);
    madvise(stack, (i - (long long)stack)/sizeof(char), MADV_DONTNEED);
    return NULL;
}

int main(int argc, char** argv) {
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int size = 32 * 1024 * 1024;  // 32M
    // int size = 1024*1024;  // 32M
    void* stack = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("%p---%p\n", stack, ((char*)stack+size));
    pthread_attr_setstack(&attr, stack, size);
  
    pthread_create(&tid, &attr, f, stack);

    void *status;
    pthread_join(tid, &status);
    return 0;
}



/************
 * grep 7ffff4d93000 -A20 /proc/`pgrep stack_over`/smaps 
 * 
 */