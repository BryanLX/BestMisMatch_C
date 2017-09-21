
#include "utils.h"
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
#define BUFSIZE 1024


/*
 * Print a formatted error message to stderr.
 */
void error(char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}


/*
 * Tokenize the command stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv) {
    int cmd_argc = 0;
    char *next_token = strtok(cmd, DELIM);
    while (next_token != NULL) {
        cmd_argv[cmd_argc] = next_token;
        cmd_argc++;

	if(cmd_argc < (INPUT_ARG_MAX_NUM - 1))
	    next_token = strtok(NULL, DELIM);
	else
	    break;
    }
    if (cmd_argc == (INPUT_ARG_MAX_NUM - 1)) {
	cmd_argv[cmd_argc] = strtok(NULL, "");
	if(cmd_argv[cmd_argc] != NULL)
	    ++cmd_argc;
    }

    return cmd_argc;
}
//method from questions.c
Node * get_list_from_file (char *input_file_name) {
    Node * head = NULL;
    char line[MAX_LINE];

    // open input file
    FILE *f = fopen(input_file_name, "r");
        if (!f){
        perror(input_file_name);
        exit(1);
    }

    // keep track of the last node in the linked list
    // (since we insert at the end)
    Node *end = head;

    // read lines and add to the list
    while (fgets(line, MAX_LINE, f) != NULL){
        // remove end-of-line characters
        line[strcspn(line, "\r\n")] = '\0';

        // create new node
        Node *new = malloc(sizeof(Node));
        if (!new){
            perror("malloc");
            exit(1);
        }

        char *newstr = malloc((strlen(line)+1)*sizeof(char));
        if (!newstr){
            perror("malloc");
            exit(1);
        }
        strcpy(newstr, line);

        new->str = newstr;
        new->next = NULL;

        // add node to end of linked list
        if (!end){ // empty list
            head = new;
            end = head;

        } else {
            end->next = new;
            end = new;
        }
    }

    fclose(f);

    return head;
}

void print_list (Node *head) {
    // we use head as pointer to each successive node in the list
    for (; head; head = head->next)
        printf("%s\n", head->str);
}

void free_list (Node *head) {
    Node *next;

    // we use head as pointer to each successive node in the list
    for (; head; head = next){
        // we need to keep track of the next pointer, since we cannot
        // access it through head once head has been freed
        next = head->next;

        // free the node string
        free(head->str);
        // free the node itself
        free(head);
    }
}

// print text to given fd
void print_text (int childfd,char text[1024]) {
    int n ;
    n = write(childfd, text, strlen(text));
    if (n < 0)
       error("ERROR writing to socket");
}
QNode *add_next_level (QNode *current, Node *list_node) {
    int str_len;

    str_len = strlen (list_node->str);
    current = (QNode *) calloc (1, sizeof(QNode));

    current->question =  (char *) calloc (str_len +1, sizeof(char ));
    strncpy ( current->question, list_node->str, str_len );
    current->question [str_len] = '\0';
    current->node_type = REGULAR;

    if (list_node->next == NULL) {
        current->node_type = LEAF;
        return current;
    }

    current->children[0].qchild = add_next_level ( current->children[0].qchild, list_node->next);
    current->children[1].qchild = add_next_level ( current->children[1].qchild, list_node->next);

    return current;
}

void print_qtree (QNode *parent, int level) {
    int i;
    for (i=0; i<level; i++)
        printf("\t");

    printf ("%s type:%d\n", parent->question, parent->node_type);
    if(parent->node_type == REGULAR) {
        print_qtree (parent->children[0].qchild, level+1);
        print_qtree (parent->children[1].qchild, level+1);
    }
    else { //leaf node
        for (i=0; i<(level+1); i++)
            printf("\t");
        print_users (parent->children[0].fchild);
        for (i=0; i<(level+1); i++)
            printf("\t");
        print_users (parent->children[1].fchild);
    }
}

void print_users (Node *parent) {
    if (parent == NULL)
        printf("NULL\n");
    else {
        printf("%s, ", parent->str);
        while (parent->next != NULL) {
            parent = parent->next;
            printf("%s, ", parent->str);
        }
        printf ("\n");
    }
}

// find the branch that answer corresponds to, starting from
// node current
QNode *find_branch(QNode *current, int answer){
    return current->children[answer].qchild;
}

// add user to the end of the user list and return the list
Node *add_user(Node *head, char *user){
    // create a new user node
    Node *new = malloc(sizeof(Node));
    if (!new){
        perror("malloc");
        exit(1);
    }

    char *newstr = malloc((strlen(user)+1)*sizeof(char));
    if (!newstr){
        perror("malloc");
        exit(1);
    }
    strcpy(newstr, user);

    new->str = newstr;
    new->next = NULL;

    // list is empty
    if (!head){
        head = new;

    } else {
        // find the end of the list
        Node *current;
        for (current = head; current->next; current = current->next);

        current->next = new;
    }

    return head;
}

// find a user in the tree
// return NULL if the user was not found
Node *find_user(QNode *current, char *name){
    // current is an inner node
    if (current->node_type == REGULAR){

        Node *head;
        // look for the node (recursively) in the 0 subtree
        head = find_user(current->children[0].qchild, name);
        if (head)
            return head;

        // look for the node (recursively) in the 1 subtree
        head = find_user(current->children[1].qchild, name);
        if (head)
            return head;

    // current is a leaf node
    } else {
        // look for the user in the 0 child
        Node *head = current->children[0].fchild;

        while (head != NULL) {
            if (strcmp(head->str, name) == 0)
                return current->children[0].fchild;
            head = head->next;
        }

        // look for the user in the 1 child
        head = current->children[1].fchild;

        while (head != NULL) {
            if (strcmp(head->str, name) == 0)
                return current->children[1].fchild;
            head = head->next;
        }
    }

    return NULL;
}

//helper
int get_len(QNode * parent){
    int result = 0;
    QNode *temp = parent;
    while(temp->node_type == REGULAR){
      result ++;
      temp = temp->children[0].qchild;
    }
    result ++;
    return result;
}
// get a list of answers for a given username
int* find_answer(QNode *current, char *name){
    // current is an inner node
    int len = get_len(current);
    if(!find_user(current, name)){
       return NULL;
    }
    int * result = calloc(len,sizeof(int));
    QNode *cur = current;
    int index = 0;
    while(cur->node_type == REGULAR){
      if(find_user(cur->children[0].qchild,name)){
        cur = cur->children[0].qchild;
        result[index] = 0;
      }else{
        cur = cur->children[1].qchild;
        result[index] = 1;
      }
      index ++;
    }
    result[len-1] = 0;
    Node *head = cur->children[1].fchild;
    while (head != NULL) {
        if (strcmp(head->str, name) == 0)
            result[len-1] = 1;
        head = head->next;
    }
    return result;

}
// find opposite
Node *find_opposite(QNode *current, char *name){
    // current is an inner node
    int * path = find_answer(current, name);
    int path_len = get_len(current);
    QNode * cur = current;
    for(int i = 0; i<path_len-1;i++){
      if(path[i] ==1){
        cur = cur->children[0].qchild;
      }else{
        cur = cur->children[1].qchild;
      }
    }
    if(path[path_len-1] == 0){
      free(path);
      return cur->children[1].fchild;
    }else{
      free(path);
      return cur->children[0].fchild;
    }
}
// free tree
void free_qtree(QNode *current){
    if (current){
        if (current->node_type == LEAF){
            // free lists of users
            free_list(current->children[0].fchild);
            free_list(current->children[1].fchild);

        } else {
            // free left subtree
            free_qtree(current->children[0].qchild);
            // free right subtree
            free_qtree(current->children[1].qchild);
        }

        free(current->question); // free question
        free(current);
    }
}
  // from categorizer
  int validate_answer(char answer[BUFSIZE]){
      if (strlen(answer) > 5){
          printf("1");
          return 2;
      }

      if (answer[0] == 'n' || answer[0] == 'N')
          return 0;

      if (answer[0] == 'y' || answer[0] == 'Y')
          return 1;
      answer[BUFSIZE-1] = '\0';
      printf("%s \n",answer);
      return 2;
  }

  void print_friends(Node *list,int fd){
      print_text (fd,"Here are your best mismatches\n");
      // iterate over user list and count the number of friends
      Node * cur = list;
      while (cur) {
          print_text (fd,cur->str);
          print_text (fd,"\n");
          cur = cur->next;
      }
  }

int find_network_newline(char *buf, int inbuf){
      int i;
      for(i=0;i<inbuf -1 ;i++)
          if((buf[i]=='\r') && (buf[i+1]=='\n'))
              return i;
      return -1;
}


//helper method for partical reading
char * full_read (int fd){
      char *result = calloc(BUFSIZE,sizeof(char));;
      bzero(result, BUFSIZE);
      result[0] ='\0';
      char temp[10];
      bzero(temp, 10);
      int n ;
      while(1){
        bzero(temp, 10);
        n = read(fd,temp,10);
        temp[n] = '\0';
        strcat(result,temp);
        if(find_network_newline(result,strlen(result))!=1){
           return result;
        }
      }

}
void print_client(struct client ** head){
    struct client * cur = *head;
    while(cur != NULL){
      printf("%s\n",cur->username);
      cur = cur->next;
    }
}
  /*
   * Read and process commands
   */
  int process_args(int cmd_argc, char **cmd_argv, QNode **root, Node *interests,
  		 struct client *current_client, struct client *head) {
  	QNode *qtree = *root;

  	if (cmd_argc <= 0) {
  		return 0;

  	} else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
  		/* Return an appropriate value to denote that the specified
  		 * user is now need to be disconnected. */
      print_text (current_client->fd,"See you next time.\n");
  		return -1;

  	} else if (strcmp(cmd_argv[0], "do_test") == 0 && cmd_argc == 1) {
  		/* The specified user is ready to start answering questions. You
  		 * need to make sure that the user answers each question only
  		 * once.
  		 */

       // check whether the user with this name already exists
       Node *user_list = find_user(qtree, current_client->username);

       if (!user_list){
           // find the list of friends to which the user should be attached
           QNode *prev, *curr;
           prev = curr = qtree;

           // iterate over list of interests
           Node *i = interests;
           int ans;
           //char buf[BUFSIZE];
           while (i){
               bzero(current_client->buf, BUFSIZE);
               strcpy(current_client->buf,"Do you like ");
               strcat(current_client->buf, i->str);
               strcat(current_client->buf, "? \n");
               print_text (current_client->fd,current_client->buf);
               // read answer to prompt
               bzero(current_client->buf, BUFSIZE);
               int n = read(current_client->fd,current_client->buf,BUFSIZE);
               if(n == 0){
                 return -1;
               }
               if(strcmp(current_client->buf,"\r\n")==0){
                 bzero(current_client->buf, BUFSIZE);
                 n = read(current_client->fd,current_client->buf,BUFSIZE);
                 if(n == 0){
                   return -1;
                 }
               }
               ///
               ans = validate_answer(current_client->buf);
               // if answer if not valid, continue to loop
               if (ans == 2)
                   continue;

               prev = curr;
               curr = find_branch(curr, ans);

               i = i->next;
           }
           print_text (current_client->fd,"Test compeled.\n");
           // add user to the end of the list
           user_list = prev->children[ans].fchild;
           prev->children[ans].fchild = add_user(user_list,current_client->username );
           current_client->answers = find_answer(qtree, current_client->username);
       }else{
          print_text (current_client->fd,"You have already done the test.\n");
       }

  	} else if (strcmp(cmd_argv[0], "get_all") == 0 && cmd_argc == 1) {
  		/* Send the list of best mismatches related to the specified
  		 * user. If the user has not taked the test yet, return the
  		 * corresponding error value (different than 0 and -1).
  		 */
       Node *user_list = find_user(qtree, current_client->username);
       if(!user_list){
         return -2;
       }
       Node * result = find_opposite(qtree, current_client->username);
       if(!result){
         print_text (current_client->fd,"No completing personalities found.Please try again later\n");
       }else{
         print_friends(result,current_client->fd);
       }
  	} else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc == 3) {
  		/* Send the specified message stored in cmd_argv[2] to the user
  		 * stored in cmd_argv[1].
  		 */
       printf("sending message between client.\n");
       struct client * cur = head;
       struct client * sendto = NULL;
       while(cur != NULL){

         if(strncmp(cur->username, cmd_argv[1],128)==0){
           sendto = cur;
         }
         cur = cur->next;
       }
       if(!sendto){
         print_text (current_client->fd,"The person you posting is not online.\n");
       }else{
         print_text (sendto->fd,cmd_argv[2]);
         print_text (sendto->fd," \n");
       }
  	}
  	else {
  		/* The input message is not properly formatted. */
      print_text (current_client->fd,"Invalid command.\n");
  		error("Incorrect syntax");
  	}
  	return 0;
  }
