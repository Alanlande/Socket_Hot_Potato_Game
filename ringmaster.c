#include "potato.h"

//check the parameters
void check(int port_num, int player_num, int hops_num){
  if(port_num < 1024 || port_num > 65535)
  errorPrint("ERROR!!! Port number should be between 1024 and 65535.\n");
  if(player_num <= 1)
    errorPrint("ERROR!!! Number of players should be greater than 1.\n");
  if(hops_num < 0)
    errorPrint("ERROR!!! Hops should not be smaller than  0.\n");
}
// ringmaster connects to every player
void playerConnect(int player_num, playerInfo * playersInfo, int ringMasterfd){
  for(int count = 0; count < player_num; count++){
    struct sockaddr_in player_sc;
    socklen_t player_ringMaster_len = sizeof(player_sc);
    // accept
    int playerRingMasterfd = accept(ringMasterfd, (struct sockaddr *) &player_sc, &player_ringMaster_len);
    if(playerRingMasterfd < 0)
      errorPrint("ERROR!!! Failed to accept connection from player.\n");
    //send player's ID and player num
    if(send(playerRingMasterfd, &count, sizeof(int), 0) < 0 || send(playerRingMasterfd, &player_num, sizeof(int), 0) < 0)
      errorPrint("ERROR!!! Failed to send data to player.\n");
    //nessasary to store player_hostent???????
    struct hostent* player_hostent = gethostbyaddr((char *)&player_sc.sin_addr, sizeof(struct in_addr), AF_INET);
    //receive player's port num and save player's info
    int player_port;
    if(recv(playerRingMasterfd, &player_port, sizeof(int), MSG_WAITALL) < 0)
      errorPrint("ERROR!!! Failed to receive data from player.\n");
    //printf("Player %d is on %s, ready to play\n", count, player_hostent->h_name);
    printf("Player %d is ready to play\n", count);
    playersInfo[count].playerID = count;
    playersInfo[count].player_ringMaster_fd = playerRingMasterfd;
    playersInfo[count].listen_port = player_port;
    strcpy(playersInfo[count].hostname, player_hostent->h_name);
  }
}
//connect neighbor players 
void playerNeighbor(int player_num, playerInfo * playersInfo){
  player_notify notify;
  for(int count = 0; count < player_num; count++){
    //notify a player there will be a right neighbor connection
    memset(&notify, 0, sizeof(player_notify));
    sprintf(notify.info, "playerNeighbor");
    if(send(playersInfo[count].player_ringMaster_fd, &notify, sizeof(player_notify), 0) < 0)
      errorPrint("ERROR!!! Failed to send data to player when establishing playerNeighbor.\n");
    // receive ack from left player
    int leftOver = 0;
    if(recv(playersInfo[count].player_ringMaster_fd, &leftOver, sizeof(int), MSG_WAITALL) < 0)
      errorPrint("ERROR!!! Failed to receive ack data to player when establishing left playerNeighbor.\n");

    // send the player's right neighbor its ip info
    memset(&notify, 0, sizeof(player_notify));
    notify.port_num = playersInfo[count].listen_port;
    sprintf(notify.info, "%s", playersInfo[count].hostname);
    if(send(playersInfo[(count + 1) % player_num].player_ringMaster_fd, &notify, sizeof(player_notify), 0) < 0)
      errorPrint("ERROR!!! Failed to send data to player when establishing playerNeighbor.\n");
    // receive ack from right player
    int rightOver = 0;
    if(recv(playersInfo[(count + 1) % player_num].player_ringMaster_fd, &rightOver, sizeof(int), MSG_WAITALL) < 0)
      errorPrint("ERROR!!! Failed to receive ack data to player when establishing right playerNeighbor.\n");
  }
}
// send hotpotato to each player, close all fd and end the game
void endGame(int player_num, playerInfo * playersInfo, int ringMasterfd, potato_st hotPotato){
  //printf("Initial hops number is 0, game over.\n");
   for(int count = 0; count < player_num; count++){
    if(send(playersInfo[count].player_ringMaster_fd, &hotPotato, sizeof(potato_st), 0) < 0)
      errorPrint("ERROR!!! Failed to send data to player when initial hops num is 0.\n");
    close(playersInfo[count].player_ringMaster_fd);
  }
  close(ringMasterfd);
  exit(EXIT_SUCCESS);
}
//send startPotato to a random player
void sendToRandPlayer(int player_num, potato_st startPotato, playerInfo * playersInfo){
  srand((unsigned int) time(NULL) + player_num);
  int rand_player = rand() % player_num;
  printf("Ready to start the game, sending potato to player %d\n", rand_player);
  if(send(playersInfo[rand_player].player_ringMaster_fd, &startPotato, sizeof(potato_st), 0) < 0)
    errorPrint("ERROR!!! Failed to send hotpotato to the first player.\n");
}


