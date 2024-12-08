#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

void parent_process(int *BankAccount, sem_t *mutex) {
    srand(time(NULL) ^ (getpid()<<16));
    int localBalance;
    
    while(1) {
        // Sleep random amount between 0-5 seconds
        sleep(rand() % 6);
        printf("Dear Old Dad: Attempting to Check Balance\n");
        
        // Generate random number to decide action
        int action = rand() % 2;  // 0 for even, 1 for odd
        
        sem_wait(mutex);
        localBalance = *BankAccount;
        
        if (action == 0) {  // Even - might deposit
            if (localBalance < 100) {
                // Generate random amount between 0-100
                int amount = rand() % 101;
                
                if (rand() % 2 == 0) {  // Even - will deposit
                    localBalance += amount;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", 
                           amount, localBalance);
                    *BankAccount = localBalance;
                } else {  // Odd - won't deposit
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", 
                       localBalance);
            }
        } else {  // Odd - just check balance
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        
        sem_post(mutex);
    }
}

void child_process(int *BankAccount, sem_t *mutex) {
    srand(time(NULL) ^ (getpid()<<16));
    int localBalance;
    
    while(1) {
        // Sleep random amount between 0-5 seconds
        sleep(rand() % 6);
        printf("Poor Student: Attempting to Check Balance\n");
        
        // Generate random number to decide action
        int action = rand() % 2;  // 0 for even, 1 for odd
        
        sem_wait(mutex);
        localBalance = *BankAccount;
        
        if (action == 0) {  // Even - attempt to withdraw
            // Generate random need between 0-50
            int need = rand() % 51;
            printf("Poor Student needs $%d\n", need);
            
            if (need <= localBalance) {
                localBalance -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", 
                       need, localBalance);
                *BankAccount = localBalance;
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
        } else {  // Odd - just check balance
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        
        sem_post(mutex);
    }
}

int main() {
    // Create shared memory for bank account
    int *BankAccount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, 
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (BankAccount == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    
    // Initialize bank account
    *BankAccount = 0;
    printf("Orig Bank Account = %d\n", *BankAccount);
    
    // Create named semaphore
    sem_t *mutex = sem_open("/bank_sem", O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open failed");
        munmap(BankAccount, sizeof(int));
        exit(1);
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        sem_close(mutex);
        sem_unlink("/bank_sem");
        munmap(BankAccount, sizeof(int));
        exit(1);
    } else if (pid == 0) {
        child_process(BankAccount, mutex);
        exit(0);
    } else {
        parent_process(BankAccount, mutex);
        wait(NULL);
    }
    
    // Cleanup
    sem_close(mutex);
    sem_unlink("/bank_sem");
    munmap(BankAccount, sizeof(int));
    return 0;
}
