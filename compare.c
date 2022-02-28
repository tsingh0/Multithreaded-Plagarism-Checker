#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#ifndef QSIZE
#define QSIZE 100
#define BOUND 1
#define UNBOUND 0
#endif

struct node *new_node(char *word, int length);
void insertFile(char *name, struct node *root);
void printFileList();
void inorder(struct node *root);
struct node *insert(struct node *root, char *word, int length);
void free_tree(struct node *root);
void computeFreq(struct node *root, float total);
struct node *read_from_file(int fd);
struct node *make_string(struct node *root, int stringIndex, char *currentString);
unsigned set_value(char *arg);
int updated_set_value(char *arg);
char *set_suffix(char *arg);
int isreg(char *name);
int isdir(char *name);
int recursive_read_directory(DIR *dirp, char *suffix, char *directoryName);

struct queue_node *allocate_Node(char *fileName);
int print_queue(struct queue_node *head);
int getPath(char *fileName, char *directory, char *path);
int getDirectoryPath(char *directory, char *path);

struct node *Trees_new_node(char *word, unsigned length, float freq, int count);
struct node *Trees_insert(struct node *root, char *word, unsigned length, float freq, int count);
struct node *copyTree(struct node *tree, struct node *root);
float calculate_KLD(struct node *tree1, struct node *combinedTree);
float findInTree(struct node *root, char *word);
int check(char *string1, char *suffix);
//void JSD_Calculation(struct fileList *tree1, struct fileList *tree2);
int getCount();
int totalWords(struct node *root, int count);
typedef struct
{
    struct fileList *data[QSIZE][2];
    unsigned count;
    unsigned head;
    int open;
    pthread_mutex_t lock;
    pthread_cond_t read_ready;
    pthread_cond_t write_ready;
} queue2_t;

int init2(queue2_t *Q);
int destroy2(queue2_t *Q);
int enqueue2(queue2_t *Q, struct fileList *item1, struct fileList *item2);
int dequeue2(queue2_t *Q, struct fileList **item1, struct fileList **item2);
int qclose2(queue2_t *Q);

struct targs2
{
    queue2_t *queue;
};

typedef struct
{
    unsigned count;
    int bounded; //if we pass one into init we knwo the Q is bounded otherwise unbounded
    struct queue_node *head;
    struct queue_node *tail;
    int open;
    int active;
    pthread_mutex_t lock;
    pthread_cond_t read_ready;
    pthread_cond_t write_ready;
} queue_file;

typedef struct
{
    unsigned count;
    struct pair_node *head;
    struct pair_node *tail;
    int open;
    pthread_mutex_t lock;
    pthread_cond_t read_ready;
    pthread_cond_t write_ready;
} jsd_queue;

struct queue_node
{
    char *fileName;
    struct queue_node *next;
};

int destroy(queue_file *Q);
int init(queue_file *Q, int bounded);
int free_queue(struct queue_node *head);
int enqueue(queue_file *Q, char *fileName);
char *dequeue(queue_file *Q);
void *queue_directories(void *A);
void *queue_files(void *A);
int qclose(queue_file *Q);

struct node
{
    char *data;
    unsigned frequency;
    float realFrequency;
    struct node *leftChild;
    struct node *rightChild;
};
struct pair_node
{
    char *fileOne;
    char *fileTwo;
};

struct fileList
{
    char *filename;
    struct node *wordsTree;
    struct fileList *next;
};
void free_FileList(struct fileList *head);
struct threadArgs
{
    queue_file *directoryQ;
    queue_file *fileQ;
    char *suffix;
    int id;
};
struct threadArgs2
{
    queue_file *fileQ;
    int id;
};

struct anal_output
{
    char *fileOne;
    char *fileTwo;
    int totalWords;
    float JSD;
    struct anal_output *next;
};
struct anal_list
{
    pthread_mutex_t lock;
    pthread_cond_t read_ready;
    pthread_cond_t write_ready;
    struct anal_output *head;
    int size;
};

struct anal_list *finalList = 0;
struct anal_list *add_to_list(struct anal_list *list, struct anal_output *temp);
void anal_init();
int free_anal(struct anal_output *, int arraysize);
void anal_destroy();
struct anal_output *allocate_anal(char *fileOne, char *fileTwo, float JSD, int words);
void printFinalOutput(struct anal_output *output, int arraysize);
void insertSort(struct anal_output **head, int arraysize);
void sortedInsert(struct anal_output **head, struct anal_output *newNode);
struct anal_output *JSD_Calculation(struct fileList *tree1, struct fileList *tree2);

