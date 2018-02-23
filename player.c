#include "potato.h"

// check player parameters
void playerCheck(struct hostent* player_hostent, int port_num){
  if(!player_hostent)
    errorPrint("ERROR!!! Invalid hostname!\n");
  if(port_num < 1024 || port_num > 65535)
    errorPrint("ERROR!!! Invalid port number.\n");
}
// receive curr player's ID and player num
void playerConfig(int * playerID, int * player_num, int playerfd){
  if(recv(playerfd, playerID, sizeof(int), MSG_WAITALL) < 0 || recv(playerfd, player_num, sizeof(int), MSG_WAITALL) < 0 ){
    errorPrint("ERROR!!! Failed to receive player config info.\n");
  }
  printf("Connected as player %d out of %d total players\n", *playerID, *player_num);
}
// get an available port, send it to ringmaster
void getPort(int * playerCurrfd, int playerfd){
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0)
    errorPrint("ERROR!!! Failed to create socket in getPort.\n");
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  //  for(int i = 1024; i <= 65535; i++){
  for(int i = 51015; i <= 51097; i++){
    addr.sin_port = htons(i);
    if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
      continue;
    else{
      if(send(playerfd, &i, sizeof(int), 0) < 0)
	errorPrint("ERROR!!! Failed to send port number back to ringmaster.\n");
      *playerCurrfd = fd;
      return;
    }
  }
  errorPrint("ERROR!!! Failed to find available port.\n");
}
// the right player is coming, get ready
void setRightfd(int playerfd, int playerCurrfd, int * rightPlayerfd){
  struct sockaddr_in neighbor;
  memset(&neighbor, 0, sizeof(neighbor));
  if(listen(playerCurrfd, 2) < 0)
    errorPrint("ERROR!!! Failed to listen.\n");
  socklen_t len = sizeof(neighbor);
  // send ack to ringmaster
  int rightSig = 0;
  if(send(playerfd, &rightSig, sizeof(int), 0) < 0)
    errorPrint("ERROR!!! Failed to send ack in setRightfd.\n");  
  //accept
  *rightPlayerfd = accept(playerCurrfd, (struct sockaddr *) &neighbor, &len);
  if(*rightPlayerfd < 0)
    errorPrint("ERROR!!! Failed to accept right neighbor connection.\n");
}

// connec to the left player with given ip info
void setLeftfd(player_notify notify, int playerfd, int * leftPlayerfd){
  struct sockaddr_in neighbor;
  memset(&neighbor, 0, sizeof(neighbor));
  *leftPlayerfd = socket(AF_INET, SOCK_STREAM, 0);
  if (*leftPlayerfd < 0)
    errorPrint("ERROR!!! Failed to create socket.\n");
  struct hostent * player_hostent = gethostbyname(notify.info);
  if (!player_hostent)
    errorPrint("ERROR!!! Invalid neighbor hostname!\n");
  
  neighbor.sin_family = AF_INET;
  bcopy((char *)player_hostent->h_addr, (char *)&neighbor.sin_addr.s_addr, player_hostent->h_length);
  neighbor.sin_port = htons(notify.port_num);
  //printf("port:%d\n",notify.port_num);
  //connect
  //printf("leftPlayerfd: %d\n", *leftPlayerfd);
  if(connect(*leftPlayerfd, (struct sockaddr *) &neighbor, sizeof(neighbor)) < 0)
    errorPrint("ERROR!!! Failed to connect to left neighbor.\n");
  //send ack to ringmaster I'm done this part
  int leftSig = 0;
  if(send(playerfd, &leftSig, sizeof(int), 0) < 0)
    errorPrint("ERROR!!! Failed to send ack in setLeftfd.\n");
}

