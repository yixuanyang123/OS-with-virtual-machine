#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>

#define GET_LOG_LEVEL 435
#define SET_LOG_LEVEL 436
#define PROC_LOG_CALL 437

// Library functions
int get_proc_log_level();
int set_proc_log_level(int new_level);
int proc_log_message(int level, char* message);
// Harness functions
int* retrieve_set_level_params(int new_level);
int* retrieve_get_level_params();
int interpret_set_level_result(int ret_value);
int interpret_get_level_result(int ret_value);
int interpret_log_message_result(int ret_value);