struct wordNode_t
{
    char *word;
    float frequency;
};
void getWordTreeCount(struct node *root, int *count);
void makeWordArray(struct node *root, struct wordNode_t *nodeArray, int *count);
float getWordFreq(struct node *root, char *word);
void *jsd_thread(void *arg);
void *enqueue_thread(void *arg);
struct fileList *GetFileElement(int idx);
struct fileList *head; //GLOBAL VARIABLE - THE LIST THAT HOLDS ALL THE FILES which contains the trees

int main(int argc, char **argv)
{

    head = NULL;
    finalList = malloc(sizeof(struct anal_list));
    anal_init();
    if (argc < 2)
    {
        char *error = "Invalid input";
        fprintf(stderr, error, strlen(error));
    }

    unsigned int directoryThread = 1;
    unsigned int fileThread = 1;
    unsigned int analysisThread = 1;
    unsigned int invalid = 0;
    char *suffix = ".txt";
    queue_file *directoryQ = malloc(sizeof(queue_file));
    init(directoryQ, UNBOUND);
    queue_file *fileQ = malloc(sizeof(queue_file));
    init(fileQ, UNBOUND);

    for (int i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "-d", 2) && strlen(argv[i]) > 2)
            directoryThread = updated_set_value(argv[i]);
        else if (!strncmp(argv[i], "-f", 2) && strlen(argv[i]) > 2)
            fileThread = updated_set_value(argv[i]);
        else if (!strncmp(argv[i], "-a", 2) && strlen(argv[i]) > 2)
            analysisThread = updated_set_value(argv[i]);
        else if (!strncmp(argv[i], "-s", 2) && strlen(argv[i]) >= 2)
            suffix = set_suffix(argv[i]);
        else if (!strncmp(argv[i], "-", 1))
            invalid = 1;
        if (directoryThread == 0 || fileThread == 0 || analysisThread == 0 || strlen(argv[i]) < 2 || invalid)
        { //if bad input exit failure
            //printf("%s %ld\n", argv[i], strlen(argv[i]));
            destroy(directoryQ);
            destroy(fileQ);
            printf("Incorrect Input Arguments!\n");
            return EXIT_FAILURE;
        }
        else if (isdir(argv[i]))
        {
            enqueue(directoryQ, argv[i]);

        } //checks if the arg is a file //if a file and no set destination we write to terminal
        else if ((isreg(argv[i]) && check(argv[i], suffix)) || (isreg(argv[i]) && check(argv[i], suffix) == -1))
        {
            enqueue(fileQ, argv[i]);
        }
    }

    //HERE IS THE BEGINNIG OF THREADING
    pthread_t *tids = malloc((directoryThread + fileThread) * sizeof(pthread_t));
    struct threadArgs *args = (struct threadArgs *)malloc(sizeof(struct threadArgs));
    struct threadArgs2 *args2 = (struct threadArgs2 *)malloc(sizeof(struct threadArgs2));
    args2->fileQ = fileQ;
    args->suffix = suffix;
    args->directoryQ = directoryQ;
    args->fileQ = fileQ;
    for (int i = 0; i < directoryThread; i++)
    {
        //printf("Thread running: %d\n", i);
        args->id = i;
        int rc = pthread_create(&tids[i], NULL, (void *)queue_directories, (void *)args);
        if (rc)
        {
            printf("ERROR; pthread_create() return code is %d\n", rc);
            exit(-1);
        }
    }
    for (int i = 0; i < directoryThread; i++)
    {
        pthread_join(tids[i], NULL);
    }
    //file threads must dequque all files from queue tokenize the file and compute its WFD, and add to the WFD list
    //so call a method that is going to dequeue file and
    for (int i = 0; i < fileThread; i++)
    {
        //printf("Thread running: %d\n", i);
        args2->id = i;
        int rc = pthread_create(&tids[i], NULL, (void *)queue_files, (void *)args2);
        if (rc)
        {
            printf("ERROR; pthread_create() return code is %d\n", rc);
            exit(-1);
        }
    }
    for (int i = 0; i < fileThread; i++)
    {
        pthread_join(tids[i], NULL);
    }
    //  printf("fileQ count :%d \n", fileQ->count);
    //printFileList();
    //free_FileList(head);
    free(args);
    free(args2);
    qclose(directoryQ);
    qclose(fileQ);
    destroy(directoryQ);
    destroy(fileQ);
    free(tids);
    pthread_t *tids2 = malloc((analysisThread + 1) * sizeof(pthread_t));
    queue2_t queue;
    init2(&queue);

    struct targs2 *args3;
    args3 = malloc(sizeof(struct targs2));
    args3->queue = &queue;
    pthread_create(&tids2[0], NULL, enqueue_thread, args3);

    for (int i = 1; i < analysisThread + 1; i++)
    {
        pthread_create(&tids2[i], NULL, jsd_thread, args3);
    }
    for (int i = 0; i < analysisThread + 1; ++i)
    {
        pthread_join(tids2[i], NULL);
    }
    struct anal_output *link = finalList->head;
    insertSort(&link, finalList->size);
    printFinalOutput(link, finalList->size);
    anal_destroy();
    destroy2(&queue);
    free(args3);
    free(tids2);

    return EXIT_SUCCESS;
}
int check(char *string1, char *suffix)
{
    if (suffix == 0)
    {
        return -1;
    }
    int length = strlen(suffix);
    int lengthOfFileString = strlen(string1);
    if (length > lengthOfFileString)
    { //suffix is longer than the string
        return 0;
    }
    const char *suffixOfFile = &string1[lengthOfFileString - length];
    //printf("SUFFIX : %s suffixOfFile %s\n", suffix, suffixOfFile);
    if (strcmp(suffix, suffixOfFile) == 0)
    {
        return 1;
    }
    return 0;
}
void printFinalOutput(struct anal_output *output, int arraysize)
{
    printf("This Final Output:\n------------------\n");
    struct anal_output *temp = output;
    for (int i = 0; i < arraysize; i++)
    {
        printf("%f %s %s\n", temp->JSD, temp->fileOne, temp->fileTwo);
        if (i != arraysize - 1)
            temp = temp->next;
    }
}

