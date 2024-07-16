#include "main.h"

#define VERSION "1.0"

/*	To download libarchive package use the below command
 	"sudo apt-get install libarchive-dev"
*/

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
	if ((r = archive_read_open_filename(a, filename, TAR_BLOCK_SIZE)))
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

/* main program entry */
int main(int argc, char *argv[])
{
	bool wordFound = false;
	int ret = 0;
	int file = 0;
	int buffer_index = 0;
	int lines_in_buffer = 0;
	int NUM_LOG_FILES = 4;
	char line[MAX_LINE_LEN];
	char buffer[BUFFER_SIZE][MAX_LINE_LEN];
	char *LOG_FILES[] = {COMM_PATH, HMA_PATH, LIB_PATH, COMM3_PATH};
	char *tarFile = argv[1];
	char *WordToFind = "Listen";
	FILE *comm3File;

	printf("Log parser Version: %s\n", VERSION);

	/* Print the file name we are using */
	if (argc != 2)
	{
		printf("tar file missing\n");
		return -1;
	}
	printf("using %s file\n", tarFile);

	/* Extract the tar file in a folder */
	ret = extractTar(tarFile);
	if (ret < 0)
	{
		printf("Failed to extract the tar file: %s\n", tarFile);
		exit(1);
	}

	/* check if the log files exist in the paths */
	for (file = 0; file < NUM_LOG_FILES; file++)
	{
		ret = isFileExist(LOG_FILES[file]);
		if (ret < 0)
		{
			printf("Cannot access file: %s\n", LOG_FILES[file]);
		}
	}

	/* Open comm3.log */
	comm3File = fopen(COMM3_PATH, "r");
	if (comm3File == NULL)
	{
		perror("Error opening file\n");
		exit(1);
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
			printf("The IP: %s is found in the line: %s\n", RETURN_CENTER_IP, line);
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
			printf("Low voltage interrupt found in last logs.\n");
		}
	}


	/* CLose comm3.log file */
	fclose(comm3File);

	return 0;
}
