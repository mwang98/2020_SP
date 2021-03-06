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
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_DEPTH 2
#define BUFF_SIZE 1024
#define N_PLAYER 8
#define N_ROUND 10
#define N_POINT 11 // 0, 1, 2, ...... 10
#define END_SIG -1
#define MAX_PID 12
#define MIN_PID 1

// receive from fifos
int host_id;
int key;
int depth;
// derived 
int n_player;
int* playerids;
// aux
int i_round;
int win_id;
int win_bid;
int* player_record;     // record number of victory
int* player_rank;       // record rank
FILE* read_l_child;
FILE* read_r_child;
FILE* write_l_child;
FILE* write_r_child;

int fdsP2CL[2]; // parent to left child
int fdsCL2P[2]; // left child to parent
int fdsP2CR[2]; // parent to right child
int fdsCR2P[2]; // right child to parent
int mystdio[2];

char fifo_r_pathname[BUFF_SIZE];
char fifo_w_pathname[] = "fifo_0.tmp";

char buf[BUFF_SIZE];

// utils
void print_playerids();
void print_fds();
void print_bucket(int row, int col, int[][col]);
int max(int, int);
void reset_buf();
void reset_round();
void reset_player_record();
void reset_player_rank();
int get_player_num();
char* tostring(int);
void read_message(FILE*, int*, int*);
void read_message_v(FILE*, int, int*);

// argument check and initialize
int check_valid_args(int argc, char** argv);
int check_strtol(char*, char*, long);
void initialize(int, int, int);
void set_child_stream();

// stdio
void get_player_ids_from_stdin();
void get_winner_from_child();
void send_player_ids_to_child();

// root only
void redirect_std_to_fifo();
void update_winner_record(int);
void prepare_ranking();
void send_ranking_to_stdout();

// general host
void redirect_std_to_pipe();
void close_pipe(int, ...);
void create_pipe(int, ...);

int main(int argc, char** argv) {
    pid_t pid_child[2];
    int status[2];

    if(!check_valid_args(argc, argv)) exit(0);
    if(depth == 0) redirect_std_to_fifo();

    create_pipe(4, fdsP2CL, fdsCL2P, fdsP2CR, fdsCR2P);
    set_child_stream();
    if(depth == 2){
        while(1){
            get_player_ids_from_stdin();
            print_playerids();

            // fork child process to execute player (fork for each turn)
            if((pid_child[0] = fork()) == 0){ // child
                fprintf(stderr, "Leaf host fork left child\n");

                redirect_std_to_pipe(fdsP2CL[0], fdsCL2P[1]);
                close_pipe(4, fdsP2CL, fdsCL2P, fdsP2CR, fdsCR2P);      
                execl("./player", "player", tostring(playerids[0]), (char*)0);
            }
            else{   // parent
                if((pid_child[1] = fork()) == 0){ // child
                    fprintf(stderr, "Leaf host fork right child\n");

                    redirect_std_to_pipe(fdsP2CR[0], fdsCR2P[1]);
                    close_pipe(4, fdsP2CL, fdsCL2P, fdsP2CR, fdsCR2P);
                    execl("./player", "player", tostring(playerids[1]), (char*)0);
                }
            }
            while(i_round++ <= N_ROUND){
                get_winner_from_child();
                printf("%d %d\n", win_id, win_bid); fflush(stdout);
            }

            reset_round();
            waitpid(pid_child[0], &status[0], 0);
            waitpid(pid_child[1], &status[1], 0);
            if(win_id == END_SIG) exit(0);
        }
    }
    else{
        // fork child process to execute other host (fork once and reuse)
        if((pid_child[0] = fork()) == 0){ // child
            if(depth == 0)
                fprintf(stderr, "Root host fork left child\n");
            else
                fprintf(stderr, "Middle host fork left child\n");

            redirect_std_to_pipe(fdsP2CL[0], fdsCL2P[1]);
            close_pipe(4, fdsP2CL, fdsCL2P, fdsP2CR, fdsCR2P);
            execl("./host", "host", tostring(host_id), tostring(key), tostring(depth+1), (char*)0);
        }
        else{   // parent
            if((pid_child[1] = fork()) == 0){ // child
                if(depth == 0)
                    fprintf(stderr, "Root host fork right child\n");
                else
                    fprintf(stderr, "Middle host fork right child\n");

                redirect_std_to_pipe(fdsP2CR[0], fdsCR2P[1]);
                close_pipe(4, fdsP2CL, fdsCL2P, fdsP2CR, fdsCR2P);
                execl("./host", "host", tostring(host_id), tostring(key), tostring(depth+1), (char*)0);
            }
        }
        while(1){
            get_player_ids_from_stdin();
            send_player_ids_to_child();
            print_playerids();

            while(i_round++ <= N_ROUND){
                get_winner_from_child();
                if(depth == 0){
                    update_winner_record(win_id);
                }
                else{
                    printf("%d %d\n", win_id, win_bid); fflush(stdout);
                }
            }
            reset_round();
            if(win_id == END_SIG) break;
            if(depth == 0){
                prepare_ranking();
                send_ranking_to_stdout();
                reset_player_record();
                reset_player_rank();
            }
        }
        waitpid(pid_child[0], &status[0], 0);
        waitpid(pid_child[1], &status[1], 0);
    }
    return 0;
}