int getCount()
{
    int count = 0;                   // Initialize count
    struct fileList *current = head; // Initialize current
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}

/**
 * 
 * 
 * ALL CODE TO DO WITH QUEUE 1
 * 
 * 
 */
int enqueue(queue_file *Q, char *fileName)
{
    pthread_mutex_lock(&Q->lock);
    while (Q->count == QSIZE && Q->open && Q->bounded)
    {
        pthread_cond_wait(&Q->write_ready, &Q->lock);
    }
    if (!Q->open)
    {
        pthread_mutex_unlock(&Q->lock);
        return -1;
    }
    if (Q->head == 0) //empty queue
    {
        Q->head = allocate_Node(fileName);
        //("enqueue1 : %s\n", Q->head->fileName);
        Q->tail = Q->head;
    }
    else
    { //something in the queus
        // printf("enqueue2 : %s\n", fileName);
        Q->tail->next = allocate_Node(fileName);
        Q->tail = Q->tail->next;
    }
    ++Q->count;

    pthread_cond_signal(&Q->read_ready);

    pthread_mutex_unlock(&Q->lock);

    return 0;
}
char *dequeue(queue_file *Q)
{
    ++Q->active;
    //("Q->Active %d\n", Q->active);
    while (Q->count == 0 && Q->open)
    {
        --Q->active;
        if (Q->active == 0)
        {
            pthread_mutex_unlock(&Q->lock);
            return NULL;
        }
        pthread_cond_wait(&Q->read_ready, &Q->lock);
    }
    if (Q->count == 0)
    {
        --Q->count;
        pthread_mutex_unlock(&Q->lock);

        return 0;
    }
    char *path;
    if (Q->count >= 2)
    {
        struct queue_node *temp = Q->head;
        //printf("Deque : %s\n", temp->fileName);
        path = malloc(strlen(temp->fileName) + 1);
        strcpy(path, temp->fileName);
        Q->head = Q->head->next;
        free(temp->fileName);
        free(temp);
    }
    else
    {
        path = malloc(strlen(Q->head->fileName) + 1);
        strcpy(path, Q->head->fileName);
        //printf("Deque : %s\n", Q->head->fileName);
        free(Q->head->fileName);
        free(Q->head);
        Q->head = NULL;
    }
    --Q->count;
    --Q->active;
    pthread_cond_signal(&Q->write_ready);
    //printf("Deque : %s\n", path);
    return path;
}
int init(queue_file *Q, int bounded)
{
    Q->count = 0;
    Q->head = 0;
    Q->tail = 0;
    Q->open = 1;
    Q->active = 0;
    Q->bounded = bounded;
    pthread_mutex_init(&Q->lock, NULL);
    pthread_cond_init(&Q->read_ready, NULL);
    pthread_cond_init(&Q->write_ready, NULL);
    return 0;
}

