#include "libs.h"
#include "game.c"

// server params
const int PORT = 8887;
const char* IP_ADDRESS = "192.168.8.105";

// globals
int socket_desc;
struct sockaddr_in server;

void setup_connection() {
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Error while creating socket");
  }

  server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
}

int main () {

  // create socket
  setup_connection();

  // connect to server
  if (connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
    puts("Error while connecting");
    return 1;
  } else {
    puts("Connected");
    start_game();
  }

  return 0;
}