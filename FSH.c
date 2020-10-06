#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_LINE 128 // The maximum length of a command

// Declarations
char history[10][50]; // History array to store history commands
int count = 0;

// Function to display the history of commands
void displayHistory()
{
    printf("Shell command history:\n");

    int j = 0;
    int histCount = count;

    // Loop for iterating through commands
    for (int i = 0; i < 10; i++)
    {
        // Command index
        printf("%d. ", histCount);
        while (history[i][j] != '\n' && history[i][j] != '\0')
        {
            // Printing command
            printf("%c", history[i][j]);
            j++;
        }
        printf("\n");

        j = 0;
        histCount--;

        if (histCount == 0)
            break;
    }
    printf("\n");
}

// Function to get the command from shell, tokenize it and set the args parameter
int formatCommand(char inputBuffer[], char *args[], int *flag)
{
    int length; //  Number of chars in command line
    int i;      //  Loop index for inputBuffer
    int start;  //  Index of beginning of next command
    int ct = 0; //  Index of where to place the next parameter into args[]
    int hist;

    // Read user input on command line and checking whether the command is !! or !n
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    start = -1;
    if (length == 0)
        exit(0); // End of command
    if (length < 0)
    {
        printf("Command not read\n");
        exit(-1); // Terminate
    }

    // Examine each character
    for (i = 0; i < length; i++)
    {
        switch (inputBuffer[i])
        {
        case ' ':
        case '\t': // To separate arguments
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0'; // Add a null char at the end
            start = -1;
            break;

        case '\n': // final char
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; // No more args
            break;

        default:
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *flag = 1; // This flag is to differentiate whether the child process is invoked in background
                inputBuffer[i] = '\0';
            }
        }
    }

    args[ct] = NULL; // If the input line was > MAX_LINE

    // Implementing the exit feature
    if (strcmp(args[0], "exit") == 0)
        exit(0);

    // Implementing the cd .. feature
    if (strcmp(args[0], "cd") == 0)
    {
        if (chdir(args[1]) != 0)
            printf("chdir() to %s failed\n", args[1]);

        return -1;
    }

    // Implementing the history feature
    if (strcmp(args[0], "history") == 0)
    {
        if (count > 0)
            displayHistory();
        else
            printf("\nNo Commands in the history\n");

        return -1;
    }
    else if (args[0][0] - '!' == 0)
    {
        int x = args[0][1] - '0';
        int z = args[0][2] - '0';

        if (x > count) // Second letter check
        {
            printf("\nNo Such Command in the history\n");
            strcpy(inputBuffer, "Wrong command");
        }
        else if (z != -48) // Third letter check
        {
            printf("\nNo Such Command in history. Enter <= !9 (buffer size is 10 along with current command)\n");
            strcpy(inputBuffer, "Wrong command");
        }
        else
        {
            if (x == -15)                        // Checking for '!!',ascii value of '!' is 33.
                strcpy(inputBuffer, history[0]); // This will be your 10 th(last) command
            else if (x == 0)                     // Checking for '!0'
            {
                printf("Enter proper command");
                strcpy(inputBuffer, "Wrong command");
            }
            else if (x >= 1) // Checking for '!n', n >=1
                strcpy(inputBuffer, history[count - x]);
        }
    }

    for (i = 9; i > 0; i--) // Moving the history elements one step higher
        strcpy(history[i], history[i - 1]);

    strcpy(history[0], inputBuffer); // Updating the history array with input buffer

    count++;
    if (count > 10)
        count = 10;
}

int main()
{
    char cwd[MAX_LINE];           // Store the current working directory
    char inputBuffer[MAX_LINE];   // Buffer to hold the input command
    int flag;                     // Equals 1 if a command is followed by "&"
    char *args[MAX_LINE / 2 + 1]; // Max arguments

    pid_t pid;

    while (1)
    {
        flag = 0; // Flag = 0 by default

        // Finding the current working directory
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("getcwd() error");
            return 1;
        }

        // Printing the shell prompt
        printf("\033[1;32m");
        printf("FSH");
        printf("\033[0m");
        printf(":");
        printf("\033[1;34m");
        printf("%s", cwd);
        printf("\033[0m");
        printf("$ ");
        fflush(stdout);

        if (formatCommand(inputBuffer, args, &flag) != -1) // Get next command
        {
            pid = fork();

            if (pid < 0) // If pid is less than 0, forking fails
            {
                printf("Fork failed\n");
                exit(1);
            }
            else if (pid == 0) // If pid ==0
            {
                // Command not executed
                if (execvp(args[0], args) == -1)
                    printf("Error executing command\n");
            }
            //  If flag == 0, the parent will wait, otherwise returns to the formatCommand() function.
            else if (flag == 0)
                wait(NULL);
        }
    }

    return 0;
}