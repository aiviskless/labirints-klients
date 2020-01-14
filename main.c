#include "libs.h"
#include "game.c"
#include "consts.h"
#include "maps.h"

// server params
const int PORT = 8887;
const char* IP_ADDRESS = "192.168.8.105";

// globals
int socket_desc;
struct sockaddr_in server;

void join_server();
void setup_connection();

int main () {
  setup_connection();

  join_server();

  return 0;
}

void join_server() {
  char join_packet[NICKNAME_LENGTH + 1];
  char response[RESPONSE_LENGTH];
  char response_code;
  char nickname[NICKNAME_LENGTH];

  printf("Enter your nickname: ");
  bzero(nickname, NICKNAME_LENGTH);
  fgets(nickname, sizeof(nickname), stdin);

  // add nickname to JOIN_GAME code
  bzero(join_packet, sizeof(join_packet));
  join_packet[0] = JOIN_GAME;
  // code + nickname
  strcat(join_packet, nickname);
  
  // send JOIN_GAME
  if (write(socket_desc, join_packet, NICKNAME_LENGTH + 1) < 0) {
    printf("Join to server failed\n");
  }

  // get reply
  if (read(socket_desc, response, ARENA_WIDTH_IN_TILES * ARENA_HEIGHT_IN_TILES + 2) < 0) {
    puts("Read from server failed");
  } else {
    switch (response[0]) {
      case LOBBY_INFO:
        printf("Received lobby info\n");
      case GAME_IN_PROGRESS:
        printf("Please try later - game in progress\n");
        break;
      case USERNAME_TAKEN:
        printf("Your username is taken, try another one:\n");
        join_server();
      case MAP_ROW:
        // get map and start game
        strcat(arena_map, &response[1]);
        start_game();
      default:
        printf("Please try later...\n");
        break;
    }
  }
}

void setup_connection() {
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    perror("Error while creating socket\n");
  }

  server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);

  // connect to server
  if (connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
    perror("Error while connecting\n");
  } else {
    printf("Connected to server!\n");
  }
}