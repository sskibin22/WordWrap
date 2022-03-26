#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#define DEBUG 0
#define BUFSIZE 8
#define WORDSIZE_INIT 16

// word is a pointer to a resizable block of chars that builds the string of
// non-whitespace chars that make up a word
char *word;
int word_char_ct = 0, word_size = WORDSIZE_INIT;

void add_char(char c)
{
    // resize word if necessary
    if (word_char_ct == word_size) {
        word_size *= 2;
        word = realloc(word, word_size * sizeof(char));
    }
    word[word_char_ct] = c;
    word_char_ct++;
}

/* Inform user of write() errors and writing fewer than bytes than requested
 * Return -1 if either of these situations is inferred from the arguments,
 * 0 otherwise
 */
int inform_write_err(int requested, int written)
{    
    int ret_value = 0;
    if (written == -1) {
        perror("ERROR: file write error");
        ret_value = -1;
    }
    else if (written < requested) {
        fprintf(stderr, "ERROR: tried to write %d bytes, only wrote %d\n",
            requested, written);
        ret_value = -1;
    }
    return ret_value;
}

/*  write_word: write the chars referenced by word to the file with file descriptor fd_out.
 *  Prepend newlines or space to the word based on whether
 *  a) the word is long enough to need a new line given the chars already written
 *     to the current line
 *  b) two or more newline chars were encountered prior to the word
 *  return values:
 *  2: new paragraph started with no errors
 *  1: newline started with no errors
 *  0: newline not started, no errors
 * -1: newline started with word length > column width
 * -2: error occurred upon call to write() or fewer bytes were written than requested 
 */
int write_word(int fd_out, int col_width, int *line_char_ct, int newline_chars)
{
    char nl[2] = {'\n', '\n'};
    char sp = ' ';
    int newlines = 0;
    int bytes_written = 0;
    // if word has at least one char, write it to the output file
    if (word_char_ct > 0) {
        // determine how many newlines to prepend to the word
        // 2 or more newlines implies a new paragraph
        if (newline_chars > 1) {
            newlines = 2;
        }
        // test against col_width to determine if the word is long enough to need
        // a new line given the chars already written to the current line;
        // add 1 char to account for prepended space
        else if (*line_char_ct + 1 + word_char_ct > col_width) {
            newlines = 1;
        }
        // write newlines and reset *line_char_ct if indicated
        if (newlines > 0) {
            bytes_written = write(fd_out, nl, newlines);
            if (inform_write_err(newlines, bytes_written)) {
                word_char_ct = 0;                
                return -2;
            }
            *line_char_ct = 0;
        } 
        // write space and increment *line_char_ct if indicated
        if (*line_char_ct > 0) {
            bytes_written = write(fd_out, &sp, 1);
            if (inform_write_err(1, bytes_written)) {
                word_char_ct = 0;
                return -2;
            }
            (*line_char_ct)++;
        }
        // write word to output file, increment *line_char_ct
        bytes_written = write(fd_out, word, word_char_ct);
        if (inform_write_err(word_char_ct, bytes_written)) {  
            word_char_ct = 0;          
            return -2;
        }
        else {
            *line_char_ct += bytes_written;
            // get word as a string, for error messages
            char *word_str = malloc(sizeof(char) * (word_char_ct + 1));
            strncpy(word_str, word, word_char_ct);
            word_str[word_char_ct] = '\0';
            // return newlines unless word is longer than col_width
            int return_val = newlines;
            if (word_char_ct > col_width) {
                fprintf(stderr, "\nALERT: Input contains '%s' with width %d, "
                    "but column width is only %d\n", word_str, word_char_ct, col_width);
                return_val = -1;
            }
            word_char_ct = 0;
            free(word_str);
            return return_val;
        }
    }
    // if word has no chars, just return 0
    return 0;
}

/* process_content: read the contents of the input file to buf and write words
 * to the output file as whitespace and non-whitespace chars are encountered.
 * fd_in and fd_out are the file descriptors of the input and output files,
 * which are assumed to be open already.
 * Returns 1 if all file operations completed successfully. If write_word returns
 * an error value (i.e. an int < 0), process_content returns that value. If a
 * read error occurs, process_content returns -2.
 */
