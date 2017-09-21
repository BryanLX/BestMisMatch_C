
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <getopt.h>
#include "utils.h"
#define BUFSIZE 1024
#ifndef PORT
  #define PORT 50133
#endif
#if 0
/*
 * Structs exported from in.h
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr;
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif


int remove_user(struct client ** head,char name[MAX_NAME]);
void wrap_up();
struct client * contain_user(struct client ** head,char * name);
struct client * find_client(struct client ** head,int fd);
void add_client(struct client ** head,struct client *new_client);
QNode *root = NULL;
Node *interests = NULL;


int main(int argc, char *argv[]){

  fd_set master;/* master file descriptor list */
  fd_set read_fds;/* temp file descriptor list for select() */
  int fdmax;   /* maximum file descriptor number */
  int parentfd; /* parent socket */
  int childfd; /* child socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char namebuf[MAX_NAME];
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */


  int i;

  /* clear the master and temp sets */
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  portno = PORT;
  /*set up qtree and so on */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  // read interests from file
  interests = get_list_from_file (argv[1]);
  if (interests == NULL)
      return 1;
  printf("Listenig on %d \n",PORT);
  // build question tree
  root = add_next_level (root,  interests);

  /* Create parent socket */

  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0)
    error("ERROR opening socket");


  /*solving address in use issue  */

  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));
  bzero((char *) &serveraddr, sizeof(serveraddr));
  /* bind */

  serveraddr.sin_family = AF_INET;

  serveraddr.sin_addr.s_addr =  htonl(INADDR_ANY);

  serveraddr.sin_port = htons((unsigned short)portno);


  if(bind(parentfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) <0)
      error("ERROR on binding");



  /* listen */

  if(listen(parentfd, 10) <0)
     error("ERROR on listen");


  clientlen = sizeof(clientaddr);
  struct client *active_list = calloc(1,sizeof(struct client *));
  active_list = NULL;
  /* add the listener to the master set */

  FD_SET(parentfd, &master);

  fdmax = parentfd; /* so far, the biggest file descripter is this one*/
  /* main loop */
  struct client current_client;
  for(;;){
    read_fds = master;
    if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
      error("ERROR on select");
    }

    /*run through the existing connections looking for data to be read*/
    for(i = 0; i <= fdmax; i++){
      if(FD_ISSET(i, &read_fds)){
        if(i == parentfd){
          clientlen = sizeof(clientaddr);
          if((childfd = accept(parentfd, (struct sockaddr *)&clientaddr, (unsigned int * )&clientlen)) <0){
            error("ERROR on accept");
          }else{
            FD_SET(childfd, &master); /* add to master set */
            if(childfd > fdmax){
              fdmax = childfd;
            }
            hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                sizeof(clientaddr.sin_addr.s_addr), AF_INET);
            if (hostp == NULL)
              error("ERROR on gethostbyaddr");
            hostaddrp = inet_ntoa(clientaddr.sin_addr);
            if (hostaddrp == NULL)
              error("ERROR on inet_ntoa\n");
            printf("server established connection with %s (%s)\n",hostp->h_name, hostaddrp);
            print_text (childfd,"What is your username? \n");
            bzero(namebuf,MAX_NAME);
            n = read(childfd, namebuf, MAX_NAME);
            if (n < 0)
              error("ERROR reading from socket");
            namebuf[strlen(namebuf)-2] = '\0';
            /* create new cilent */

            struct client *new_client =calloc(1,sizeof(struct client));
            new_client->fd = childfd;
            strcpy(new_client->username, namebuf);
            new_client->state = 1;
            new_client->next = NULL;
            add_client(&active_list,new_client);

            print_text (childfd,"Welcome.\n");
            print_text (childfd,"Go ahead and enter user commands>\n");
          }
        }else{
          /* handle data from a client */
          current_client = *find_client(&active_list,i);
          bzero(current_client.buf, BUFSIZE);
          n = read(current_client.fd, current_client.buf, BUFSIZE);
          if (n < 0){
            close(i);
            /* remove from master set */
            FD_CLR(i, &master);
            error("ERROR reading from socket");
          }
          int argc = 8;
          char ** cmd_argv = calloc(3,sizeof(char*));
          for(int j =0;j<3;j++){
            cmd_argv[j] = calloc(128,sizeof(char));
          }
          argc=tokenize(current_client.buf,cmd_argv);
          int result = process_args(argc,cmd_argv, &root,interests,&current_client,active_list);
          if(result == -1){
            printf("Removing client %s \n",current_client.username);
            current_client.state = 0;
            if(remove_user(&active_list,current_client.username)==0){
              error("Can not remove from active list.\n");
            }
            FD_CLR(i, &master);
            close(i);
            //break;
          }
          if(result == -2){
            print_text(current_client.fd,"You haven't done test yet.\n");
          }
        }

      }

    }

  }
  wrap_up();
  int reuse = 1;
  if(setsockopt(parentfd,SOL_SOCKET,SO_REUSEADDR,(char *)&reuse,sizeof(int))==-1)
      error("Can't set the 'reuse' option on the socket.");
  return 0;

}

void wrap_up(){
    //end of main loop - the user typed "q"
    print_qtree (root, 0);

    free_list (interests);
    free_qtree(root);

    exit(0);
}
struct client * contain_user(struct client ** head,char name[MAX_NAME]){
    struct client * cur = *head;
    return NULL;//
    while(cur != NULL){
      if(strncmp(cur->username, name,128)==0){
        return cur;
      }
      cur = cur->next;
    }
    return NULL;
}
struct client * find_client(struct client ** head,int fd){
    struct client * cur = *head;
    while(cur != NULL){
      if(cur->fd == fd){
        return cur;
      }
      cur = cur->next;
    }
    return NULL;
}

// add client
void add_client(struct client ** head,struct client *new_client){
  if(*head == NULL){
    *head = new_client;
  }else{
    new_client->next = *head;
    struct client * temp = calloc(1,sizeof(struct client *));
    temp= new_client;
    *head = temp;
  }

}
// remove user from active list,return 1 if success 0 otherwise.
int remove_user(struct client ** head,char name[MAX_NAME]){
  struct client * cur = *head;
  struct client * prev = cur;
  if(cur ->next == NULL){
    if(strncmp(cur->username,name,128)==0){
      *head = NULL;
      return 1;
    }else{
      return 0;
    }
  }
  while(cur != NULL){
    if(strncmp(cur->username, name,128)==0){
      prev->next = cur->next;
      return 1;
    }
    prev = cur;
    cur = cur->next;
  }
  return 0;
}
