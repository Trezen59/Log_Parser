#include "main.h"

#define VERSION "2.0"

char *gfolderPath = NULL;
char *gLOG_FILES[] = {COMM_PATH, HMA_PATH, LIB_PATH, COMM3_PATH};
char *gNoVoltFilePaths[MAX_LINE_LEN] = {0};
char *gVoltFilePaths[MAX_LINE_LEN] = {0};
int gNoVoltCount = 0;
int gVoltCount = 0;

/* Extract the tar file and real the files */
int extractTar(const char *filename)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int flags;
	int r;

	// Select which attributes to restore
	flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
		ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;

	// Allocate and initialize the read archive object
	a = archive_read_new();
	archive_read_support_format_tar(a); // Add support for tar format
	archive_read_support_filter_gzip(a); // Add support for gzip format

	// Allocate and initialize the write archive object for disk extraction
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	// Open the tar file
	r = archive_read_open_filename(a, filename, TAR_BLOCK_SIZE);
	if (r != ARCHIVE_OK)
	{
		printf("Could not open %s: %s\n", filename, archive_error_string(a));
		return -1;
	}

	// Read each entry in the tar file and extract it
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
		//printf("Extracting %s\n", archive_entry_pathname(entry));
		archive_write_header(ext, entry);
		const void *buff;
		size_t size;
		int64_t offset;

		// Read the data blocks of the entry
		while ((r = archive_read_data_block(a, &buff, &size, &offset)) != ARCHIVE_EOF)
		{
			if (r < ARCHIVE_OK)
			{
				printf("Error reading data block: %s\n", archive_error_string(a));
				return -1;
			}
			if (r > ARCHIVE_WARN)
			{
				archive_write_data_block(ext, buff, size, offset);
			}
		}
		archive_write_finish_entry(ext);
	}

	// Clean up
	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);
}

/* Check if file exists */
int isFileExist(char *FILE_PATH)
{
	int ret = -1;
	if (access(FILE_PATH, F_OK) != -1)
	{
		ret = 0;
	}
	return ret;
}

/* function to compare the IP to return center IP */
bool compareIP(const char *line, const char *target_ip)
{
	int i = 0;
    const char *interface = "wlan0 ";
    char *pos = strstr(line, interface);
	char ip[6] = {0};

    if (pos != NULL)
	{
        pos += strlen(interface);

        /* Extract the IP address, only first 5 digits
		   because observed the first 5 digits remain constant
		   in return centre IP
		 */
        while (*pos != '\0' && *pos != ':' && i < 6)
		{
            ip[i++] = *pos++;
        }
        ip[i] = '\0';

		//printf("IP: %s\n", ip);

        /* Compare the extracted IP address with the target IP */
        if (strcmp(ip, target_ip) == 0)
		{
            return true;
        }
    }
    return false;
}

/* Fill the array of files paths */
int addFilePath(int index, char *path, char **arr)
{
	if (index < 0 || index >= MAX_LINE_LEN)
	{
		printf("index out of bound\n");
		return -1;
	}

	arr[index] = (char *)malloc(strlen(path) + 1);
	if (arr[index] == NULL)
	{
		printf("Failed to allocate memory to path.\n");
		return -1;
	}

	strcpy(arr[index], path);
	return 0;
}

