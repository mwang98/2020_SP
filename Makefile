CC 		 = gcc
CFLAGS 	 = -c
OPTFLAGS = -O3

EXECHOST  	= host
EXECPLAYER	= player

EXECHOST_D = host
EXECPLAYER_D = player

all	: $(EXECHOST) $(EXECPLAYER)
	@echo -n ""

$(EXECHOST): host.o
	$(CC) $(OPTFLAGS) -o $@ host.o -lm
$(EXECPLAYER): player.o
	$(CC) $(OPTFLAGS) -o $@ player.o -lm

host.o : host.c
	$(CC) $(OPTFLAGS) $(CFLAGS) host.c -o host.o -lm
player.o : player.c
	$(CC) $(OPTFLAGS) $(CFLAGS) player.c -o player.o -lm

# debug : $(EXECHOST_D) $(EXECPLAYER_D)

# $(EXECHOST_D) : host_d.o
# 	$(CC) $(OPTFLAGS) -o $@ host_d.o -lm
# $(EXECPLAYER_D): player_d.o
# 	$(CC) $(OPTFLAGS) -o $@ player_d.o -lm

# host_d.o : host.c
# 	$(CC) $(OPTFLAGS) $(CFLAGS) host.c -DDEBUG -o host_d.o
# player_d.o : player.c
# 	$(CC) $(OPTFLAGS) $(CFLAGS) player.c -DDEBUG -o player_d.o 

clean:
	rm -rf *.o $(EXECHOST) $(EXECPLAYER) *.tmp *.err *.echo
env:
	rm -rf *.tmp *.err *.echo