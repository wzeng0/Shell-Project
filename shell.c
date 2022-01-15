#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
/* reprints the user input */
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}
/* exits program when there is an error */
void errorHandle()
{
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}
/* gives size of array */
int aSize(char* str, char* excludeStr)
{
    char* subStr = strdup(str);
    char* subExcludeStr = strdup(excludeStr);
    char* cmd = NULL;
    int i = 0;
    cmd = strtok(subStr, subExcludeStr);
    while(cmd != NULL)
    {
        i++;
        cmd = strtok(NULL, subExcludeStr);
    }
    // printf("%d\n", i);
    return i + 1; // +1 for '\0'
}
/* forms an array of commands and separates based on given characters */
char** arrayOfCommands(char* str, char* excludeStr)
{
    int i = 0;
    char* cmd = NULL;
    char* sub = strdup(str);
    char* subExcludeChar = strdup(excludeStr);
    char** array = (char**)malloc(sizeof(char*) * (aSize(str, subExcludeChar) + 1));
    cmd = strtok(sub, subExcludeChar);
    while(cmd != NULL)
    {
        array[i] = cmd;
        i++;
        cmd = strtok(NULL, subExcludeChar);
    }
    array[i] = NULL;
    return array;
}
/* changing directories */
void cd(char** userInput, int size)
{
    char* directory;
    if (size > 3)
    {
        // myPrint("bad size ");
        errorHandle();
        return;
    }
    else if (size < 3)
        directory = getenv("HOME");
    else
        directory = userInput[1];
    if (chdir(directory))
    {
        // myPrint("bad directory ");
        errorHandle();
    }
}
int emptyLine(char* str)
{
    char* strCp = strdup(str);
    for (int i = 0; i < strlen(strCp); i++)
    {
        if ((strCp[i] == ' ') || (strCp[i] == '\t') || (strCp[i] == '\n'))
            continue;
        else
            return 0;
    }
    return 1;
}
/* checks if there is redirection in the input and returns int bool val */
int redirHelper(char* str)
{
    if (strstr(str, ">") == NULL)
        return 0;
    else
        return 1;
}
/* checks if there is advanced redirection in command and returns int bool val */
int advRedirHelper(char* str)
{
    if (strstr(str, ">+") == NULL)
        return 0;
    else
        return 1;
}
/* checks whether command includes cd, pwd, or exit and returns int bool val */
int excludeRedir(char* str)
{
    if((strstr(str, "cd") == NULL) && (strstr(str, "pwd") == NULL) && (strstr(str, "exit") == NULL))
        return 0; // if these commands are in the str then return true
    else
    {
        return 1;
    }
}

char* rearrange(char* str)
{
    int j = 0;
    if (strstr(str, ">+") != NULL)
    {
        char* new_str = (char*)malloc(sizeof(char) * (strlen(str) + 2));
        for (int i = 0; j < strlen(str); i++)
        {
            if ((str[j] == '>') && (i == j))
            {
                new_str[i] = ' ';
                new_str[i + 1] = '>';
            }
            else if((new_str[i - 1] == '+') && ((i - 1) == j))
            {
                new_str[i] = ' ';
            }
            else
            {
                new_str[i] = str[j];
                j++;
            }
        }
        return new_str;
    }
    else if (strstr(str, ">") != NULL)
    {
        char* new_str = (char*)malloc(sizeof(char) * (strlen(str) + 2));
        for (int i = 0; j < strlen(str); i++)
        {
            if ((str[j] == '>') && (i == j))
            {
                new_str[i] = ' ';
                new_str[i + 1] = '>';
            }
            else if((new_str[i - 1] == '>') && ((i - 1) == j))
            {
                new_str[i] = ' ';
            }
            else
            {
                new_str[i] = str[j];
                j++;
            }
        }
        return new_str;
    }
    else
        return str;
}