// auxliliary functions
int check_valid_args(int argc, char** argv){
    if(argc != 4){
        fprintf(stderr, "Invalid number of arguments: %d\n", argc);
        return 0;
    }

    char* pEnd_hostid;
    char* pEnd_key;
    char* pEnd_depth;

    long hid = strtol(argv[1], &pEnd_hostid, 10);
    long k = strtol(argv[2], &pEnd_key, 10);
    long d = strtol(argv[3], &pEnd_depth, 10);

    if (check_strtol(pEnd_hostid, argv[1], hid)){
        fprintf(stderr, "Host id is invalid: %s\n", argv[1]); return 0;
    }
    else if(check_strtol(pEnd_key, argv[2], k)){
        fprintf(stderr, "Key is invalid: %s\n", argv[2]); return 0;
    }
    else if(check_strtol(pEnd_depth, argv[3], d)){
        fprintf(stderr, "Depth is invalid: %s\n", argv[3]); return 0;
    }
    else if(k < 0 || k >= USHRT_MAX){
        fprintf(stderr, "Key is out of range: %s\n", argv[2]); return 0;
    }
    else if(d < 0 || d > MAX_DEPTH){
        fprintf(stderr, "Depth is out of range: %s\n", argv[3]); return 0;
    }

    initialize(hid, k, d);
    
    return 1;
}
int check_strtol(char* pEnd, char* arg, long n){
    return pEnd == arg || *pEnd != '\0' ||
        ((n == LONG_MIN || n == LONG_MAX) && errno == ERANGE);
}
void initialize(int hid, int k, int d){
    host_id = hid;
    key = k;
    depth = d;
    snprintf(fifo_r_pathname, sizeof(fifo_r_pathname), "fifo_%d.tmp", host_id);
    n_player = get_player_num();
    playerids = malloc(n_player*sizeof(int));
    reset_player_record(); 
    reset_player_rank();
    reset_round();
    fprintf(stderr, "Successfully read from fifo: host id[%d], key[%d], depth[%d]\n", host_id, key, depth);
}

// stdio
void get_player_ids_from_stdin(){
    read_message_v(stdin, n_player, playerids);
}
void send_player_ids_to_child(){
    int middle = n_player / 2;
    for(int i = 0 ; i < middle ; ++i){
        fprintf(write_l_child, "%d ", playerids[i]);
        fflush(write_l_child);
    }
    fprintf(write_l_child, "\n"); fflush(write_l_child);
    for(int i = middle ; i < n_player ; ++i){
        fprintf(write_r_child, "%d ", playerids[i]);
        fflush(write_r_child);
    }
    fprintf(write_r_child, "\n"); fflush(write_r_child);
}
void get_winner_from_child(){
    int l_id, l_bid, r_id, r_bid;
    read_message(read_l_child, &l_id, &l_bid);
    read_message(read_r_child, &r_id, &r_bid);
    win_bid = max(l_bid, r_bid);
    win_id = win_bid == l_bid ? l_id : r_id;
}

