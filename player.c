#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h> 
#include <unistd.h>

#define GET_BID(pid, r) BID_LIST[pid + r - 2] * 100
#define ROUND 10
#define MAX_PID 12
#define MIN_PID 1

const int BID_LIST[21] = {
    20, 18, 5, 21, 8, 7, 2, 19, 14, 13,
    9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3
};
int player_id;
int auc_round;

int check_valid_pid(char* pid);

int main(int argc, char** argv) {
    if(argc != 2){
        fprintf(stderr, "Invalid number of arguments: %d\n", argc);
        exit(0);
    }


    player_id = -1;
    auc_round = 1;

    if((player_id = check_valid_pid(argv[1])) == 0){
        exit(0);
    }
    else{
        for(; auc_round <= ROUND ; ++auc_round){
            if(player_id == -1)
                printf("%d %d\n", player_id, player_id);
            else
                printf("%d %d\n", player_id, GET_BID(player_id, auc_round));
            fflush(stdout);
        }
        exit(0);
    }
}

int check_valid_pid(char* pid_str){
    char* pEnd;
    long pid = strtol(pid_str, &pEnd, 10);

    if (pEnd == pid_str || *pEnd != '\0' ||
        ((pid == LONG_MIN || pid == LONG_MAX) && errno == ERANGE)){
        fprintf(stderr, "Player id is invalid: %s\n", pid_str);
        return 0;
    }
    else if(pid == -1){
        fprintf(stderr, "End of auction\n");
    }
    else if(pid > MAX_PID || pid < MIN_PID){
        fprintf(stderr, "Player id [%ld] is out of index\n", pid);
        return 0;
    }
    fprintf(stderr, "Player id is [%ld]\n", pid);
    return pid;
}