# WordWrap
by Scott Skibin (ss3793), Scott Burke ()

---------
Briefing:
---------

1) Interpret parameters passed to main()
    ->Print error message and exit with failure if the int value of argv[1] < 1–this needs to be a positive integer
    ->If args < 2, we read from stdin (fd == 0) and write to stdout (fd == 1)
    ->Else if argv[2] is a regular file, we read from argv[2] and write to stdout
    ->Else if argv[2] is a directory, we iterate through each file in the directory. If the file’s name does not start with a period (.) or the string “wrap.”, we read that file and write to wrap.filename. If wrap.filename already exists overwrite it.

2) Open source file, looping over multiple source files if appropriate

3) Read chars into a buffer

4) Loop over each char in the read buffer and parse the string of chars
    ->At each char, execute one or more actions: ignore the char, write the char to a word_holder array list, write from the word holder into the target file, write newline chars to the output file, update flag or tracker variables

5) Close the input and output files

6) Free any heap space that was allocated

----------
Test Plan:
----------

1) Create set of test files:
    ->Empty file
    ->File that contains only whitespace
    ->A few files that tests all normalization tasks: 
        ->whitespace sequence at the beginning of the file
        ->long whitespace sequences (including three or more newlines) in the middle and at the end of the file
        ->lines with more chars than col_width
        ->at least one word with more chars than col_width

2) Write the test program to verify that the output file conforms to the spec. Run this program for every file in the set of test files.
    ->It tests for the following errors in the output file:
        ->whitespace at beginning of file
        ->multiple consecutive space chars
        ->more than 2 consecutive newline chars
        ->line(s) wrapped too soon, based on col_width
        ->line(s) overran col_width
        ->line(s) contain whitespace chars other than space and newline
        ->(optional) input and output files do not contain the same non-whitespace chars

3) Make sure that calling ww with col_width on some file x, which was itself output by ww with col_width, creates a file that is byte-identical to x. Can use the shell commands at the bottom of p. 2 of the assignment to check this. Perform this test for every file in the set of test files.

4) Make sure ww works with different buffer lengths, including BUFSIZE == 1. Do this for a few choices of BUFSIZE (1, 3, and 8), for every file in the set of test files.

5) Make sure all error conditions are generating the expected program behavior (continue vs. immediately terminate), messages, and main() return values:
    ->Input file does not exist
    ->Input file type is something other than a regular file or directory
    ->Input file contains a word with more chars than col_width
    ->Input file cannot be opened because user read permission is turned off
        ->Tested this by removing read permission for dir_t1.txt in test_dir. ww reports an error message and successfully processes the other files in test_dir.
    ->Input file cannot be read because user read permission is turned off
    ->Output file cannot be written to because user write permission is turned off (probably do this at runtime as part of a DEBUG routine)
    ->Problem writing to output file
        ->n.b. we can get the return value of main() by entering echo $? in the shell

6) Use valgrind to make sure all memory allocated from the heap is freed
    ->Usage: valgrind --leak-check=yes myprog arg1 arg2

7) Make sure directory processing works as intended: 
    ->Any file that is not a regular file is bypassed within the working directory(includes: subdirectories)
        ->Create a directory called "test_dir" to contain regular files and subdirectories.
        ->Run ./ww on test_dir then check the directory to make sure the subdirectory was bypassed and only regular files within test_dir were proccessed.
    ->Any file that starts with a "." (EX: .txt) is bypassed within the working directory
        -> create a text file called ".txt" within test_dir
        -> Run ./ww on test_dir to make sure ".txt" is bypassed
    ->Output files are named ‘wrap.inputfilename’
        ->Any file that begins with "wrap." is bypassed as a new file to wrap.  If a regular file, "test.txt" ,is wrapped within a directory the output file will be "wrap.test.txt".  If ./ww is run on the same directory again "wrap.test.txt" will be overwritten with the new wrap column width.  No new "wrap." file will be created.
            ->DESIGN CHOICE: If a regular file is orginaly named "wrap.txt", it will not be bypassed and it's output file will be "wrap.wrap.txt".
            ->TEST: create file called "wrap.txt" in test_dir
            ->Run ./ww on test_dir 
            ->Make sure "wrap.txt" is not bypassed and output file, "wrap.wrap.txt" is created
        -> Run ./ww on test_dir
        -> Make sure all valid regular files remain unchanged and all wrapped versions of the regular files are processed correctly and prefixed with "wrap."
        -> To test if output "wrap." files are proccessed correctly run ./test_ww on all output and input files in test_dir

8) If no file name is present and only a column width is given ./ww will read from standard input and write to standard output as a default.
    ->run ./ww num_col [no input file/directory] 
        ->You should be able to type within the command line (standard input), and press [ENTER]. The wrapped text(according to num_col) should then be displayed in standard output.
    


