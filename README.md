# WordWrap
by Scott Skibin (ss3793), Scott Burke (sgburke)

----------
Execution:
----------

1) Interpret arguments passed to the main() function of ww.c
    ->argv[1] is the column width to wrap to. If the int value of argv[1] < 1, print an error message and exit with failure status.
    ->If args < 2, we read from stdin (fd == 0) and write to stdout (fd == 1)
    ->Else if argv[2] is a regular file, we read from argv[2] and write to stdout
    ->Else if argv[2] is a directory, we iterate through each file in the directory. If the file’s name does not start with a period (.) or the string “wrap.”, we read that file and write to wrap.filename. If wrap.filename already exists overwrite it.

2) Open input file, looping over multiple input files if a directory name was provided

3) Read chars of input file into a read buffer

4) Loop over each char in the read buffer and parse the sequence of chars
    ->At each char, execute one or more actions: ignore the char, write the char to a word_holder array list, write from the word_holder into the output file, write newline chars to the output file, update various flag or tracker variables

5) Close the input and output files

6) Free any heap space that was allocated

----------
Test Plan:
----------

1) Create a set of test files. For each file, run ww with various column widths (e.g. 20, 30):
    ->Degenerate cases
        ->Empty file
        ->File that contains only whitespace
    ->Files that test all normalization tasks: 
        ->base case modeled on the example at the bottom of p. 3 of the assignment
        ->file with non-whitespace and space chars (' ') but no newlines
        ->whitespace sequence at the beginning of the file
        ->long whitespace sequences (including three or more newlines) in the middle and at the end of the file
        ->lines with more chars than col_width
        ->at least one word with more chars than col_width
        ->file consisting of just one non-whitespace char

2) Write a test program to verify that the output files conform to the spec. Run this program for every file in the set of test files.
The test program checks for the following errors in the output file:
    ->whitespace at beginning of file
    ->multiple consecutive space chars
    ->more than 2 consecutive newline chars
    ->any whitespace chars other than space and newline
    ->line(s) wrapped too soon, based on specified col_width
    ->line(s) overran specified col_width 
    ->(optional) input and output files do not contain the same non-whitespace chars in identical order

3) Run files in the set of test files with valgrind to make sure all memory allocated from the heap is freed.

4) Make sure that calling ww with col_width on some file x, which was itself output by ww with col_width, outputs a file that is byte-identical to x. Use shell commands like those at the bottom of p. 2 of the assignment to verify. Perform this test for files in the set of test files.

5) Make sure ww works with different read buffer lengths, including BUFSIZE == 1. Do this for a few choices of BUFSIZE (1, 3, and 8), for every file in the set of test files.

6) Make sure that error conditions are generating the expected program behavior (continue vs. immediately terminate), messages, and exit status. Test the following scenarios:
    ->Input file does not exist
    ->Input file type is something other than a regular file or directory
    ->Input file contains a word with more chars than col_width
    ->Error when opening the input file because user read permission is turned off
    ->Error when reading the input file because it was unexpectedly closed
    ->Error when writing to the output file because it was unexpectedly closed

7) Make sure directory processing works as intended: 
    ->Any file that is not a regular file is bypassed within the working directory (includes: subdirectories)
        ->Create a directory called "test_dir" to contain regular files and subdirectories.
        ->Run ./ww on test_dir then check the directory to make sure the subdirectory was bypassed and only regular files within test_dir were proccessed.
    ->Any file that starts with a "." (EX: .txt) is bypassed within the working directory
        ->Create a text file called ".txt" within test_dir
        ->Run ./ww on test_dir to make sure ".txt" is bypassed
    ->Output files are named ‘wrap.inputfilename’
        ->Any file that begins with "wrap." is bypassed as a new file to wrap.  If a regular file, "test.txt" ,is wrapped within a directory the output file will be "wrap.test.txt".  If ./ww is run on the same directory again "wrap.test.txt" will be overwritten with the new wrap column width.  No new "wrap." file will be created.
        ->Run ./ww on test_dir
        ->Make sure all valid regular files remain unchanged and all wrapped versions of the regular files are processed correctly and prefixed with "wrap."
        ->To test if output "wrap." files are proccessed correctly run ./test_ww on all output and input files in test_dir
        ->Run ./ww on test_dir multiple times with different column numbers and make sure all regular files that begin with "wrap." are overwritten and wrapped to new column number.

8) If no file name is present and only a column width is given ./ww will read from standard input and write to standard output as a default.
    ->run './ww col_width < in.txt > out.txt', inspect out.txt for conformity to the spec

---------------
Error Handling:
---------------

1) If an error occurs when opening an input or output file, or when reading an input file, ww generates an error message and aborts the processing of that input file. If a subdirectory is being processed, ww moves on to process the next input file in the subdirectory. ww will finish with status EXIT_FAILURE.

2) If a write error occurs, ww generates an error message and aborts the processing of the corresponding input file. "Write error" means that write() returns an error value (< 0) or reports that fewer bytes were written than requested. If a subdirectory is being processed, ww moves on to process the next input file in the subdirectory. ww will finish with status EXIT_FAILURE.

3) If an input file contains a word longer than the provided column width, ww generates an error message but continues processing the input file. ww will finish with status EXIT_FAILURE.
