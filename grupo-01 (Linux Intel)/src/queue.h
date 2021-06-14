    #include<stdio.h>
    #include <stdlib.h>
     


// A linked list (LL) node to store a queue entry
struct QNode {
	char* key;
	struct QNode* next;
};

// The queue, front stores the front node of LL and rear stores the
// last node of LL
struct Queue {
	struct QNode *front, *rear;
};

// A utility function to create a new linked list node.
struct QNode* newNode(char* k);

// A utility function to create an empty queue
struct Queue* createQueue();

// The function to add a key k to q
void enQueue(struct Queue* q, char* k);

// Function to remove a key from given queue q
void deQueue(struct Queue* q);