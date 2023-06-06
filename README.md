Noam Mayerfeld
My code creates the relevant number of threads, and passes each a function with a void* pointer to a struct containing variables with the number thread that it is, the total number of threads, and the relevant information to make its request to the server (with the path alternating, thread by thread, if two files are typed into the command line).
My class contains two global variables each of which is a pointer to a semaphore. These are each subsequently set equal to an array of semaphores – one semaphore per thread.  One of these arrays is used to manage the critical section, or the section of work that must be synchronized.  The other array is used to manage the total work, to ensure that Thread one doesn’t start again before all of the threads have finished their total work. The critical section array has each semaphore initialized to 0, and the total work array has each semaphore initialized to 1.
Each thread except T1 begins by “downing” the critical section semaphore of the previous thread.  Thus, it will sleep until the previous thread “ups” its critical section semaphore, indicating that it has finished the work that must be synchronized.  The thread then does the work that must be synchronized, finishing by “upping” its critical section semaphore, allowing the next thread to begin.  It then completes all of its work, and then “ups” its total work semaphore, indicating that it has finished all of its work.
T1 begins by “downing” each semaphore in the total work array (thus it will sleep unless each thread has finished all its work and “upped” its total work semaphore (but it will run the first time as they are initialized to 1).  It then does its work that must be synchronized, calls “up” on its critical section semaphore, allowing T2 to begin, finishes its work and then “ups” its own total work semaphore.
I tested by using print line statements.  After a thread began its critical work after “downing” the semaphore of the previous thread I had it print “Thread___ beginning critical section.”  Upon completion of the work, but before “upping its semaphore,” I had it print “Thread __ concluding critical section.”  Then, after it finished printing, but before “upping” its total work semaphore, I had it print “Thread ___ completed total work,” so I could ensure that they all finished before Thread 1 started again.  I ran the program and made sure the print lines were all in the right order.
