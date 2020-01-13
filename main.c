#include "libs.h"
#include "game.c"

// server params
const int PORT = 8887;
const char* IP_ADDRESS = "192.168.8.105";

// globals
int socket_desc;
struct sockaddr_in server;

void join_server(char *nickname);
void setup_connection();

int main () {
  // char nickname[NICKNAME_LENGTH];

  // // create socket
  // setup_connection();

  // // connect to server
  // if (connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
  //   printf("Error while connecting\n");
  //   return -1;
  // }

  // // get nickname  
  // bzero(nickname, NICKNAME_LENGTH);
  // printf("Connected to server!\nEnter your nickname: ");
  // fgets(nickname, sizeof(nickname), stdin);

  // join_server(nickname);

  start_game();

  return 0;
}

void join_server(char *nickname) {
  char join_packet[NICKNAME_LENGTH + 1];
  char response[RESPONSE_LENGTH];
  char response_code;

  // add nickname to JOIN_GAME code
  bzero(join_packet, sizeof(join_packet));
  join_packet[0] = JOIN_GAME;
  strcat(join_packet, nickname);
  
  // send JOIN_GAME
  if (write(socket_desc, join_packet, NICKNAME_LENGTH + 1) < 0) {
    printf("Join to server failed\n");
  }

  // get reply
  if (read(socket_desc, response, RESPONSE_LENGTH) < 0) {
    puts("Read from server failed");
  } else {
    switch (response[0]) {
      case LOBBY_INFO:
        printf("Lobby info code\n");
        start_game();
        break;
      default:
        printf("Please try later...");
        break;
    }
  }
}

void setup_connection() {
  // char ip_address[14];
  // char port[4];

  // printf("Enter server's IP address:\n");
  // fgets(ip_address, sizeof(ip_address), stdin);
  // printf("Enter server's port:\n");
  // fgets(port, sizeof(port), stdin);

  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Error while creating socket");
  }

  server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
}

// void packet_handler(unsigned char reply[RESPONSE_LENGTH]) {
//   unsigned char response_code = reply[0];
//   printf("Response code is %d \n", response_code);

//   switch (response_code) {
//     case LOBBY_INFO:
//       lobbyStatus(reply);
//       break;

//     case GAME_START:
//       break;

//     case GAME_UPDATE:
//       break;

//     case GAME_END:
//       break;

//     default: break;
//   }
// }