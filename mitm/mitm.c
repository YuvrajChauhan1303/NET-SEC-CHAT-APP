#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "dh.h"
#include "../crypto/crypto.h"

int main(){
    int ss , cc;

    struct sockaddr_in mal_addr = {0};

    char buf[100];

    ss = socket(AF_INET, SOCK_STREAM, 0);
    
    printf("Family: %d\n", mal_addr.sin_family);
    printf("Port: %d\n", ntohs(mal_addr.sin_port));
    printf("Address: %s\n", inet_ntoa(mal_addr.sin_addr));

    mal_addr.sin_family = AF_INET;
    mal_addr.sin_port = htons(8000);
    mal_addr.sin_addr.s_addr = IN_ADDDR_ANY

    bind (ss, (struct sockaddr*)&mal_addr, sizeof(mal_addr));
    listen(s, 10);

    printf("Listening to the victim at port 8000");
}

// create()listening socket
// client_fd = ccept()

// serverfd = connect_too_server()

// perform_dh_as_server()
// derive_aes_key()

// vice versa with server

// forward messages
// proxy_messages(
//         client_fd,
//         server_fd,
//         client_aes_key,
//         server_aes_key
//     );