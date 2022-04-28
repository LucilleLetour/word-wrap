# word-wrap
Project 2 of System Programming (CS214): Luke Letourneau(lml252) and Joshua Chung(jyc70)-Group 40

## Goals ##
Implement a program that reformats a text file to fit in a certain number of columns (the page width) using only POSIX I/O functions(open(), read(), and write()). It will first read the given text file and either print the reformatted version onto standard output or save the reformatted version to a new text file with the "wrap." prefix.

## Part I: Word-Wrap ##

Word wrap works by doing a loop, character by character, of the input file, until the input is expended. At each iteration the character is either whitespace (space or newline) or a character. If it is a character we add it to a word buffer, but if it is whitespace we have more logic to execute. When encountering white space, we drain the word buffer if applicable and then handle cases like checking if two or more newlines in a row (new paragraph), multiple spaces in a row, etc. 

### Testing Strategy ###
Word wrap was first tested manually with the example given and then with other manual string to ensure that spaces and newlines were handled correctly, and that the line length was correctly enforced. Then, we wrote a script that took in a sample text (lorem ipsum for example) and put a random number of spaces and possibly a newline between each word. When these random scripts were put into a directory and all wrapped at once they should be byte-identical at the end, and they were.

## Part II: Word-Wrap with Directory ##

For part II of this project, it is divided into 2 parts: the first is dedicated to file management and the second is dedicated to calling the wrap function with the correct file discriptor. Given the various argc value, it calls the wrap function by giving it the appropriate fd values. For example, if it reads from standard output, the fd for reading would be 1 while standard in would be 0. If reading from a file or writing to a file, we can open directory and determine if it is a valid fd (not -1). When given a directory with mulitple files, chdir() was used to locally create the file within the folder. After all this, it will close all the necessary files and return the if it has failed during the wrap process. 

### Testing Strategy for Part II ###
In order to test the different aspects of part II, the DEBUG macro was used to quickly determine which debug information to print. Firstly, a testing directory was created which included various test files and a directory. We determined that this procedure was correct as we could print out what each file was categorized under by testing the different directories and evaluating the results. Secondly, for creating the new file name with the "wrap." prefix, it was similarly tested by printing out the new name in comparison to the given file name.

## Makefile ##
For the Makefile, Wall, fsanitize=address,undefined , and std=c99 flags were set.





# word-wrap part 2
Project 3 of System Programming (CS214): Luke Letourneau(lml252) and Joshua Chung(jyc70)-Group _____________________________

## Goals ##
The goal of this project is to extend the first part of the Word Wrap project to support recursive directory traversal along with multi-threading to read files and write files. ./ww is a program that reformats a text file to fit in a certain number of columns (the page width) using only POSIX I/O functions(open(), read(), and write()). It will first read the given text file and either print the reformatted version onto standard output or save the reformatted version to a new text file with the "wrap." prefix. In this part of the project, there will be different queues (directory and file) in order for multiple threads to be working at the same time.

## Part I: Word-Wrap ##

Word wrap works by doing a loop, character by character, of the input file until the input is expended. At each iteration, the character is either whitespace (space or newline) or a character. If it is a character we add it to a word buffer, but if it is whitespace we have more logic to execute. When encountering white space, we drain the word buffer if applicable and then handle cases like checking if two or more newlines are in a row (new paragraph), multiple spaces in a row, etc. 

### Testing Strategy for Word-Wrap ###
Word wrap was first tested manually with the example given and then with other manual strings to ensure that spaces and newlines were handled correctly and that the line length was correctly enforced. Then, we wrote a script that took in a sample text (lorem ipsum for example) and put a random number of spaces and possibly a newline between each word. When these random scripts were put into a directory and all wrapped at once they should be byte-identical at the end, and they were.

## Part II: Queue for Directory and File ##
For this implementation of queue, there are two elements to allow multithreading: linked list nodes and a queue struct. In each LL node, it stores the directory path as a char* and the next element in the LL. In each queue node, it stores the head of the LL, the rear of the LL, an integer to represent the number of open files, an integer to see how many nodes are in the queue, a mutex lock, and 2 conditional variables for enqueue and dequeue.

In order to use the queue, there are three methods: initialize, enqueue, and dequeue. In initialize, it sets the head and rear to NULL, opens and counts to 0, and initializes all mutex-related variables. In enqueue, it inserts the node to the end of the LL, and increments the count to be used by other threads. In dequeue, it waits until a node is available and dequeues that node when ready. However, if there are no elements in the queue and there are no potential paths to the added, it will return a NULL pointer. In this implementation, before calling enqueue or dequeue, the mutex must be locked; this approach was accepted as it allowed the thread that requested a node first will get it first to prevent starvation during multithreading.

## Testing Strategy for Queue ##
As a simple test before stepping into the workers, simple tests were conducted to see if it was operational.

