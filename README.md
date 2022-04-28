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
The goal of this project is to extend the first part of the Word Wrap project to support recursive directory traversal along with multi-threading to read files and wrtie file. ./ww is a program that reformats a text file to fit in a certain number of columns (the page width) using only POSIX I/O functions(open(), read(), and write()). It will first read the given text file and either print the reformatted version onto standard output or save the reformatted version to a new text file with the "wrap." prefix. In this part of the project, there will be different queues (directory and file) in order for multiple threads to be working at the same time.

## Part I: Word-Wrap ##

Word wrap works by doing a loop, character by character, of the input file, until the input is expended. At each iteration the character is either whitespace (space or newline) or a character. If it is a character we add it to a word buffer, but if it is whitespace we have more logic to execute. When encountering white space, we drain the word buffer if applicable and then handle cases like checking if two or more newlines in a row (new paragraph), multiple spaces in a row, etc. 

### Testing Strategy for Word-Wrap ###
Word wrap was first tested manually with the example given and then with other manual string to ensure that spaces and newlines were handled correctly, and that the line length was correctly enforced. Then, we wrote a script that took in a sample text (lorem ipsum for example) and put a random number of spaces and possibly a newline between each word. When these random scripts were put into a directory and all wrapped at once they should be byte-identical at the end, and they were.

## Part II: Queue for Directory and File ##
For this implementation of queue, there are two elements to allow multithreading: linked list nodes and and a queue struct. In each LL node, it stores the directory path as a char* and the next element in the LL. In each queue node, it stores the head of the LL, the rear of the LL, an integer to represent the number of open files, an integer to see how many nodes are in the queue, a mutex lock, and 2 conditional variables for enqueue and dequeue.

In order to use the queue, there are three methods: initalize, enqueue, and dequeue. In initialize, it sets the head and rear to NULL, open and count to 0, and initializes all mutex related variables. In enqueue, it inserts the node to the end of the LL, and increments count to be used by other threads. In dequeue, it waits until a node is avaiable and dequeues that node when ready. However, if there are no elements in the queue and there are no potential paths to the added, it will return a NULL pointer. In this implementation, before calling enqueue or dequeue, the mutex must be locked; this approach was accepted as it allowed the thread that requested a node first will get it first to prevent starvation during multithreading.

## Testing Strategy for Queue ##
As a simple test before stepping into the workers, simple tests were conducted to see if it was operational.

1. Does it return NULL when nothing is in the queue?
- This will test the base case for dequeue as it is essential for multiple threads to stop
2. Does it appropriatley queue the node or is it randomized due to multithreading?
- This will test if the queue is actually a queue or just a LL
3. What happens if multiple threads try to call dequeue or enqueue?
- This will test if the queue dequeues the proper element when multiple threads are fighting for the mutex lock.

In order to determine if the queue is correct, we have a print queue method that locks at the moment and reads to entire queue from the start of the queue.





## Part II: Word-Wrap with Directory ##

For part II of this project, it is divided into 2 parts: the first is dedicated to file management and the second is dedicated to calling the wrap function with the correct file discriptor. Given the various argc value, it calls the wrap function by giving it the appropriate fd values. For example, if it reads from standard output, the fd for reading would be 1 while standard in would be 0. If reading from a file or writing to a file, we can open directory and determine if it is a valid fd (not -1). When given a directory with mulitple files, chdir() was used to locally create the file within the folder. After all this, it will close all the necessary files and return the if it has failed during the wrap process. 

### Testing Strategy for Part II ###
In order to test the different aspects of part II, the DEBUG macro was used to quickly determine which debug information to print. Firstly, a testing directory was created which included various test files and a directory. We determined that this procedure was correct as we could print out what each file was categorized under by testing the different directories and evaluating the results. Secondly, for creating the new file name with the "wrap." prefix, it was similarly tested by printing out the new name in comparison to the given file name.

## Makefile ##
For the Makefile, Wall, fsanitize=address,undefined , and std=c99 flags were set.