int destroy(queue_file *Q)
{
    pthread_mutex_destroy(&Q->lock);
    pthread_cond_destroy(&Q->read_ready);
    pthread_cond_destroy(&Q->write_ready);
    free_queue(Q->head);
    free(Q);
    return 0;
}
int qclose(queue_file *Q)
{
    pthread_mutex_lock(&Q->lock);
    Q->open = 0;
    pthread_cond_broadcast(&Q->read_ready);
    pthread_cond_broadcast(&Q->write_ready);
    pthread_mutex_unlock(&Q->lock);

    return 0;
}
void *queue_files(void *A)
{
    struct threadArgs2 *args = A;
    queue_file *fileQ = args->fileQ;
    while (fileQ->count > 0)
    {
        pthread_mutex_lock(&fileQ->lock);
        char *path1 = dequeue(fileQ);
        if (path1 == NULL)
        {
            continue;
        }
        //printf("Thread : %d, dequeue: %s\n ", thread, path1);
        int openFile = open(path1, O_RDONLY);
        if (openFile == -1)
        {
            perror("error opening  file\n");
            abort();
        }

        struct node *FileTree = read_from_file(openFile);

        close(openFile);
        insertFile(path1, FileTree); //inserts file into the fileLIst
        pthread_mutex_unlock(&fileQ->lock);
        free(path1);
    }

    return 0;
}
void *queue_directories(void *A)
{
    struct threadArgs *args = A;
    char *suffix = args->suffix;
    queue_file *directoryQ = args->directoryQ;
    queue_file *fileQ = args->fileQ;
    while (directoryQ->count > 0)
    {
        pthread_mutex_lock(&directoryQ->lock);
        char *path1 = dequeue(directoryQ); //dequeues the directory in the list //path1 is malloced in deque
        pthread_mutex_unlock(&directoryQ->lock);
        if (path1 == NULL)
        {
            continue;
        }
        DIR *dirp = opendir(path1); //opens the directory
        struct dirent *de;          //in the main thread lets make directory name the path to the directory

        while ((de = readdir(dirp))) //reads the contents of that directory and enques the shit
        {
            char path[PATH_MAX];
            memset(&path[0], 0, sizeof(path));
            strcat(path, path1);
            strcat(path, "/");
            char *file_or_directory_name = de->d_name;
            strcat(path, file_or_directory_name);

            if (!check(file_or_directory_name, suffix) || check(file_or_directory_name, suffix) == -1) //suffix is null so it returns -1
            {                                                                                          //checicking to see if the suffix matches if it doesnt continue but first check if the file is a directory
                if (isdir(path) && file_or_directory_name[0] != '.')
                {
                    //means its a directory and not a . so we add it to the queue //may be an edge case if our suffix is the directory name
                    //printf("Thread : %d, enque %s\n", id, path);
                    enqueue(directoryQ, path);
                }
            }
            if (isreg(path) && (check(file_or_directory_name, suffix) || check(file_or_directory_name, suffix) == -1))
            {
                enqueue(fileQ, path);
                // printf("Thread : %d, enque %s\n", id, path);
            }
        }
        closedir(dirp);
        free(path1);
    }
    //pthread_mutex_unlock(&directoryQ->lock);
    return 0;
}
/**
 * 
 * 
 * 
 * ALL CODE FOR QUEUE 2
 * 
 * 
 * 
 */
int init2(queue2_t *Q)
{
    Q->count = 0;
    Q->head = 0;
    Q->open = 1;
    pthread_mutex_init(&Q->lock, NULL);
    pthread_cond_init(&Q->read_ready, NULL);
    pthread_cond_init(&Q->write_ready, NULL);

    return 0;
}

int destroy2(queue2_t *Q)
{
    pthread_mutex_destroy(&Q->lock);
    pthread_cond_destroy(&Q->read_ready);
    pthread_cond_destroy(&Q->write_ready);

    return 0;
}

