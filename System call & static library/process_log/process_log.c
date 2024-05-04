#include "process_log.h"
#include <stdlib.h>

int get_proc_log_level() {
	int logLevel = syscall(GET_LOG_LEVEL);
	return logLevel;
}

int set_proc_log_level(int new_level) {
	int currentLogLevel = get_proc_log_level();
	int newLevel = syscall(SET_LOG_LEVEL, new_level);
	if(currentLogLevel != -1 && currentLogLevel >= newLevel) {
		return newLevel;
	}
	return currentLogLevel;
}

int proc_log_message(int level, char* message) {
	if(level > syscall(GET_LOG_LEVEL)) {
		return level;
	}
    int logLevel = syscall(PROC_LOG_CALL, message, level);
    return logLevel;
}

int* retrieve_set_level_params (int new_level) {
	int* params = (int*)malloc(sizeof(int) * 3);
	params[0] = SET_LOG_LEVEL;
	params[1] = 1;
	params[2] = new_level;
	return params;
}

int* retrieve_get_level_params () {
    int* params=(int*)malloc(sizeof(int) * 2);
    params[0] = GET_LOG_LEVEL;
    params[1] = 0;
	return params;
}

int interpret_set_level_result(int ret_value) {
	return ret_value;
}

int interpret_get_level_result(int ret_value) {
    return ret_value;
}

int interpret_log_message_result(int ret_value) {
    return ret_value;
}