#include "libs.h"

// server params
const int PORT = 8887;
const char* IP_ADDRESS = "192.168.8.105";

// globals
int socketDescriptor;
struct sockaddr_in server;

void setupConnection() {
  socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socketDescriptor == -1) {
    printf("Error while creating socket");
  }

  server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
}

int main () {

  // create socket
  setupConnection();

  // connect to server
  if (connect(socketDescriptor, (struct sockaddr *) &server, sizeof(server)) < 0) {
    puts("Error while connecting");
    return 1;
  } else {
    puts("Connected");
  }

  return 0;
}