// add item to end of queue
// if the queue is full, block until space becomes available
int enqueue2(queue2_t *Q, struct fileList *item1, struct fileList *item2)
{
    pthread_mutex_lock(&Q->lock);

    while (Q->count == QSIZE && Q->open)
    {
        pthread_cond_wait(&Q->write_ready, &Q->lock);
    }
    if (!Q->open)
    {
        pthread_mutex_unlock(&Q->lock);
        return -1;
    }

    unsigned i = Q->head + Q->count;
    if (i >= QSIZE)
        i -= QSIZE;

    Q->data[i][0] = item1;
    Q->data[i][1] = item2;
    ++Q->count;

    pthread_cond_signal(&Q->read_ready);

    pthread_mutex_unlock(&Q->lock);

    return 0;
}

int dequeue2(queue2_t *Q, struct fileList **item1, struct fileList **item2)
{
    pthread_mutex_lock(&Q->lock);

    if (Q->count == 0)
    {
        pthread_mutex_unlock(&Q->lock);
        return -1;
    }

    *item1 = Q->data[Q->head][0];
    *item2 = Q->data[Q->head][1];
    --Q->count;
    ++Q->head;
    if (Q->head == QSIZE)
        Q->head = 0;

    pthread_cond_signal(&Q->write_ready);

    pthread_mutex_unlock(&Q->lock);

    return 0;
}

int qclose2(queue2_t *Q)
{
    pthread_mutex_lock(&Q->lock);
    Q->open = 0;
    pthread_cond_broadcast(&Q->read_ready);
    pthread_cond_broadcast(&Q->write_ready);
    pthread_mutex_unlock(&Q->lock);

    return 0;
}

/**
 * 
 * 
 * 
 * ALL ANAL CODE
 * 
 * 
 * 
 */
struct anal_list *add_to_list(struct anal_list *list, struct anal_output *temp)
{
    pthread_mutex_lock(&list->lock);
    ++list->size;
    if (list->head == NULL)
    {
        list->head = temp;
    }
    else
    {
        temp->next = list->head;
        list->head = temp;
    }
    pthread_mutex_unlock(&list->lock);
    return list;
};
void anal_init()
{
    pthread_cond_init(&finalList->read_ready, NULL);
    pthread_cond_init(&finalList->write_ready, NULL);
    pthread_mutex_init(&finalList->lock, NULL);
    finalList->size = 0;
};

void anal_destroy()
{
    pthread_mutex_destroy(&finalList->lock);
    pthread_cond_destroy(&finalList->read_ready);
    pthread_cond_destroy(&finalList->write_ready);
    free_anal(finalList->head, finalList->size);
    free(finalList);
};
struct anal_output *allocate_anal(char *fileOne, char *fileTwo, float JSD, int words)
{
    struct anal_output *temp = malloc(sizeof(struct anal_output));
    temp->fileOne = malloc(strlen(fileOne) + 1);
    strcpy(temp->fileOne, fileOne);
    temp->fileTwo = malloc(strlen(fileTwo) + 1);
    strcpy(temp->fileTwo, fileTwo);
    temp->JSD = JSD;
    temp->totalWords = words;
    temp->next = 0;
    //printf("finished allocation\n");
    return temp;
}

/**
 * 
 * 
 * 
 * ALL CODE USED BY JSD
 * 
 * 
 * 
 */
void getWordTreeCount(struct node *root, int *count)
{
    if (root != NULL) // checking if the root is not null
    {
        getWordTreeCount(root->leftChild, count);
        (*count)++;
        getWordTreeCount(root->rightChild, count);
    }
}
void makeWordArray(struct node *root, struct wordNode_t *nodeArray, int *count)
{
    if (root != NULL) // checking if the root is not null
    {
        makeWordArray(root->leftChild, nodeArray, count);
        int found = 0;
        int count2 = (int)*count;
        for (int i = 0; i < count2; i++)
        {
            if (strcmp(root->data, nodeArray[i].word) == 0)
                found = 1;
        }

        if (!found) //we didnt find the word
        {
            int index = *count;
            nodeArray[index].word = malloc(strlen(root->data) + 1);
            strcpy(nodeArray[index].word, root->data); //buffer overflow here
            (*count)++;
        }

        makeWordArray(root->rightChild, nodeArray, count);
    }
}

