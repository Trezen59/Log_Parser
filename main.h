#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <error.h>
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>

/*	To download libarchive package use the below command
	"sudo apt-get install libarchive-dev"
*/

#define TAR_BLOCK_SIZE	(10240) // 20*512
#define BUFFER_SIZE		(500)
#define MAX_LINE_LEN	(1024)
#define LOG_PATH		"tmp/offline_logs"

#define COMM3_LOG		"comm3.log"
#define COMM_LOG		"Comm.log"
#define HMA_LOG			"hma.log"
#define LIB_LOG			"libinterface.log"
#define NUM_LOG_FILES	(4)

#define COMM3_PATH		LOG_PATH"/"COMM3_LOG
#define COMM_PATH		LOG_PATH"/"COMM_LOG
#define HMA_PATH		LOG_PATH"/"HMA_LOG
#define LIB_PATH		LOG_PATH"/"LIB_LOG

#define RETURN_CENTER_IP "10.160"
