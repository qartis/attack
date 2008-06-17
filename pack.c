#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>

const char CHECK_SUFFIX[] = "DATA";
const uint32_t l_size = sizeof(uint32_t);
const uint32_t suffix_len = sizeof(CHECK_SUFFIX);

void append_file (int packfd, const char *path, const char *filename) {
	int datafd;
	char buffer[4096];
	uint32_t bufferlen;
	uint32_t taille;

	bufferlen = strlen(filename);
	write (packfd, filename, bufferlen);
	write (packfd, &bufferlen, l_size);

	taille = 0;
	datafd = open (path, O_RDONLY);
	if (-1 == datafd) {
		printf ("Error adding %s\n", path);
		exit(EXIT_FAILURE);
	}
    for(;;){
		bufferlen = read (datafd, buffer, sizeof(buffer));
		if (bufferlen > 0) taille += write (packfd, buffer, bufferlen);
		else break;
    }

	write (packfd, &taille, l_size);
	close (datafd);
}

void append_res (int packfd, const char *path, uint32_t* nb_res) {
	DIR *rep;
	struct dirent *entry;
	struct stat s;
	char* tmp_path = NULL;
	if (!(rep = opendir(path))) {
		fprintf (stderr, "Can't open %s\n", path);
		exit(EXIT_FAILURE);
	}
	while ((entry = readdir(rep))) {
		tmp_path = (char*)realloc (tmp_path, strlen(path)+strlen(entry->d_name)+2);
		strcpy (tmp_path, path);
		strcat (tmp_path, "/");
		strcat (tmp_path, entry->d_name);
		if (!strcmp (entry->d_name, ".")) continue;
		if (!strcmp (entry->d_name, "..")) continue;
		if (-1 == stat (tmp_path, &s)) {
			fprintf (stderr, "Can't stat %s\n", tmp_path);
			exit(EXIT_FAILURE);
		}
		if (S_ISREG(s.st_mode)) {
			append_file (packfd, tmp_path, entry->d_name);
			++(*nb_res);
		}
	}
	closedir(rep);
	free (tmp_path);
}

int main (int argc, char** argv) {
	int fd;
	uint32_t resources = 0;
	uint32_t couples = 0;
	char* file;

	if (argc < 2) {
		fprintf (stderr, "usage: %s <file>\n", argv[0]);
		return 1;
	}

	file = argv[1];

	fd = open (file, O_APPEND | O_WRONLY);
	if (-1 == fd) {
		return 1;
	}

	append_res (fd, "data", &resources);

	couples = 2 * resources;

	printf("%d resources packed\n", resources);
	write (fd, &couples, l_size);

	write (fd, CHECK_SUFFIX, suffix_len);
	close (fd);
	return 0;
}