float getWordFreq(struct node *root, char *word)
{
    if (root != NULL)
    {
        if (strcmp(root->data, word) == 0)
            return root->realFrequency;
        else if (strcmp(root->data, word) < 0)
            return getWordFreq(root->rightChild, word);
        else if (strcmp(root->data, word) > 0)
            return getWordFreq(root->leftChild, word);
    }

    return 0;
}
void *jsd_thread(void *arg)
{
    struct targs2 *queues = arg;
    struct fileList *item1;
    struct fileList *item2;

    while (1)
    {
        if (dequeue2(queues->queue, &item1, &item2) == -1)
        {
            //printf("end jsd thread : \r\n");
            break;
        }

        // printf("jsd thread %s -> %s: \r\n", item1->filename, item2->filename);
        int wordCount1 = 0, wordCount2 = 0;
        getWordTreeCount(item1->wordsTree, &wordCount1);
        getWordTreeCount(item2->wordsTree, &wordCount2);
        //printf("word count %d -> %d: \r\n", wordCount1, wordCount2);

        struct wordNode_t *wordNodes = malloc((wordCount1 + wordCount2) * sizeof(struct wordNode_t));
        int arrayCount = 0;
        makeWordArray(item1->wordsTree, wordNodes, &arrayCount);
        makeWordArray(item2->wordsTree, wordNodes, &arrayCount);
        //printf("Made word array \n");
        float KLD1 = 0.0;
        float KLD2 = 0.0;
        for (int i = 0; i < arrayCount; i++)
        {
            char *_word = wordNodes[i].word;
            //printf("array words %s: \r\n", _word);
            float word_frequency_1 = getWordFreq(item1->wordsTree, _word);
            float word_frequency_2 = getWordFreq(item2->wordsTree, _word);
            float word_avg_freq = (word_frequency_1 + word_frequency_2) * 0.5;

            if (word_frequency_1 != 0.0)
                KLD1 = KLD1 + word_frequency_1 * log2(word_frequency_1 / word_avg_freq);

            if (word_frequency_2 != 0.0)
                KLD2 = KLD2 + word_frequency_2 * log2(word_frequency_2 / word_avg_freq);
        }

        float JSD = sqrt(0.5 * (KLD1 + KLD2));
        //float word_frequency = getWordFreq(item2->wordsTree, "hi");
        //printf("JSD between %s and %s= %f: \r\n", item1->filename, item2->filename, JSD);
        int totalWords = wordCount1 + wordCount2;
        struct anal_output *out = allocate_anal(item1->filename, item2->filename, JSD, totalWords);
        add_to_list(finalList, out);

        for (int i = 0; i < arrayCount; i++)
        {
            free(wordNodes[i].word);
        }
        free(wordNodes);
        //printf("------------jsd thread END------------- \n");
    }
    return 0;
}
struct fileList *GetFileElement(int idx)
{
    int index = 0;
    struct fileList *temp = head;
    while (temp != NULL)
    {
        if (index == idx)
            break;

        index++;
        temp = temp->next;
    }

    return temp;
}
void *enqueue_thread(void *arg)
{
    struct targs2 *queues = arg;
    int fileCount = 0;

    struct fileList *temp = head;
    while (temp != NULL)
    {
        fileCount++;
        temp = temp->next;
    }

    temp = head;
    for (int i = 0; i != fileCount; i++)
    {
        for (int j = i + 1; j != fileCount; j++)
        {
            enqueue2(queues->queue, GetFileElement(i), GetFileElement(j));
        }
    }
    return 0;
}
/**
 * 
 * 
 * Any other code needed.
 * 
 * 
 */
/*
void printFileList()
{
    struct fileList *temp = head;
    printf("Entire List of files is: ");
    while (temp != NULL)
    {
        printf(" %s->", temp->filename);
        temp = temp->next;
    }
    printf("\n");
}
*/
int updated_set_value(char *arg) //fuckin tokenizes the arg to extract the number i assumed the number would be last token so keeps reseting value till last
{

    char c[3] = {arg[0], arg[1], '\0'};
    char *getNum = strtok(arg, c);
    int num = atoi(getNum);
    if (num > 0)
    {
        return num;
    }

    return 0;
}

void insertFile(char *name, struct node *root)
{
    //sets the node
    struct fileList *p = malloc(sizeof(struct fileList));
    p->filename = malloc(strlen(name) + 1);
    strcpy(p->filename, name);
    p->wordsTree = root;

    p->next = head;
    head = p;
}

struct node *insert(struct node *root, char *word, int length)
{
    //root is null means tree is empty new node is now root
    if (root == NULL)
    {
        return new_node(word, length);
    }
    //compare the word if == increase the frequency
    else if (!strcmp(root->data, word))
    {
        ++root->frequency;
        return root;
    }
    else if (strcmp(root->data, word) < 0)
        root->rightChild = insert(root->rightChild, word, length);
    else if (strcmp(root->data, word) > 0)
        root->leftChild = insert(root->leftChild, word, length);
    return root;
}

