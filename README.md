# word-wrap
Project 2 of System Programming (CS214): Luke Letourneau and Joshua Chung(jyc70)-Group 40

## Goals ##
Implement a program that reformats a text file to fit in a certain number of columns (the page width) using only POSIX I/O functions(open(), read(), and write()). It will first read the given text file and either print the reformatted version onto standard output or save the reformatted version to a new text file with the "wrap." prefix.

## Part I: Word-Wrap ##
TODO

## Part II: Word-Wrap with Directory ##

For part II of this project, it is divided into 2 parts: the first is dedicated to file management, and the second is dedicated to writing the new file. In `mainHelper()` given the directory and the column width, it will first check if the directory is a valid directory then check if it is a singular file or a folder. Else, it will return EXIT_FAILURE and print out the given error. If the given directory is a single file, it will call `wrap()` and print out the reformatted string. It will also free any pointers that were malloc-ed during the process. If the given directory is a directory/folder, it will open that directory and go through the files. It skips any subdirectories and non-regular files and calls `wrap()` on regular files for `writeWW()`. In `writeWW()`, it will take the string produced by `wrap()` and the new file name with a "wrap." prefix. It will utilize the BUFFER macro which will write by that macro until the end of the reformatted string.

### Testing Strategy for Part II ###

In order to test the different aspects of part II, the DEBUG macro was used to quickly determine which debug information to print. Firstly, a testing directory was created which included various test files and a directory. We determined that this procedure was correct as we could print out what each file was categorized under by testing the different directories and evaluating the results. Secondly, for creating the new file name with the "wrap." prefix, it was similarly tested by printing out the new name in comparison to the given file name. Finally, for testing `writeWW()`, a file was read using a similar read buffer and written using `writeWW()`, and then it was byte compared to make sure it had written down everything as given.

## Makefile ##
For the Makefile, Wall, fsanitize=address,undefined , and std=c99 flags were set.
