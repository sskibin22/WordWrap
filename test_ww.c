#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* test_ww
 *
 * arguments
 * 1. col_width to which output file was wrapped
 * 2. name of output file to be tested
 * 3. (optional) name of original input file to be compared to output file
 * 
 * prints a message if any of the following errors are detected in the output file,
 * finishes silently otherwise:
 * 1. whitespace at beginning of file
 * 2. multiple consecutive space chars
 * 3. more than 2 consecutive newline chars
 * 4. line(s) wrapped too soon, based on col_width
 * 5. line(s) overran col_width
 * 6. line(s) contain whitespace chars other than space and newline
 * 7. (optional) input and output files do not contain the same non-whitespace chars
 */

#define HOLDER_SIZE_INIT 500
#define BUFSIZE 16

struct char_holder {
    char *chars;
    int ct;
};

struct char_holder nonws_in, nonws_out;
int ch_size = HOLDER_SIZE_INIT;

void add_char(struct char_holder *ch, char c)
{
    // resize char_holders if necessary
    // have to resize both char-holders--not the greatest design
    if (ch->ct == ch_size) {
        ch_size *= 2;
        nonws_in.chars = realloc(nonws_in.chars, ch_size * sizeof(char));
        nonws_out.chars = realloc(nonws_out.chars, ch_size * sizeof(char));
    }
    ch->chars[ch->ct] = c;
    ch->ct++;
}

int read_non_ws(int fd) {
    char buf[BUFSIZE];
    int bytes_read;
    while ((bytes_read = read(fd, buf, BUFSIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (!isspace(buf[i])) {
                add_char(&nonws_in, buf[i]);
            }
        }
    }
    // make this a string by adding a null terminator
    if (bytes_read == 0) {
        add_char(&nonws_in, '\0');
    }
    else if (bytes_read < 0) {
        perror("Input file read error");
    }
    return bytes_read;
}

int process_output(int fd, int col_width) {
    char buf[BUFSIZE];
    int bytes_read;
    int BOF = 1;
    int line_num = 1;
    int word_char_ct = 0;
    int line_char_ct = 0, prev_line_char_ct = 0;
    int consec_newlines = 0;
    int consec_spaces = 0;
    int return_value = 0;

    while ((bytes_read = read(fd, buf, BUFSIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            // Correct output files cannot begin with whitespace
            if (BOF && isspace(buf[i])) {
                fprintf(stderr, "Output file error: file begins with whitespace\n");
                return_value = -1;
            }
            BOF = 0;
            if (!isspace(buf[i])) {
                add_char(&nonws_out, buf[i]);
                word_char_ct++;
                line_char_ct++;
                consec_newlines = 0;
                consec_spaces = 0;
            }
            else if (buf[i] == ' ') {
                if (++consec_spaces > 1) {
                    fprintf(stderr, "Output file error: line %d has multiple spaces\n", line_num);
                    return_value = -1;
                }
                if (prev_line_char_ct > 0 && 
                    prev_line_char_ct + 1 + word_char_ct <= col_width) {
                    fprintf(stderr, "Output file error: line %d wrapped too soon\n", line_num - 1);
                    return_value = -1;
                }
                word_char_ct = 0;
                line_char_ct++;
                prev_line_char_ct = 0;
                consec_newlines = 0;
            }
            else if (buf[i] == '\n') {
                if (++consec_newlines > 2) {
                    fprintf(stderr, "Output file error: more than 2 newlines at line %d\n", line_num);
                    return_value = -1;
                }
                if (line_char_ct > col_width) {
                    fprintf(stderr, "Output file error: line %d overran col_width\n", line_num);
                    return_value = -1;
                }
                word_char_ct = 0;
                prev_line_char_ct = line_char_ct;
                line_char_ct = 0;
                line_num++;
            }
            else {
                fprintf(stderr, "Output file error: line %d contains whitespace other than space and newline\n", line_num);
                    return_value = -1;
            }
        }
    }
    // need to null-terminate the char holder to create a string
    if (bytes_read == 0) {
        add_char(&nonws_out, '\0');
    }
    else if (bytes_read < 0) {
        perror("Output file read error");
        return_value = -1;
    }
    return return_value;
}

int main(int argc, char **argv) {
    int fd_in = -1, fd_out = -1;
    int col_width;
    int fail_check = EXIT_SUCCESS;
    nonws_in.chars = malloc(sizeof(char) * HOLDER_SIZE_INIT);
    nonws_in.ct = 0;
    nonws_out.chars = malloc(sizeof(char) * HOLDER_SIZE_INIT);
    nonws_out.ct = 0;

    if (argc < 3) {
        fprintf(stderr, "usage: ./test_ww col_width output_file [input_file]\n");
        fail_check = EXIT_FAILURE;
        goto cleanup;
    }
    col_width = atoi(argv[1]);
    if (col_width < 1) {
        fprintf(stderr, "usage: ./test_ww col_width output_file [input_file]\n");
        fprintf(stderr, "col_width must be a positive integer\n");
        fail_check = EXIT_FAILURE;
        goto cleanup;
    }
    // open files    
    if ((fd_out = open(argv[2], O_RDONLY)) < 0) {
        perror("Output file open error");
        fail_check = EXIT_FAILURE;
        goto cleanup;
    }
    if (argc > 3 && (fd_in = open(argv[3], O_RDONLY)) < 0) {
        perror("Input file open error");
        fail_check = EXIT_FAILURE;
        goto cleanup;
    }
    // read all non-whitespace chars from input file
    if (argc > 3 && read_non_ws(fd_in) < 0) {
        fail_check = EXIT_FAILURE;
    }
    // read all chars from output file
    if (process_output(fd_out, col_width) < 0) {
        fail_check = EXIT_FAILURE;
    }
    // test for identical non-whitespace chars
    if (argc > 3 && (nonws_in.ct != nonws_out.ct ||
        (nonws_in.ct == nonws_out.ct && strncmp(nonws_in.chars, nonws_out.chars, nonws_in.ct)))) {        
        fprintf(stderr, "Input and output files do not contain the same non-whitespace chars\n");
        fail_check = EXIT_FAILURE;
    }
    // close files
    cleanup:
    if (fd_in >= 0) close(fd_in);
    if (fd_out >= 0) close(fd_out);
    // free memory
    free(nonws_in.chars);
    free(nonws_out.chars);
    return fail_check;
}