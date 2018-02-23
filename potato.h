#ifndef __potato_H__
#define __potato_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <assert.h>

typedef struct p{
  int playerID;
  int player_ringMaster_fd;
  int listen_port;
  char hostname[255];
}playerInfo;

typedef struct potato{
  int hops_num;
  char trace[2048];
}potato_st;

typedef struct notify{
  int port_num;
  char info[255];
}player_notify;

void errorPrint(const char * msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

/****** For ringmaster ******/

//check the parameters
void check(int port_num, int player_num, int hops_num);
// ringmaster connects to every player
void playerConnect(int player_num, playerInfo * playersInfo, int ringMasterfd);
// when the initial hops num is 0, end the game
void endGame(int player_num, playerInfo * playersInfo, int ringMasterfd, potato_st hotPotato);
//connect neighbor players
void playerNeighbor(int player_num, playerInfo * playersInfo);
//send startPotato to a random player
void sendToRandPlayer(int player_num, potato_st startPotato, playerInfo * playersInfo);




/****** For player ******/ 

// check player parameters
void playerCheck(struct hostent* player_hostent, int port_num);
// receive curr player's ID and player num
void playerConfig(int * playerID, int * player_num, int playerfd);
// get an available port, send it to ringmaster
void getPort(int * playerCurrfd, int playerfd);
// the right player is coming, get ready
void setRightfd(int playerfd, int playerCurrfd, int * rightPlayerfd);
// connec to the left player with given ip info
void setLeftfd(player_notify notify, int playerfd, int * leftPlayerfd);  


#endif