// utils
void print_fds(){
    fprintf(stderr, "fdsP2CL: %d, %d |fdsCL2P: %d, %d |fdsP2CR: %d, %d |fdsCR2P: %d, %d\n",
    fdsP2CL[0], fdsP2CL[1], fdsCL2P[0], fdsCL2P[1], fdsP2CR[0], fdsP2CR[1], fdsCR2P[0], fdsCR2P[1]);
}
void print_playerids(){
    reset_buf();
    strcat(buf, "Player ids: ");
    for(int i = 0 ; i < n_player ; ++i){
        char tmp[BUFF_SIZE];
        snprintf(tmp, BUFF_SIZE, "%d ",playerids[i]);
        strcat(buf, tmp);
    }
    strcat(buf, "\n");
    fprintf(stderr, "%s\n", buf);
}
void print_bucket(int row, int col, int buckets[][col]){
    reset_buf();
    for(int i = 0 ; i < N_POINT ; ++i){
        for(int j = 0 ; j < N_PLAYER ; ++j){
            char tmp[BUFF_SIZE];
            snprintf(tmp, BUFF_SIZE, "%d ", buckets[i][j]);
            strcat(buf, tmp);
        }
        strcat(buf, "\n");
    }
    fprintf(stderr, "%s", buf);
}
int max(int a, int b){
    return (a >= b)? a : b;
}
int get_player_pos(int id){
    int pos = -1;
    for(int i = 0 ; i < n_player ; ++i){
        if(playerids[i] == id){
            pos = i;
            break;
        }
    }
    assert(pos != -1);
    return pos;
}
int get_player_num(){
    return (8 / pow(2, depth));
}
void reset_buf(){
    memset(buf, 0, sizeof(buf));
}
void reset_round(){
    i_round = 1;
}
void reset_player_record(){
    player_record = malloc(n_player*sizeof(int));
    for(int i = 0 ; i < n_player ; ++i) player_record[i] = 0;
}
void reset_player_rank(){
    player_rank = malloc(n_player*sizeof(int));
}
void create_pipe(int n_pipe, ...){
    int* fds;
    va_list pipe_list;
    va_start(pipe_list, n_pipe);
    for(int i = 0 ; i < n_pipe ; ++i){
        fds = va_arg(pipe_list, int*);
        pipe(fds);
    }
    va_end(pipe_list);
}
void close_pipe(int n_pipe, ...){
    int* fds;
    va_list pipe_list;
    va_start(pipe_list, n_pipe);
    for(int i = 0 ; i < n_pipe ; ++i){
        fds = va_arg(pipe_list, int*);
        close(fds[0]);
        close(fds[1]);
    }
    va_end(pipe_list);
}
void redirect_std_to_pipe(int toi, int too){
    dup2(toi, STDIN_FILENO);
    dup2(too, STDOUT_FILENO);
}
char* tostring(int n){
    char* rtn = malloc(BUFF_SIZE);
    sprintf(rtn, "%d", n);
    return rtn;
}
void read_message(FILE* stream, int* n1, int* n2){
    fscanf(stream, "%d", n1);
    fscanf(stream, "%d", n2);
}
void read_message_v(FILE* stream, int n, int* arr){
    for(int i = 0 ; i < n ; ++i){
        fscanf(stream, "%d", &arr[i]);
    }
}
void set_child_stream(){
    read_l_child = fdopen(fdsCL2P[0], "r");
    read_r_child = fdopen(fdsCR2P[0], "r");
    write_l_child = fdopen(fdsP2CL[1], "w");
    write_r_child = fdopen(fdsP2CR[1], "w");
}
void check_file_status_flag(int fd){
    int val, accmode;
    val = fcntl(fd, F_GETFL, 0);
    accmode = val & O_ACCMODE;

    switch(accmode){
        case O_RDONLY:
            fprintf(stderr, "Read only\n"); break;
        case O_WRONLY:
            fprintf(stderr, "Write only\n"); break;
        case O_RDWR:
            fprintf(stderr, "Read write\n"); break;
    }
}

// root only
int find_first_nonzero(int* b){
    int pos = 0;
    for(int i = 0 ; i < N_PLAYER ; ++i){
        if(b[i] == 0){
            pos = i;
            break;
        }
    }
    return pos;
}
void prepare_ranking(){
    fprintf(stderr, "Start prepare_ranking\n");
    int buckets[N_POINT][N_PLAYER] = {0};
    int rank = 1;
    for(int i = 0 ; i < N_PLAYER ; ++i){
        int pid = playerids[i];
        int point = player_record[i];
        int insert_pos = find_first_nonzero(buckets[point]);
        buckets[point][insert_pos] = pid;
        fprintf(stderr, "PID: %d | POINT: %d | INSERT_POS: %d\n", pid, point, insert_pos);
    }              
    for(int i = N_POINT -1 ; i >= 0 ; --i){
        int cnt = 0;

        for(int j = 0 ; j < N_PLAYER ; ++j){
            if(buckets[i][j] == 0) continue;
            int pid = buckets[i][j];
            int pos = get_player_pos(pid);
            player_rank[pos] = rank;
            ++cnt;
        }
        rank += cnt;
    }
    print_bucket(N_POINT, N_PLAYER, buckets);
    fprintf(stderr, "Finish prepare_ranking\n");
}
void redirect_std_to_fifo(){
    fprintf(stderr, "In redirect_std_to_file: read from %s, write to %s\n", fifo_r_pathname, fifo_w_pathname);
    int r_fd = open(fifo_r_pathname, O_RDWR);
    int w_fd = open(fifo_w_pathname, O_RDWR);
    dup2(r_fd, STDIN_FILENO); close(r_fd);
    dup2(w_fd, STDOUT_FILENO); close(w_fd);
    fprintf(stderr, "Successfully redirect\n");
}
void update_winner_record(int id){
    if(id == END_SIG) return;
    fprintf(stderr, "Update Win Count: %d\n", id);
    int win_pos = get_player_pos(id);
    player_record[win_pos] += 1;
}
void send_ranking_to_stdout(){
    reset_buf();
    snprintf(buf, BUFF_SIZE, "%d\n", key);
    for(int i = 0 ; i < N_PLAYER ; ++i){
        char tmp[BUFF_SIZE];
        snprintf(tmp, BUFF_SIZE, "%d %d\n", playerids[i], player_rank[i]);
        strcat(buf, tmp);
    }
    printf("%s", buf);
    fflush(stdout);
}
