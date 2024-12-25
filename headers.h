#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>  // For standard bool, true, false









#define SHKEY 300

///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================

int getClk()
{
    return *shmaddr;
}

typedef struct {
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    int size;
} processData;

// Message structure for process arrival
typedef struct {
    long mtype;
    processData process;
} process_msgbuff;

// Message structure for process termination
typedef struct {
    long mtype;
    int pid;
    int status; // status = 1 means process has finished
} termination_msgbuff;

/*
* All process call this function at the beginning to establish communication between them and the clock module.
* Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}

/*
* All process call this function at the end to release the communication
* resources between them and the clock module.
* Again, Remember that the clock is only emulation!
* Input: terminateAll: a flag to indicate whether that this is the end of simulation.
*                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

// Process states
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
} ProcessState;

// Process Control Block structure
typedef struct {
    int pid;                 
    int arrival_time;      
    int runtime;           
    int priority;          
    int remaining_time;    
    int waiting_time;      
    int start_time;        
    ProcessState state;    
    int last_run;
    int id;
    int size;         
} PCB;

//----------------------queue---------------------------------
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// A structure to represent a queue
typedef struct {
    int front, rear, size;
    unsigned capacity;
    PCB** array;
} Queue;

// function to create a queue
// of given capacity.
// It initializes size of queue as 0
Queue* createQueue(unsigned capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (PCB**)malloc(queue->capacity * sizeof(PCB*));
    return queue;
}

// Queue is full when size becomes
// equal to the capacity
int isFull(Queue* queue) {
    return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmptyQ(Queue* queue) {
    return (queue->size == 0);
}

// Enqueue function for HPF (Priority Queue)
// Processes are ordered based on priority (lower number means higher priority)
void enqueue(Queue* queue, PCB* item) {
    if (isFull(queue)) {
        printf("Queue is full. Cannot enqueue item.\n");
        return;
    }

    int i;

    // If the queue is empty, insert at position 0
    if (queue->size == 0) {
        queue->array[0] = item;
        queue->front = 0;
        queue->rear = 0;
        queue->size = 1;
        return;
    }

    // Start from the last element and shift items to the right to make room
    i = queue->size - 1;

    // Adjust the condition based on priority and arrival time
    while (i >= 0 && (queue->array[i]->priority > item->priority ||
          (queue->array[i]->priority == item->priority && queue->array[i]->arrival_time > item->arrival_time))) {
        queue->array[i + 1] = queue->array[i];
        i--;
    }

    // Insert the new item at the correct position
    queue->array[i + 1] = item;

    // Update rear and size
    queue->size++;
    queue->rear = queue->size - 1;
}

// Enqueue function for SJF
// Processes are ordered based on remaining time
void enqueue_SJF(Queue* queue, PCB* item) {
    if (isFull(queue)) {
        printf("Queue is full. Cannot enqueue item.\n");
        return;
    }

    int i;

    // If the queue is empty, insert at position 0
    if (queue->size == 0) {
        queue->array[0] = item;
        queue->front = 0;
        queue->rear = 0;
        queue->size = 1;
        return;
    }

    // Start from the last element and shift items to the right to make room
    i = queue->size - 1;

    // Adjust the condition based on remaining_time
    while (i >= 0 && queue->array[i]->remaining_time > item->remaining_time) {
        queue->array[i + 1] = queue->array[i];
        i--;
    }

    // Insert the new item at the correct position
    queue->array[i + 1] = item;

    // Update rear and size
    queue->size++;
    queue->rear = queue->size - 1;
}

// Enqueue function for RR
// Processes are enqueued in FCFS order
void enqueue_RR(Queue* queue, PCB* item) {
    if (isFull(queue)) {
        printf("Queue is full. Cannot enqueue item.\n");
        return;
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size++;
}

// Function to remove an item from queue.
// It changes front and size
PCB* dequeue(Queue* queue) {
    if (isEmptyQ(queue)) {
        printf("Queue is empty. Cannot dequeue item.\n");
        return NULL;
    }
    PCB* item = queue->array[queue->front];

    // Shift all elements one position to the left
    for (int i = 0; i < queue->size - 1; i++) {
        queue->array[i] = queue->array[i + 1];
    }

    queue->size--;
    queue->rear = queue->size - 1;

    // Optional: Set the now-unused position to NULL (good practice)
    queue->array[queue->size] = NULL;

    // If the queue is now empty, reset front and rear
    if (queue->size == 0) {
        queue->front = 0;
        queue->rear = -1;
    }

    return item;
}

// Function to get front of queue
PCB* front(Queue* queue) {
    if (isEmptyQ(queue)) {
        return NULL;
    }
    return queue->array[queue->front];
}

// Function to get rear of queue
PCB* rear(Queue* queue) {
    if (isEmptyQ(queue)) {
        return NULL;
    }
    return queue->array[queue->rear];
}







typedef struct Node {
    int size;          // Size of the memory block
     PCB* pcb;   // Process occupying the block (NULL if free)
    struct Node* left; // Left buddy
    struct Node* right; // Right buddy
} Node;

// Function to create a new node
Node* createNode(int size, PCB* pcb) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->size = size;
    newNode->pcb = pcb;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}



// Function to create a new PCB

// Function to round up to the nearest power of two
int roundUpToPowerOfTwo(int x) {
    int power = 1;
    while (power < x) {
        power *= 2;
    }
    return power;
}



void deallocate(Node* root, PCB* pcb) {
    
    if (root == NULL) return;
    
    // If the block is occupied by the given PCB, deallocate it
    if (root->pcb == pcb) {
        printf("the process %d has deallocated %d\n",pcb->id,root->size);
        root->pcb = NULL;
        return;
    }

    // Recurse into left and right children
    deallocate(root->left, pcb);
    deallocate(root->right, pcb);

    // Merge buddies if both are free
    if (root->left && root->right &&
        root->left->pcb == NULL && root->right->pcb == NULL &&
        root->left->left == NULL && root->right->right == NULL) {
        free(root->left);
        free(root->right);
        root->left = NULL;
        root->right = NULL;
    }
}



Node* findFreeBlock(Node* root, int size) {
    if (root == NULL) return NULL;

    Node* smallestBlock = NULL;

    // If this is a leaf node and it's free (no allocated PCB)
    if (root->left == NULL && root->right == NULL && root->pcb == NULL) {
        if (root->size >= size) {  // The block can fit the requested size
            if (smallestBlock == NULL || root->size < smallestBlock->size) {
                smallestBlock = root;  // Keep track of the smallest fitting block
            }
        }
    }

    // Recursively search both subtrees for free blocks
    Node* leftBlock = findFreeBlock(root->left, size);
    if (leftBlock != NULL) {
        if (smallestBlock == NULL || leftBlock->size < smallestBlock->size) {
            smallestBlock = leftBlock;  // Update smallest if necessary
        }
    }

    Node* rightBlock = findFreeBlock(root->right, size);
    if (rightBlock != NULL) {
        if (smallestBlock == NULL || rightBlock->size < smallestBlock->size) {
            smallestBlock = rightBlock;  // Update smallest if necessary
        }
    }

    return smallestBlock;  // Return the smallest available block that fits the size
}

void splitBlock(Node* block) {
    if (block == NULL || block->left != NULL || block->right != NULL) return;  // If already split or invalid node

    int halfSize = block->size / 2;

    // Create left and right children (split the block)
    block->left = createNode(halfSize,NULL);
    block->right = createNode(halfSize,NULL);

    printf("Block of size %d split into two blocks of size %d.\n", block->size, halfSize);
}

Node* allocateMemoryWithSplit(Node* root, int size, PCB*b) {
    int roundedSize = roundUpToPowerOfTwo(size);  // Round the size up to the nearest power of 2
    printf("Requesting %d bytes, rounded to %d bytes.\n", size, roundedSize);

    // First, try to find an exact match for the requested size
    Node* block = findFreeBlock(root, roundedSize);

    // If we found a block of the exact size, allocate it
    if (block != NULL && block->size == roundedSize) {
        // Allocate the block by assigning a PCB
        block->pcb =b ;
        printf("The process %d has allocated memory %d\n",b->id,block->size);
    
        return block;
    }
    
    // If no exact match, find a larger block and split it
    if (block == NULL || block->size > roundedSize) {
       

        
            // Split the larger block into smaller blocks
            splitBlock(block);
            // Allocate memory from the smaller blocks after splitting
            return allocateMemoryWithSplit(root, size, b);
            }
            else {
            // No suitable block available
            printf("Memory allocation failed: No suitable block available.\n");
            return NULL;
                 }
    

    return NULL;  // Fallback if no suitable block was found
}



























#endif // HEADERS_H
