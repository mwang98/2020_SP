CC 		 = gcc
CFLAGS 	 = -c
OPTFLAGS = -O3

EXECHOST  	= host
EXECPLAYER	= player

all	: $(EXECHOST) $(EXECPLAYER)
	@echo -n ""

$(EXECHOST): host.o
	$(CC) $(OPTFLAGS)  $^ -o $@ -lm
$(EXECPLAYER): player.o
	$(CC) $(OPTFLAGS)  $^ -o $@ -lm

host.o : host.c
	$(CC) $(OPTFLAGS) $(CFLAGS) host.c -o host.o
player.o : player.c
	$(CC) $(OPTFLAGS) $(CFLAGS) player.c -o player.o 

clean:
	rm -rf *.o $(EXECHOST) $(EXECPLAYER) *.tmp *.err *.echo
env:
	rm -rf *.tmp *.err