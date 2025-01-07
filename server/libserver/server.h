#ifndef SERVER_H
#define SERVER_H

#define PORT 3030
#define BUFFER 1024

/* structure saves address list users */
typedef struct {
    struct sockaddr_in* addresses;
    size_t capacity;
    size_t count;
} addressList;

/* functions allowing manipulate addressList */

/* create address list structure */
addressList* create_address_list(size_t);
/* add address to addressList structure */
short int add_address(addressList*, const struct sockaddr_in*);
/* find address in addressList structure */
int find_address(addressList*, const struct sockaddr_in*);
/* remove address from addressList structure */
short int remove_address(addressList*, const struct sockaddr_in*);
/* free address list */
void free_address_list(addressList*);

/* server functions */

/* shut down on sigint signal */
void shutDownServerHandler(int);
/* handle client connectins function */
void handleClient(int*, addressList*, char*);
/* bind socket and recv clients  */
void serveServer(addressList*, int*);

#endif