#include "systemcalls.h"
#include <stdlib.h>     // system, exit
#include <stdbool.h>
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS
#include <unistd.h>     // fork, execv, dup2, close
#include <fcntl.h>      // open, O_CREAT, O_TRUNC, O_WRONLY



/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    if(cmd == NULL){
        return false;
    }

    int ret = system(cmd);

    if(ret == -1){
        return false;
    }

    return WIFEXITED(ret) && (WEXITSTATUS(ret) == 0);
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    // must be an asolute path
    if (command[0] == NULL || command[0][0] != '/') {
        va_end(args);
        return false;
    }

    va_end(args);

    pid_t pid = fork();
    if(pid == -1){
        return false;
    }

    if (pid == 0){
        execv(command[0],command);
        exit(-1);
    }

    int status;
    if(waitpid(pid, &status, 0) == -1){
        return false;
    }

    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    va_end(args);

    pid_t pid = fork();
    if(pid == -1){
        return false;
    }

    if (pid == 0){
        // open the output file
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(fd < 0){
            // connot open the file
            exit(-1);
        }

        // redirect stdout to this file
        if((dup2(fd, STDOUT_FILENO) < 0)){
            close(fd);
            exit(-1);
        }
        close(fd);

        // execute command
        execv(command[0], command);

        // if execv return, it is an error
        exit(-1);
    }
  
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return false;
    }

    // Return true only if child exited normally and code = 0
    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);

}
