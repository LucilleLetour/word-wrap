#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

// TODO: Too many newlines sometimes, lines getting cut too short (spaces?)

typedef enum bool { false = 0, true = 1 } bool;

int wrap(int fd, int line_length, char *out) {
	int pstage;
	char c[1];
	char word[1024];
	int wordlen = 0;
	int linelen = 0;
	int pos = 0;
	bool failed = false;
	while (read(fd, c, 1)) {
		if(c[0] != '\n') { // Check to see if the last character was \n
			
		}
		if (c[0] == ' ' || c[0] == '\n') { // Handle ws
			if (wordlen > 0) { // Add the word that's been being built first
				if (wordlen > line_length) { // The word is longer than the wrap length; report failure
					failed = true;
				}
				if (linelen + wordlen > line_length && out[pos - 1] != '\n') { // Word (when added) will exceed wrap length, so add newline
					if(out[pos - 1] == ' ') {
						out[pos - 1] = '\n'; // Convert ending space to newline
					} else {
						out[pos] = '\n';
						pos++;
					}
					linelen = 0;
				}
				for (int i = 0; i < wordlen; i++) { // Add in the word
					out[pos] = word[i];
					pos++;
					linelen++;
				}
				wordlen = 0;
			}

			if (c[0] == '\n') {
				if(pstage == 1) { // Previous character was also newline, must make new paragraph
					out[pos] = '\n';
					pos++;
					if(out[pos - 2] != '\n') {
						out[pos] = '\n';
						pos++;
					}
					linelen = 0;
					pstage = 2; // Directly proceding newlines are ignored
				} else if(pstage == 0) {
					pstage = 1;
				}
			} else if (c[0] == ' ') {
				if (pos != 0 && out[pos - 1] != ' ' && out[pos - 1] != '\n' && linelen <= line_length) { // Add space char if appropriate: no preceding whitespace and not overrunning line length
					out[pos] = ' ';
					pos++;
					linelen++;
				}
			}
		} else {
			if(pstage == 1) {
				if(c[0] != ' ' && out[pos - 1] != ' ' && out[pos - 1] != '\n') { // Add a space instead of the newline, if it's needed
			 		out[pos] = ' ';
					pos++;
					linelen++;
				}
			}
			pstage = 0;
			word[wordlen] = c[0];
			wordlen++;
		}
	}
	if (wordlen > 0) { // Repeat of above, add word into file in case EOF with word in buffer
		if (wordlen > line_length) {
			failed = true;
		}
		if (linelen + wordlen > line_length) {
			out[pos] = '\n';
			linelen = 0;
			pos++;
		}
		for (int i = 0; i < wordlen; i++) {
			out[pos] = word[i];
			pos++;
			linelen++;
		}
		wordlen = 0;
	}
	out[pos] = '\0';
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	int fd = open(argv[1], O_RDONLY);
	struct stat st;
	fstat(fd, &st);
	char* out = (char*)malloc((st.st_size * 2 + 1) * sizeof(char));
    wrap(fd, atoi(argv[2]), out);
	puts(out);
	free(out);
	close(fd);
}