/* Action items specified on log files to perform */
int logFileAnalysis(const char *folder, char *tarFile)
{
	FILE *comm3File;
	bool isLowVoltError = false;
	int file = 0;
	int ret = -1;
	int buffer_index = 0;
	int lines_in_buffer = 0;
	char line[MAX_LINE_LEN];
	char buffer[BUFFER_SIZE][MAX_LINE_LEN];
	char fullFilePath[1024];

	/* Folder + file path */
	sprintf(fullFilePath, "%s%s", folder, tarFile);

	/* Extract the tar file in a folder */
	ret = extractTar(fullFilePath);
	if (ret < 0)
	{
		printf("Failed to extract the tar file: %s\n", fullFilePath);
		return -1;
	}

	/* check if the log files exist in the paths */
	for (file = 0; file < NUM_LOG_FILES; file++)
	{
		ret = isFileExist(gLOG_FILES[file]);
		if (ret < 0)
		{
			printf("Cannot access file: %s\n", gLOG_FILES[file]);
			return -1;
		}
	}

	/* Open comm3.log */
	comm3File = fopen(COMM3_PATH, "r");
	if (comm3File == NULL)
	{
		perror("Error opening file\n");
		return -1;
	}

	/* Find "Listen" string and check for return center IP */
	while (fgets(line, sizeof(line), comm3File))
	{
        /* Store the line in the circular buffer */
        strcpy(buffer[buffer_index], line);
        buffer_index = (buffer_index + 1) % BUFFER_SIZE;
        if (lines_in_buffer < BUFFER_SIZE)
		{
            lines_in_buffer++;
        }

		/* Compare IP */
		if(compareIP(line, RETURN_CENTER_IP))
		{
			printf("The IP: %s is found in the line:\n%s\n", RETURN_CENTER_IP, line);
#if 0
			/* Print the buffer */
			for (int i = 0; i < lines_in_buffer; i++)
			{
				printf("%s", buffer[(buffer_index + i) % BUFFER_SIZE]);
			}
#endif
			break;
		}
	}

	/* in buffer find for low voltage interrupt */
	for (int i = 0; i < lines_in_buffer; i++)
	{
		if (strstr(buffer[(buffer_index + i) % BUFFER_SIZE], "low voltage") != NULL)
		{
			isLowVoltError = true;
			printf("Low voltage interrupt found in last logs. line:\n%s \n",
					buffer[(buffer_index + i) % BUFFER_SIZE]);
			break;
		}
	}

	if (!isLowVoltError)
	{
		/* Store the file path in global array */
		ret = addFilePath(gNoVoltCount, fullFilePath, gNoVoltFilePaths);
		if (ret != 0)
		{
			printf("Failed adding path to array.\n");
			fclose(comm3File);
			return -1;
		}
		gNoVoltCount++;
		isLowVoltError = false;
	}
	else
	{
		ret = addFilePath(gVoltCount, fullFilePath, gVoltFilePaths);
		if (ret != 0)
		{
			printf("Failed adding path to array.\n");
			fclose(comm3File);
			return -1;
		}
		gVoltCount++;
	}

	/* CLose comm3.log file */
	fclose(comm3File);

	printf("===================================================\n");

	return 0;
}

/* for every file in the directory do the analysis */
int iterate_files_in_directory(const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);
	int i = 0;
	int ret = -1;

    if (dp == NULL)
	{
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dp)) != NULL)
	{
        if (entry->d_type == DT_REG)
		{
            printf("\nFolder: %s, File: %s\n", path, entry->d_name);
			ret = logFileAnalysis(path, entry->d_name);
			if (ret != 0)
			{
				printf("Failed to run analysis on '%s' log file.\n", entry->d_name);
				return -1;
			}
        }
		else if (entry->d_type == DT_DIR)
		{
            // Skip the current and parent directories
            if (strcmp(entry->d_name, ".") == 0
					|| strcmp(entry->d_name, "..") == 0)
			{
                continue;
            }
            printf("Directory: %s\n", entry->d_name);
        }
		else
		{
            printf("Other: %s\n", entry->d_name);
        }
    }
    closedir(dp);
	return 0;
}

/* main program entry */
int main(int argc, char *argv[])
{
	bool wordFound = false;
	int ret = 0;
	int file = 0;
	char *WordToFind = "Listen";
	int i = 0;

	gfolderPath = argv[1];
	printf("Log parser Version: %s\n", VERSION);

	/* Print the file name we are using */
	if (argc != 2)
	{
		printf("folder path missing\n");
		exit(1);
	}
	printf("using %s folder\n", gfolderPath);

	ret = iterate_files_in_directory(gfolderPath);
	if (ret != 0)
	{
		printf("Failed to iterate over files in directory: %s\n", gfolderPath);
		exit(1);
	}

	/* show files with no low voltage issue found */
	printf("\nFiles with no low voltage interrupts.\n");
	for (i = 0; i < gNoVoltCount; i++)
	{
		printf("%s\n", gNoVoltFilePaths[i]);
	}

	/* show files with low voltage issue found */
	printf("\nFiles with low voltage interrupts.\n");
	for (i = 0; i < gVoltCount; i++)
	{
		printf("%s\n", gVoltFilePaths[i]);
	}

	printf("\nLog parser executed successfully\n");
	return 0;
}