/* creates new redirection array without everything after ">" including ">" */
char** redirArray(char** userInput, int size)
{
    int i = 0;
    char** output = (char**)malloc(sizeof(char*) * (size - 2));
    while(strcmp(userInput[i], ">") != 0)
    {
        // printf("%s\n", output[i]);
        output[i] = userInput[i];
        i++;
    }
    output[i] = NULL;
    return output;
}
/* creates new redirection array without everything after ">+" including ">+" */
char** advRedirArray(char** userInput, int size)
{
    int i = 0;
    char** output = (char**)malloc(sizeof(char*) * (size - 2));
    while(strcmp(userInput[i], ">+") != 0)
    {
        // printf("%s\n", output[i]);
        output[i] = userInput[i];
        i++;
    }
    output[i] = NULL;
    return output;
}
/* Appends components of the first file into the second file*/
void fappend(char* source, char* output)
{
    /* open temp file with o_append
     * open redirection file read flag
     * for/while loop into buffer
     */
    int output_fd = open(output, O_RDWR | O_APPEND , S_IRUSR | S_IWUSR); // uses user input of file name to create file
    int source_fd = open(source, O_RDONLY , S_IRUSR | S_IWUSR);
    char buff[1000];
    int file_size = read(source_fd, buff, 1500);
    write(output_fd, buff, file_size);
}

/* main redirection function 
 * userInput: the input array
 * size: size of input array
 * advanced: bool that states whether the redirection is advanced or not
 */