void computeFreq(struct node *root, float total)
{

    if (root != NULL) // checking if the root is not null
    {
        computeFreq(root->leftChild, total);
        root->realFrequency = (float)root->frequency / total;
        computeFreq(root->rightChild, total);
    }
}

struct node *read_from_file(int fd)
{
    char buffer[256];
    ssize_t bytesRead = 0;
    //read enough to fill the buffer

    bytesRead = read(fd, buffer, strlen(buffer));

    char *currentString = malloc(sizeof(char) * 10); //initial size of dynamically allocated string is 10 if we need to make it bigger we realloc
    struct node *root = NULL;                        //set root to null so we just return a new node on first insert
    int stringIndex = 0;

    int count = 0;
    while (bytesRead > 0)
    {
        for (int i = 0; i < bytesRead; i++)
        {
            //if we reach a space we are done with the word insert it into the tree
            if (isspace(buffer[i]) && stringIndex > 0)
            {
                root = make_string(root, stringIndex, currentString); //inserts the string into the string after building it.
                count++;
                stringIndex = 0; //reset the index to 0 for next word
                continue;
            }

            if (ispunct(buffer[i]) && buffer[i] != '-') //if its a punctuation we skip the iteration unless it is a "-", this avoids ' as well
                continue;

            if (isspace(buffer[i])) //safety check, if the character is a space we dont want to put it in the current string so continue
                continue;

            if (stringIndex >= sizeof(currentString)) //check if the stringIndex is greater than the current string length realloc if it is to fit it
                currentString = realloc(currentString, sizeof(char) * (stringIndex)*2);
            //before inserting make every character lowercase
            currentString[stringIndex] = tolower(buffer[i]);
            stringIndex++;
        }
        bytesRead = read(fd, buffer, strlen(buffer)); //read a new buffer in
    }
    if (stringIndex > 0) //if a string was not put in after all the bytes were read it puts it in
        root = make_string(root, stringIndex, currentString);
    count++;
    computeFreq(root, count);
    free(currentString);
    return root;
}

struct node *make_string(struct node *root, int stringIndex, char *currentString)
{
    char *stringBeingInserted = malloc(sizeof(char) * stringIndex + 1); //dynamically allocates a string to be inputted into tree
    memcpy(stringBeingInserted, currentString, stringIndex);            //copies n bytes into the string being inserted from the current string
    stringBeingInserted[stringIndex] = '\0';                            //makes it a null terminated string so we can use string.h methods on it
    root = insert(root, stringBeingInserted, stringIndex + 1);          //we insert the string into the tree, this could be multi threaded
    free(stringBeingInserted);
    return root;
}

int isdir(char *name)
{
    struct stat data;
    int err = stat(name, &data);
    if (err)
    {
        //perror(name); // print error message

        return 0;
    }
    if (S_ISDIR(data.st_mode))
    {
        return 1;
    }

    return 0;
}

int isreg(char *name)
{
    struct stat data;

    int err = stat(name, &data);

    if (err)
    {
        //perror(name); // print error message
        return 0;
    }
    if (S_ISREG(data.st_mode))
    {
        return 1;
    }
    return 0;
}

void free_FileList(struct fileList *head)
{
    while (head != NULL)
    {
        struct fileList *temp = head->next;
        free(head->filename);
        free_tree(head->wordsTree);
        free(head);
        head = temp;
    }
}
struct queue_node *allocate_Node(char *fileName)
{
    struct queue_node *temp = malloc(sizeof(struct queue_node));
    temp->next = 0;
    temp->fileName = malloc(strlen(fileName) + 1);
    strcpy(temp->fileName, fileName);

    return temp;
}
void inorder(struct node *root)
{

    if (root != NULL) // checking if the root is not null
    {
        inorder(root->leftChild);
        printf("word: %s  Freq %u  RealFreq %f\n", root->data, root->frequency, root->realFrequency);
        inorder(root->rightChild);
    }
}