int process_content(int fd_in, int fd_out, int col_width) {
    char buf[BUFSIZE];
    int bytes_read = 0;
    int BOF = 1;
    // keep track of how many chars (including whitespace) have been written to a line so far
    int line_char_ct = 0;
    int newline_chars = 0;
    // need to remember how many newline chars precede the most recently completed word
    int prev_newline_chars = 0;
    int attempt_write = 0;
    int write_result = 0;
    int return_value = 1;

    while ((bytes_read = read(fd_in, buf, BUFSIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            // ignore any whitespace at the beginning of the input file
            if (isspace(buf[i]) && !BOF) {
                // write the contents of word to the output file as soon as the next
                // non-whitespace char is encountered; this ensures that all newline
                // chars are counted before writing the word, which in turn ensures
                // correct paragraph formatting
                attempt_write = 1;
                if (buf[i] == '\n') newline_chars++;
            }
            else if (!isspace(buf[i])) {
                BOF = 0;
                if (attempt_write) {
                    write_result = write_word(fd_out, col_width, &line_char_ct,
                        prev_newline_chars);
                    // stop process_content if write_word returns an error value of -2
                    if (write_result == -2) {
                        return -2;
                    }
                    // set return_value if write_word returns an error value of -1,
                    // but continue process_content
                    else if (write_result == -1) {
                        return_value = write_result;
                    }
                    attempt_write = 0;
                    prev_newline_chars = newline_chars;
                    newline_chars = 0;
                }
                add_char(buf[i]);
            }
        }
    }
    // inform of read errors
    if (bytes_read < 0) {
        perror("ERROR: file read error");
        return_value = -2;
    }
    // attempt one final write    
    if ((write_result = write_word(fd_out, col_width, &line_char_ct,
        prev_newline_chars)) < 0) {
        return_value = write_result;
    }
    // need to terminate output with a newline unless the input file had no words
    if (!BOF) {
        char nl = '\n';
        write(fd_out, &nl, 1);
    }    
    return return_value;
}

int main(int argc, char **argv) {
    int fd_in, fd_out;
    int col_width;
    int fail_check = EXIT_SUCCESS;
    struct stat argv_stat;
    word = malloc(sizeof(char) * WORDSIZE_INIT);
    // open input and output files
    if (argc < 2) {
        fprintf(stderr, "usage: ./ww col_width [filename | dirname]\n");
        fail_check = EXIT_FAILURE;
    }
    else {
        col_width = atoi(argv[1]);
        if (col_width < 1) {
            fprintf(stderr, "usage: ./ww col_width [filename | dirname]\n");
            fprintf(stderr, "col_width must be a positive integer\n");
            free(word);
            exit(EXIT_FAILURE);
        }
        // if no filename is provided, use stdin for input
        if (argc == 2) {
            fd_in = STDIN_FILENO;
            fd_out = STDOUT_FILENO;
            if(process_content(fd_in, fd_out, col_width) <0){
                fail_check = EXIT_FAILURE;
            }
            // close files as needed
            close(fd_in); 
            close(fd_out);
        }
        else {
            //make sure argv[2] is a valid file or directory
            if (stat(argv[2], &argv_stat)){
                fprintf(stderr, "ERROR: %s\n", strerror(errno));
                free(word);
                exit(EXIT_FAILURE);
            }
            //if argv[2] is a directory type loop through directory and process each file
            if(S_ISDIR(argv_stat.st_mode)){
                //declare local variables
                const char *prefix = "wrap.";
                char comp[6] = {'a', 'b', 'c', 'd', 'e'}; //set default comp array to ensure it does not equal "wrap."
                int n;
                char *file_name;
                DIR *dp;
                struct dirent *de;
                struct stat file_stat;
                dp = opendir(argv[2]);
                chdir(argv[2]);     //change directory to given argument directory

                //loop through directory
                while ((de = readdir(dp)) != NULL) {
                    //bypass current directory indicator
                    if (!strcmp(de->d_name, "."))
                        continue;
                    //bypass parent directory indicator
                    if (!strcmp(de->d_name, ".."))    
                        continue;
                    //bypass file that starts with a "."
                    if (de->d_name[0] == '.')    
                        continue;
                    //bypass a file that starts with "wrap." and is not named "wrap.txt"
                    if(strlen(de->d_name) > 4){
                        for (int c = 0; c < 5; c++){
                            comp[c] = de->d_name[c];
                        }
                    }
                    if (!strcmp(comp, prefix))
                        continue;
                    
                    //make sure stat returns no errors
                    if (stat(de->d_name, &file_stat)){
                        fprintf(stderr, "ERROR: stat(%s): %s\n", de->d_name, strerror(errno));
                        fail_check = EXIT_FAILURE;
                        continue;
                    }
                    //if current file is a regular file
                    if(S_ISREG(file_stat.st_mode)){
                        //open read in file as current file/ check for errors
                        if ((fd_in = open(de->d_name, O_RDONLY)) < 0) {
                            perror("ERROR: file open error");
                            fail_check = EXIT_FAILURE;
                            continue;
                        }
                        //get total number of characters for wrapped file name
                        n = strlen(de->d_name) + strlen(prefix) + 1;
                        //initialize arrayList to file_name pointer
                        file_name = (char *)malloc(n * sizeof(char));
                        //copy prefix to file_name then append current file name to file_name
                        strcpy(file_name, prefix );             
                        strcat(file_name, de->d_name);
                        //create new file as "wrap.filename" with all user permissions
                        //if file exists, overwrite it
                        if((fd_out = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU)) < 0){
                            perror("ERROR: file open error");
                            close(fd_in);
                            free(file_name);
                            fail_check = EXIT_FAILURE;
                            continue;
                        }
                        //process input file and output wrapped text to "wrap." file
                        if(process_content(fd_in, fd_out, col_width) < 0){
                            close(fd_in);
                            close(fd_out);
                            free(file_name);
                            fail_check = EXIT_FAILURE;                     
                            continue;
                        }
                        //close open files
                        close(fd_in);
                        close(fd_out);
                        //free memory allocated by malloc
                        free(file_name);
                    }
                }
                //close directory
                closedir(dp);
            }
            //argv[2] is a regular file type
            else if(S_ISREG(argv_stat.st_mode)){
                if ((fd_in = open(argv[2], O_RDONLY)) < 0) {
                    perror("ERROR: file open error");
                    free(word);
                    exit(EXIT_FAILURE);
                }
                fd_out = STDOUT_FILENO;
                if(process_content(fd_in, fd_out, col_width) < 0){
                    close(fd_in); 
                    close(fd_out);
                    free(word);
                    exit(EXIT_FAILURE);
                }
                close(fd_in); 
                close(fd_out);
            }
            //if arg[2] is anything other then a directory or regular file print error and exit program
            else{
                fprintf(stderr, "ERROR: argv[2] is not a valid file or directory\n");
                fail_check = EXIT_FAILURE;
            }
        }
    }
    // free memory
    free(word);
    return fail_check;
}