int main(int argc, char *argv[]){
  if(argc != 4)
    errorPrint("ERROR!!! Syntax: ./ringmaster <port_num> <num_players> <num_hops>\n");
  int port_num = atoi(argv[1]), player_num = atoi(argv[2]), hops_num = atoi(argv[3]);
  char hostname[255];
  playerInfo playersInfo[player_num];
  //check parameters and print config
  check(port_num, player_num, hops_num);
  gethostname(hostname, sizeof(hostname));
  printf("Potato Ringmaster \nPlayers = %d\nHops = %d\n", player_num, hops_num); 
  ////type of socket created
  struct sockaddr_in master_sc;
  memset(&master_sc, 0, sizeof(master_sc));
  master_sc.sin_family = AF_INET;
  master_sc.sin_addr.s_addr = INADDR_ANY;
  master_sc.sin_port = htons(port_num);
  //create socket, setsockopt, bind and listen
  int ringMasterfd = socket(AF_INET, SOCK_STREAM, 0);
  //printf("ringmaster:%d\n", ringMasterfd);
  if(ringMasterfd < 0)
    errorPrint("ERROR!!! Failed to create ringmaster socket.\n");
  int yes = 1;
  if(setsockopt(ringMasterfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    errorPrint("ERROR!!! Failed at setsockopt.\n");
  if(bind(ringMasterfd, (struct sockaddr *) &master_sc, sizeof(master_sc)) < 0)
    errorPrint("ERROR!!! Failed at bind.\n");
  if(listen(ringMasterfd, player_num) < 0)
    errorPrint("ERROR!!! Failed at listen.\n");
  //connect with each player
  playerConnect(player_num, playersInfo, ringMasterfd);
  //connect neighbor players, establish the ring
  playerNeighbor(player_num, playersInfo);
  //if initial hops is 0, end game when it starts
  potato_st hotPotato;
  memset(&hotPotato, 0, sizeof(potato_st));
  hotPotato.hops_num = hops_num;
  if(hops_num == 0)
    endGame(player_num, playersInfo, ringMasterfd, hotPotato);
  //send startPotato to a random player
  sendToRandPlayer(player_num, hotPotato, playersInfo);
  
  fd_set readfds;
  int max_fd = 0;
  FD_ZERO(&readfds);
  // add descriptors to an fd_set
  for(int count = 0; count < player_num; count++){
    FD_SET(playersInfo[count].player_ringMaster_fd, &readfds);
    max_fd = max_fd >= playersInfo[count].player_ringMaster_fd ? max_fd : playersInfo[count].player_ringMaster_fd;
  }
  if(select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
    errorPrint("ERROR!!! Failed to select.\n"); 
  //incoming connection
  memset(&hotPotato, 0, sizeof(potato_st));
  for(int count = 0; count < player_num; count++){
    if(FD_ISSET(playersInfo[count].player_ringMaster_fd, &readfds)){
      if(recv(playersInfo[count].player_ringMaster_fd, &hotPotato, sizeof(potato_st), MSG_WAITALL) < 0)
	errorPrint("ERROR!!! Failed to receive hotpotato.\n");
      break;
    }
  }
  assert(hotPotato.hops_num == 0);
  printf("Trace of potato:\n%s\n", hotPotato.trace);
  endGame(player_num, playersInfo, ringMasterfd, hotPotato);
}
