/* Generic */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
/* Network */
#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE 100
char buf[BUF_SIZE];
sem_t *criticalSection;
sem_t *totalWork;
struct args {
    int index;
    int numThreads;
    char *host;
    char *port;
    char *path;
};
// Get host information (used to establishConnection)
struct addrinfo *getHostInfo(char* host, char* port) {
    int r;
    struct addrinfo hints, *getaddrinfo_res;
    // Setup hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res))) {
        fprintf(stderr, "[getHostInfo:21:getaddrinfo] %s\n", gai_strerror(r));
        return NULL;
    }

    return getaddrinfo_res;
}

// Establish connection with host
int establishConnection(struct addrinfo *info) {
    if (info == NULL) return -1;

    int clientfd;
    for (;info != NULL; info = info->ai_next) {
        if ((clientfd = socket(info->ai_family,
                               info->ai_socktype,
                               info->ai_protocol)) < 0) {
            perror("[establishConnection:35:socket]");
            continue;
        }

        if (connect(clientfd, info->ai_addr, info->ai_addrlen) < 0) {
            close(clientfd);
            perror("[establishConnection:42:connect]");
            continue;
        }

        freeaddrinfo(info);
        return clientfd;
    }

    freeaddrinfo(info);
    return -1;
}

// Send GET request
void GET(int clientfd, char *host, char *port, char *path) {
    char req[1000] = {0};
    sprintf(req, "GET %s HTTP/1.1\r\nConnection: close\r\nHost: %s:%s\r\n\r\n", path,host,port);
    send(clientfd, req, strlen(req), 0);
}
int counter=0;
void *thread_function(void *input){
    while(1){
        int errorChecker;
        struct args *ar=(struct args *)input;
        int index=ar->index;
        int numThreads=ar->numThreads;
        char *port=ar->port;
        char *host=ar->host;
        char *path=ar->path;
        if(index==0){
            for(int i=0;i<numThreads;i++){ //if you are the first thread, call down on the total work semaphore of each other thread to make sure they have finished and "upped" their semaphore
                errorChecker= sem_wait(&totalWork[i]);
                if(errorChecker==-1){
                    fprintf(stderr,"error calling down on semaphore %d in totalWork",i);
                    return (void *) 1;
                }
            }
        }
        else{
            errorChecker=sem_wait(&criticalSection[index-1]); //if you are not T1, call down on the critical section semaphore of previous thread to make sure it is done with its work that must be synchronized
            if(errorChecker==-1){
                fprintf(stderr,"error calling down on semaphore %d in criticalSection before starting next critical section",index-1);
                return (void *) 1;
            }
        }
        //fprintf(stdout,"\n\nthread number %d beginning work \n\n\n",index);
        // Establish connection with <hostname>:<port>
        int clientfd = establishConnection(getHostInfo(host, port));
        if (clientfd == -1) {
            fprintf(stderr,
                    "[main:73] Failed to connect to: %s:%s%s \n",
                    host, port, path);
            return (void *) 3;
        }

        // Send GET request > stdout
        GET(clientfd, host, port, path);
        //fprintf(stdout,"\n\nthread number %d finished critical section \n\n",index);
        errorChecker= sem_post(&criticalSection[index]); //call up on my semaphore, so next thread can start in sync
        if(errorChecker==-1){
            fprintf(stderr,"error calling up on semaphore %d when finished critical section",index);
            return (void *) 1;
        }
        while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
            fputs(buf, stdout);
            memset(buf, 0, BUF_SIZE);
        }
        //fprintf(stdout,"\n\n\n\nthread number %d printing its stuff\n\n",index);
        close(clientfd);
        //fprintf(stdout,"\n\nthread number %d finished printing\n\n",index);
        errorChecker= sem_post(&totalWork[index]); //call up on my semaphore, so first thread knows I'm done
        if(errorChecker==-1){
            fprintf(stderr,"error calling up on semaphore %d when finished critical section",index);
            return (void *) 1;
        }
    }
}
int main(int argc, char **argv) {
    if (argc != 5&&argc!=6) {
        fprintf(stderr, "USAGE: ./httpclient <hostname> <port> <threads> <request path1> <request path2> \n");
        return 1;
    }
    int N= atoi(argv[3]);
    //fprintf(stderr,"\n\n\n\n VALUE OF N IS %d",N);
    int errorChecker;
    sem_t critical_section[N]; //array of semaphores to maintain synchronization for requests
    criticalSection=critical_section; //setting semaphore pointer global var to the array so threads can access it
    sem_t total_work[N]; //array of semaphores to maintain ordering- 1 will wait until all are finished
    totalWork=total_work;//setting semaphore pointer global var to the array so threads can access it
    for(int i=0;i<N;i++){ //initialize critical section semaphores to 0, with the last one at 1 (so the first thread can call down on the previous thread's sempahore)
        if(i==N-1){
            errorChecker=sem_init(&critical_section[i],0,1);
            if(errorChecker==-1){
                fprintf(stderr,"failed to initialize semaphore");
                return 1;
            }
        }
        else{
            errorChecker=sem_init(&critical_section[i],0,0);
            if(errorChecker==-1){
                fprintf(stderr,"failed to initialize semaphore");
                return 1;
            }
        }
        errorChecker=sem_init(&total_work[i],0,1); //initialize the total work semaphores to 1
        if(errorChecker==-1){
            fprintf(stderr,"failed to initialize semaphore");
            return 1;
        }
    }
    pthread_t thread_id[N];
    for(int i=0;i<N;i++){ //give each thread its variables and tell it which path to request- alternating if there were 2 requests typed into command line
        struct args *Arg = (struct args *)malloc(sizeof(struct args));
        Arg->index = i;
        Arg->numThreads=N;
        Arg->host=argv[1];
        Arg->port=argv[2];
        if(argc==5){
            Arg->path=argv[4];
        }
        else{
            if(i%2==0){
                Arg->path=argv[4];
            }
            else{
                Arg->path=argv[5];
            }
        }
        pthread_create(&thread_id[i],NULL,thread_function,(void *)Arg);
    }
    for(int j=0;j<N;j++){
        pthread_join(thread_id[j],NULL);
    }
    return 0;
}