void free_tree(struct node *root)
{
    if (root != NULL)
    {
        free_tree(root->rightChild);
        free(root->data); //if data was heap allocated, need to free it
        free_tree(root->leftChild);
        free(root);
    }
}
int free_anal(struct anal_output *root, int arraysize)
{
    struct anal_output *next;
    for (int i = 0; i < arraysize; i++)
    {
        next = root->next;
        free(root->fileOne);
        free(root->fileTwo);
        free(root);
        root = next;
    }
    return 0;
}
char *set_suffix(char *arg)
{
    char *value = strtok(arg, "-s");
    if (value == NULL)
    {
        return 0;
    }
    // printf("Value %s\n", value);
    return value;
}
struct node *new_node(char *word, int length)
{
    //sets the node
    struct node *p = malloc(sizeof(struct node));
    p->data = malloc(length);
    strcpy(p->data, word);
    p->frequency = 1.0;
    p->realFrequency = 0;
    p->leftChild = NULL;
    p->rightChild = NULL;

    return p;
}
int free_queue(struct queue_node *head)
{
    while (head != 0)
    {
        struct queue_node *temp = head->next;
        free(head->fileName);
        free(head);
        head = temp;
    }
    return 0;
}

struct node *Trees_new_node(char *word, unsigned length, float freq, int count)
{
    //sets the node
    struct node *p = malloc(sizeof(struct node));
    p->data = malloc(length + 1);
    strcpy(p->data, word);
    p->frequency = count;
    p->realFrequency = freq;
    p->leftChild = NULL;
    p->rightChild = NULL;

    return p;
}

struct node *Trees_insert(struct node *root, char *word, unsigned length, float freq, int count)
{
    //root is null means tree is empty new node is now root
    //printf("word: %s  count: %d  freq: %f  legnth: %u\n", word,count,freq,length);

    if (root == NULL)
    {
        //printf("word: %s  count: %d  freq: %f  legnth: %u\n", word,count,freq,length);
        return Trees_new_node(word, length, freq, count);
    }
    //compare the word if == increase the frequency
    else if (!strcmp(root->data, word))
    {
        root->frequency += count;
        root->realFrequency = (root->realFrequency + freq);
        return root;
    }
    else if (strcmp(root->data, word) < 0)
        root->rightChild = Trees_insert(root->rightChild, word, length, freq, count);
    else if (strcmp(root->data, word) > 0)
        root->leftChild = Trees_insert(root->leftChild, word, length, freq, count);

    return root;
}

struct node *copyTree(struct node *tree, struct node *root)
{

    if (tree != NULL) // checking if the root is not null
    {
        //printf("%d",tree->frequency);
        root = copyTree(tree->leftChild, root);
        root = Trees_insert(root, tree->data, strlen(tree->data), tree->realFrequency, tree->frequency);
        //printf("word: %s  Freq %u  RealFreq %f   Strlen %lu!!!!!!!!!!!!!!\n", tree->data, tree->frequency, tree->realFrequency,strlen(tree->data));
        root = copyTree(tree->rightChild, root);
    }

    return root;
}

float calculate_KLD(struct node *tree1, struct node *combinedTree)
{

    if (tree1 == NULL)
    {
        return 0;
    }
    return (tree1->realFrequency * log2(tree1->realFrequency / findInTree(combinedTree, tree1->data)) + calculate_KLD(tree1->leftChild, combinedTree) + calculate_KLD(tree1->rightChild, combinedTree));
}

float findInTree(struct node *root, char *word)
{

    if (root == NULL)
        return 0;
    if (!strcmp(root->data, word))
    {
        return (root->realFrequency / 2);
    }
    if (strcmp(root->data, word) > 0)
        return findInTree(root->leftChild, word);
    return findInTree(root->rightChild, word);
}

int totalWords(struct node *root, int count)
{

    if (root != NULL)
    {
        count = totalWords(root->leftChild, count);
        count += root->frequency;
        count = totalWords(root->rightChild, count);
    }
    return count;
}
void sortedInsert(struct anal_output **head, struct anal_output *newNode)
{
    struct anal_output dummy;
    struct anal_output *current = &dummy;
    dummy.next = *head;

    while (current->next != NULL && current->next->totalWords > newNode->totalWords)
    {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;
    *head = dummy.next;
}

// Given a list, change it to be in sorted order (using `sortedInsert()`).
void insertSort(struct anal_output **head, int arraysize)
{
    struct anal_output *result = NULL;   // build the answer here
    struct anal_output *current = *head; // iterate over the original list
    struct anal_output *next = NULL;
    if (current == 0)
    {
        return;
    }
    for (int i = 0; i < arraysize; i++)
    {
        // tricky: note the next pointer before we change it
        if (i != arraysize - 1)
            next = current->next;
        sortedInsert(&result, current);
        current = next;
    }
    //free(current);
    *head = result;
    finalList->head = *head;
}
