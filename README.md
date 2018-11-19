## Socket_Hot_Potato_Game
### This is a hot potato game using socket networking programming


### Usage:
```sh
make
// in test.sh, change the host name to your linux hostname
bash test.sh
```

### Implementation 
- I used socket programming to help the ringmaster and players communicate with each other

- As the communication has strict order: ringmaster->some player->other player->...->ringmastrer, we have to use blocking synchronized IO, otherwise using asyncIO has undetermined behaviors

- Firstly, I establish ringmaster's socket, then make it wait for players' connection and gather their information(port number, hostname. etc). After so, we established connections as below:
![alt text](https://github.com/Alanlande/RSVP_WebApp/blob/master/sample2_pending_page.png "The main pending page")

- Secondly, with ringmaster's help, establish each player's connection to its two neighbors: as the left player to wait for its right neighbor's connection and then send signal to the right play to connect to its left neighbot. 
![alt text](https://github.com/Alanlande/RSVP_WebApp/blob/master/sample2_pending_page.png "The main pending page")

- Lastly, kick off the game and wait to see which player is the last one who got the hot potato.
