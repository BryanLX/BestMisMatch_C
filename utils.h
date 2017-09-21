#ifndef _UTILS_H
#define _UTILS_H

#include <netinet/in.h>
#define MAX_NAME 128
#define INPUT_ARG_MAX_NUM 3
#define DELIM " \r\n"
#define MAX_LINE 256

/*
 * This document is orginally copy from a4 helper ducument,I combined all the
 * fucntion needed for this assignment in this utils.
 */
typedef struct client {
    int fd;
    int * answers;
    int state;
    char buf[1024];
    char username[128];
    struct client *next;
}Client;

typedef enum {
    REGULAR, LEAF
} NodeType;

union Child {
	struct str_node *fchild;
	struct QNode *qchild;
} Child;

typedef struct QNode {
	char *question;
	NodeType node_type;
	union Child children[2];
} QNode;

typedef struct str_node {
	char *str;
	struct str_node *next;
} Node;
/*
 * Print a formatted error message to stderr.
 */
void error(char *);

/*
 * Read and process commands
 */
int process_args(int, char **, QNode **, Node *, struct client *, struct client *);


/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *, char **);
void print_text (int childfd,char text[1024]);
QNode *add_next_level (QNode *current, Node * list_node);

void print_qtree (QNode *parent, int level);
void print_users (Node *parent);

Node *add_user(Node *head, char *user);
QNode *find_branch(QNode *current, int answer);
Node *find_user(QNode *current, char *name);
void free_qtree(QNode *root);
int* find_answer(QNode *current, char *name);
int get_len(QNode * parent);
Node *find_opposite(QNode *current, char *name);
int validate_answer(char *answer);
void print_friends(Node *list,int fd);
Node * get_list_from_file (char *input_file_name);
void print_list (Node *head);
void free_list (Node *head);


#endif /* _UTILS_H */