int main(int argc, char *argv[]){
  if(argc != 3)
    errorPrint("ERROR!!! Syntax: ./player <machine_name> <port_num>\n");
  int playerID = 0, player_num = 0, playerCurrfd = 0, leftPlayerfd = 0, rightPlayerfd = 0;
  struct hostent* player_hostent = gethostbyname(argv[1]);
  int port_num = atoi(argv[2]);
  playerCheck(player_hostent, port_num);
  struct sockaddr_in player_sc;
  memset(&player_sc, 0, sizeof(player_sc));
  player_sc.sin_family = AF_INET;
  player_sc.sin_port = htons(port_num);
  bcopy((char *)player_hostent->h_addr, (char *)&player_sc.sin_addr.s_addr, player_hostent->h_length);
  int playerfd = socket(AF_INET, SOCK_STREAM, 0);
  if(playerfd < 0)
    errorPrint("ERROR!!! Invalid sockect in player\n");
  // connect to ringmaster
  if(connect(playerfd, (struct sockaddr *) &player_sc, sizeof(player_sc)) < 0)
    errorPrint("ERROR!!! Failed to connect to ringmaster.\n");
  // receive curr player's info
  playerConfig(&playerID, &player_num, playerfd);
  //printf("Searching for bug 0\n");
  // get useful player port and send to ringmaster
  getPort(&playerCurrfd, playerfd);
  //printf("Searching for bugs 0\n");
  player_notify notify;
  //connect neighbor players
  for(int connect = 0; connect < 2; connect++){
    memset(&notify, 0, sizeof(player_notify));
    if(recv(playerfd, &notify, sizeof(player_notify), MSG_WAITALL) < 0)
      errorPrint("ERROR!!! Failed to receive from ringmaster\n");
    if(strcmp(notify.info, "playerNeighbor") == 0){ 
      //printf("player %d set RIGHT:\n", playerID);
      setRightfd(playerfd, playerCurrfd, &rightPlayerfd);
    }
    else{
      //printf("player %d set LEFT:\n", playerID);
      setLeftfd(notify, playerfd, &leftPlayerfd);
    }
  }
  //printf("Searching for bugs 1\n");
  srand((unsigned int) time(NULL) + playerID);
  fd_set readfds;
  int max_fd = 0;
  potato_st hotPotato;
  while(1){
    memset(&hotPotato, 0, sizeof(potato_st));
    // add descriptors to an fd_set 
    FD_ZERO(&readfds);
    FD_SET(playerfd, &readfds);
    FD_SET(leftPlayerfd, &readfds);
    FD_SET(rightPlayerfd, &readfds);
    max_fd = max_fd > playerfd ? max_fd : playerfd;
    max_fd = max_fd > leftPlayerfd ? max_fd : leftPlayerfd;
    max_fd = max_fd > rightPlayerfd ? max_fd : rightPlayerfd;
    //incoming connection from ringmster, left player or right player
    if(select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
      errorPrint("ERROR!!! Failed to select.");
    if(FD_ISSET(playerfd, &readfds)){
      if (recv(playerfd, &hotPotato, sizeof(potato_st), MSG_WAITALL) < 0)
	errorPrint("ERROR!!! Failed to receive potato.");
    }
    else if(FD_ISSET(leftPlayerfd, &readfds)){
      if (recv(leftPlayerfd, &hotPotato, sizeof(potato_st), MSG_WAITALL) < 0)
	errorPrint("ERROR!!! Failed to receive potato.");
    }
    else if(FD_ISSET(rightPlayerfd, &readfds)){
      if (recv(rightPlayerfd, &hotPotato, sizeof(potato_st), MSG_WAITALL) < 0)
	errorPrint("ERROR!!! Failed to receive potato.");
    }
    // the game goes on
    if(hotPotato.hops_num >= 1){
      hotPotato.hops_num--;
      // chosen as the first random player
      if(strlen(hotPotato.trace) == 0)
	sprintf(hotPotato.trace, "%d",  playerID);
      // middle player
      else
	sprintf(hotPotato.trace, "%s,%d", hotPotato.trace, playerID);
      // the last player who gets the hot potato
      if(!hotPotato.hops_num){
	printf("I'm it.\n");
	if (send(playerfd, &hotPotato, sizeof(potato_st), 0) < 0)
	  errorPrint("ERROR!!! Failed to send potato back to ringmaster.");
      }
      // continue the game
      else{
	int rand_player = rand() % 2;
	int nextID = 0;
	if(rand_player == 0){
	  nextID = (playerID + player_num - 1) % player_num;
	  printf("Sending potato to %d\n", nextID);
	  if (send(leftPlayerfd, &hotPotato, sizeof(potato_st), 0) < 0)
	    errorPrint("ERROR!!! Failed to send potato to left player.");
	}//left
	else{
	  nextID = (playerID + 1) % player_num;
	  printf("Sending potato to %d\n", nextID);
	  if (send(rightPlayerfd, &hotPotato, sizeof(potato_st), 0) < 0)
	    errorPrint("ERROR!!! Failed to send potato to right player.");
	}//right
      }
    }
    // the game is over
    else if(hotPotato.hops_num == 0){
      close(playerfd);
      close(playerCurrfd);
      close(leftPlayerfd);
      close(rightPlayerfd);
      break;
    }
  }
  exit(EXIT_SUCCESS);
}