void redir(char** userInput, int size, int advanced)
{
    int status;
    if (fork() == 0)
    {
        if (access(userInput[size - 2], F_OK) == -1) // check if file exists. If it does not then create file
        {
            int output_fd = open(userInput[size - 2], O_RDWR | O_CREAT, S_IRWXU); // uses user input of file name to create file
            dup2(output_fd, STDOUT_FILENO); // redirects output to the file
            execvp(userInput[0], redirArray(userInput, size)); // calls for function to execute
            close(output_fd); // closes file after using
            exit(0);
        }
        else
        {
             if (advanced == 1) // checks if there it is advanced redirection while file exists
            {
                int output_fd = open("output.temp", O_RDWR | O_CREAT, S_IRWXU); // creates temp file
                dup2(output_fd, STDOUT_FILENO); // redirects output to temp file
                if (execvp(userInput[0], advRedirArray(userInput, size)) == -1) // check if there is an output on execvp
                {
                    // myPrint("before exit");
                    exit(0);
                }
                close(output_fd); // close file after using
            }
            else
            {
                // printf("error2\n");
                errorHandle();
            }
        }
    }
    else
    {
        waitpid(-1, &status, 0); // waits for child processes to be done
        if (advanced == 1)
        {
            fappend(userInput[size - 2], "output.temp"); // function to append source file to output
            remove(userInput[size - 2]);
            rename("output.temp", userInput[size - 2]); // renames function to what the user wants
        }   
    }
}
/* execute command given user input */
void executeCommand(char* userInput)
{
    char *userInputCY = strdup(userInput);
    // char *redirCY = strdup(userInput);
    char **argv = arrayOfCommands(userInputCY, " \t");
    int status; // for waitpid() purposes
    // int numArg = aSize(userInputCY, " \t");
    if(strcmp(argv[0], "pwd") == 0)
    {
        char buf[256];
        if (getcwd(buf, sizeof(buf)) == NULL) // gets current working directory and puts it in buf
            errorHandle(); // if cwd shows nothing then there is something wrong
        else
        {
            if (argv[1] != NULL)
            {
                // printf("error1\n");
                errorHandle();
            }
            else
            {
                write(STDOUT_FILENO, buf, strlen(buf)); // returns cwd to command line
                myPrint("\n");
            }
        }
    }
    else if (strcmp(argv[0], "cd") == 0)
    {
        cd(argv, aSize(userInputCY, " \t")); // cds into given directory
    }
    else if (strcmp(argv[0], "exit") == 0)
    {
        if (argv[1] != NULL)
        {
            // printf("error1\n");
            errorHandle();
        }
        else
            exit(0); // exits program when called
    }
    else 
    {
        // printf("hello1\n");
        if (fork() == 0)
        {
            if (execvp(argv[0], argv) == -1)
            {
                // printf("error1\n");
                errorHandle();
                exit(0);
            }
        }
        else
            waitpid(-1, &status, 0);
    }
}
/* main function */
int main(int argc, char *argv[]) 
{
    char cmd_buff[514];
    char *pinput;
    char *sub;
    FILE *file;
    int batch = 0;
    // check if there is batch mode
    if(argc == 1)
        file = stdin;
    else if (argc == 2) {
        // makes batch true to run
        batch = 1;
        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            errorHandle();
            exit(0);
        }
    }
    else
    {
        errorHandle();
        exit(0);
    }
    if (batch == 1) {
        while (batch == 1) {
        // while (1) {
            pinput = fgets(cmd_buff, sizeof(cmd_buff), file);
            // pinput = fgets(cmd_buff, sizeof(cmd_buff), stdin);
            if (!pinput) {
                exit(0);
            }
            // if (strlen(cmd_buff) >= 513)
            //     errorHandle();
            if (strlen(pinput) >= 513 && pinput[strlen(pinput) -1] != '\n') 
            {
                myPrint(pinput);
                while(1)
                {
                    pinput = fgets(cmd_buff, sizeof(cmd_buff), file);
                    if (pinput == NULL)
                        break;
                    myPrint(pinput);
                    if (pinput[strlen(pinput) - 1] == '\n')
                        break;
                }
                errorHandle();
                continue;
            }
            if (!emptyLine(pinput))
                myPrint(cmd_buff); // for debugging uses (reprints user input)
            sub = strdup(pinput);
            // makes array without ; and \n
            char **pinputArray = arrayOfCommands(sub, ";\n");
            
            // goes through array of commands
            for(int i = 0; pinputArray[i] != NULL; i++) {
                if (strlen(pinputArray[i]) >= 513)
                    errorHandle();
                int arraySize = aSize(pinputArray[i], " \t");
                // if redirection is true (> if present in command) and none of the built-in is in the same function then execute redirection
                if (emptyLine(pinputArray[i]))
                    continue;
                else if (redirHelper(pinputArray[i]) && !(excludeRedir(pinputArray[i])))
                {
                    // redir(arrayOfCommands(pinputArray[i], " \t"), arraySize, advRedirHelper(pinputArray[i]));
                    redir(arrayOfCommands(rearrange(pinputArray[i]), " \t"), arraySize, advRedirHelper(rearrange(pinputArray[i])));
                }
                else
                {
                    if (redirHelper(pinputArray[i]))
                        errorHandle();
                    else
                        executeCommand(pinputArray[i]); // just execute normally if no redirection
                }
            }
        }
    }
    else
    {
        while (batch != 1) {
        // while (1) {
            pinput = fgets(cmd_buff, sizeof(cmd_buff), stdin);
            // pinput = fgets(cmd_buff, sizeof(cmd_buff), stdin);
            if ((!pinput) && (strlen(pinput) >= 513)) {
                exit(0);
            }
            if (!emptyLine(pinput))
                myPrint(cmd_buff); // for debugging uses (reprints user input)
            sub = strdup(pinput);
            // makes array without ; and \n
            char **pinputArray = arrayOfCommands(sub, ";\n");
            
            // goes through array of commands
            for(int i = 0; pinputArray[i] != NULL; i++) {
                int arraySize = aSize(pinputArray[i], " \t");
                // if redirection is true (> if present in command) and none of the built-in is in the same function then execute redirection
                if (emptyLine(pinputArray[i]))
                    continue;
                else if (redirHelper(pinputArray[i]) && !(excludeRedir(pinputArray[i])))
                {
                    redir(arrayOfCommands(rearrange(pinputArray[i]), " \t"), arraySize, advRedirHelper(rearrange(pinputArray[i])));
                }
                else
                {
                    if (redirHelper(pinputArray[i]))
                        errorHandle();
                    else
                        executeCommand(pinputArray[i]); // just execute normally if no redirection
                }
            }
        }
    }
}