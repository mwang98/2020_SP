#! /bin/bash
function myEcho(){
    echo $1 >> $file_echo_out
}
function getFinalRecord(){
    for i in $(seq 1 $n_player)
    do
        echo $i ${player_record[$i]}
    done
}
function createHost(){
    # args n_host
    for i in $(seq 1 $n_host)
    do
        exec "./host" $i $i 0 &> "tmp_$i.err" &
    done
}
function sendTerminationSig(){
    initCombination $n_player_per_host -1
    for i in $(seq 1 $n_host)
    do
        echo ${combination[@]} >> "fifo_${i}.tmp"
    done
}
function readRankingFromHost(){
    local filename="fifo_0.tmp" # "fifo_0.tmp"
    local n_read_line=0     # 8 players + header => 9 lines
    local host_id

    while read line || [[ $n_read_line -ne $((1+$n_player_per_host)) ]]
    # while read line
    do  
        if [[ -z $line ]]
        then 
            continue 
        fi

        if [ $n_read_line -eq 0 ]
        then 
            host_id=$line
            myEcho "Host id [$host_id]"
        else
            local pid
            local rank
            IFS=' ' read pid rank <<< $line
            local point=$(($n_player_per_host-$rank))
            local o_point=${player_record[$pid]}
            myEcho "pid $pid  rank $rank  Get point $point"
            # set player record
            player_record[$pid]=$(($point+$o_point))
        fi
        
        n_read_line=$(($n_read_line+1))
        if [ $n_read_line -eq $((1+$n_player_per_host)) ]
        then
            break
        fi
    done < $filename
    # set available host
    available_host="fifo_${host_id}.tmp"
    myEcho "Available: $available_host"
    # update unread combination
    n_read_rank=$(($n_read_rank+1))
    
    myEcho "player_record: ${player_record[@]}"
    myEcho "Read $n_read_rank results"
}
function initCombination(){
    # args: array_length init_val 
    for i in $(seq 0 $(($1-1)))
    do  
        combination[$i]=$2
    done
}
function initRecord(){
    # args: array_length init_val 
    for i in $(seq 1 $(($1)))
    do  
        player_record[$i]=$2
    done
}
function getAvailableHost(){
    if [ $cnt_init_host -le $n_host ]
    then
        myEcho "FROM INIT"
        available_host="fifo_${cnt_init_host}.tmp"
        cnt_init_host=$(($cnt_init_host+1))
    else
        myEcho "FROM RANK"
        readRankingFromHost # set available_host in function
    fi
}
function combinationUtils()
{
    # args: combinationUtils <start> <end> <index> <r>
    local start=$1
    local end=$2
    local index=$3
    local comb_len=$4

    if [ $index -eq $comb_len ]
    then
        getAvailableHost
        myEcho "Send to: $available_host"
        myEcho "Combination: ${combination[@]}"
        echo ${combination[@]} >> $available_host
        n_combination=$(($n_combination+1))
        return
    fi

    local i=$start
    while [ $i -le $end -a $(($end-$i+1)) -ge $(($r-$index)) ]
    do
        combination[$index]=$(($i+1))
        combinationUtils $(($i+1)) $end $(($index+1)) $comb_len
        i=$(($i+1))
    done
}
function getCombination()
{
    local start=0
    local end=$((n_player-1))
    local index=0
    local comb_len=$n_player_per_host

    combinationUtils $start $end $index $comb_len
}
# =====================Main Sciprt=================

# read from arguments
n_host=$1
n_player=$2

n_player_per_host=8
cnt_init_host=1
available_host=1
n_combination=0
n_read_rank=0

file_echo_out="shell.echo"

# create fifo for each host
for i in $(seq 0 $n_host);
do
    mkfifo "fifo_${i}.tmp";
done

# create array to record the combination 
declare -a combination
declare -a player_record

initCombination $n_player_per_host 0 
initRecord $n_player 0

# create host
createHost

# generate all combination
getCombination

# read unread record
while [ $n_combination -ne $n_read_rank ]
do
    readRankingFromHost
done
myEcho "Fianl player record: ${player_record[@]}"

# send termination to host
sendTerminationSig

# show result
getFinalRecord

# clear environment
make env