1. Does it return NULL when nothing is in the queue?
- This will test the base case for dequeue as it is essential for multiple threads to stop
2. Does it appropriately queue the node or is it randomized due to multithreading?
- This will test if the queue is actually a queue or just a LL
3. What happens if multiple threads try to call dequeue or enqueue?
- This will test if the queue dequeues the proper element when multiple threads are fighting for the mutex lock.

In order to determine if the queue is correct, we have a print queue method that locks at the moment and reads to the entire queue from the start of the queue.

## Project layout ##
There are two queues: One for storing directories and one for storing files. Each worker will have access to both of the queues to determine when the thread should stop running. There is one master thread in which the program will start and it will start M directory worker threads and N file worker threads. Thus at most 1 + M + N threads will start.

## Part III: Directory Worker ##
For each directory worker that we start, it will continue as long as there is a node in the directory or there is a potential directory that can be added to the queue. As a base case, if this condition is no longer true, it will signal dequeue_ready, unlock mutex locks, and end the thread. Else, it will wait until there is a dequeue-ready signal. After receiving a signal, it will once again check if it can be dequeued. Else it will exit as normal. If there was a successful dequeue, it will open the directory and go through each item. If the item is a file, it will be enqueued into the file queue. It will also check for the "wrap." case and "."/".." case as well. If the item is a directory, it will be enqueued to the directory queue. After that, it will continue as normal and start again.

## Testing Strategy for Directory Worker ##
To test the directory workers, multiple folders were created with multiple subdirectories that included empty files, empty folders, and folders with items to determine if everything was enqueued correctly.

1. Does it enqueue all the files into the file queue on a single thread?
- This will test the base case of if a single thread will continue and end at the base case.
2. Does it enqueue all the files into the file queue on multiple threads? (around 2 to 5)
- This will test if the files are enqueued properly and all threads end when there is nothing to be dequeued or waited for.
3. Does it enqueue all the files into the file queue on a lot of threads? (around 10 to 100)
- This will test if the files are enqueued properly and all threads end when they are met with the base case when they start.

## Part IV: File Worker ##

## Testing Strategy for File Worker ##

## Part V: System Call Parsing and Extra Credit ##
In order for our program to work with multiple variations of system call for the extra credit, multiple variations of the calls were accounted for:
1. Check for Valid argc
- If there are less than 3 variables in the call, it is an invalid call, and therefore it is Errored out.
2. Existence of -r in the call
- If there is a -r in the call, it goes through the arguments and enqueue the directories paths into the directory queue and enqueue the file paths into the file queue before multithreading
- If there is not a -r in the call, it will go through the arguments and enqueue the files in the immediate subdirectories into the file queue.
3. Add to directory queue and file queue when necessary
4. Run the correct M and N for correct multi-threading
- If there was a -r in the call, it will parse -rM,N, and -rN and call it accordingly. Else, it will call -r1,1 as it is equivalent.
- If only a single file is given, it will print to standard output.

After parsing, M directory workers will start and N file workers will start. If not present, it will default to -r1,1. Each thread started will have access to the directory queue, file queue, and the id which is used for debugging.

## Testing Strategy for System Call Parsing and EC ##
To check if the system call was working, after parsing and before starting the multiple threads, each queue was printed and the M and N values were also printed. 
1. Did it enqueue the proper directories and files?
- This will make sure that the starting queues are correct and that no files are missed out accidentally
2. Did it parse -rM,N properly?
- This will make sure that the proper number of worker threads starts.
3. Is it printing to standard output when needed?
- This will make sure that the wrapped text is not being written to a file

## Part VI: Testing Strategy ##
For the final testing strategy, all the testing strategies previously noted were combined along with the DEBUG macro to print out necessary information when needed.
1. Calling the program when called with -r1,1?
- This makes sure that the program can reach its base case in the worst-case scenario, confirming its robustness and capability with one thread each.
2. Calling the program when called with -rc,1 where c is an appropriate thread count between 2 to 10?
- This makes sure that multiple directory workers start and end properly, confirming that it is enqueuing as it should and stopping when it is empty.
3. Calling the program when called with -r1,c where c is an appropriate thread count between 2 to 10?
- This makes sure that multiple file workers start and end properly, confirming that it is enqueuing as it should and stopping when it is empty.
4. Calling the program when called with -rc,d where c and d are appropriate thread counts between 2 to 10?
- This makes sure that multiple directory and file worker threads start and end properly for the given directories and files in the queue.
5. Calling the program when called with -rc,d where c and d are extreme thread counts between 1000 to 5000?
- This makes sure that the threads do indeed end if there is nothing to do.

After calling each call, each directory was carefully checked to see the proper wrapped file with the proper prefix with the line_lengths.

## Makefile ##
For the Makefile, Wall, fsanitize=address, undefined, pthread, and std=c99 flags were set.

In addition, there is a make cleant option to delete any files with wrap. prefix to avoid manual labor after each trial.
