/*	
 *	CS 631 - APUE
 *	Homework 2
 *	
 *	"I pledge my honor that I have abided by the Stevens Honor System."
 *	
 *	(c) 2015 Patrick Murray, pmurray1@stevens.edu
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include <fcntl.h>
#include <libgen.h>

#include <sys/stat.h>
#include <sys/types.h>

/*	
 *	Define the default buffer size if one has not been set already.
 *	Runtime testing has shown that a buffer size of 4096 bytes has an
 *	optimal performance/memory ratio, as shown in the README.
 */

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 4096
#endif


int main(int argc, char **argv)
{
	char *source_filepath;
	char *target_filepath;
	
	int source_fd;
	int directory_fd;
	int target_fd;
	
	struct stat stat_buffer;
	mode_t source_mode;
	ino_t source_inode;
	ino_t target_inode;
	
	char source_realpath_buffer[PATH_MAX];
	char target_realpath_buffer[PATH_MAX];
	char *source_absolute_path;
	char *target_absolute_path;
	
	int  bytes_read;
	char buffer[BUFFER_SIZE];
	
	/*	
	 *	If the user neither provides a source nor a target destination,
	 *	then we display the program's usage information and exit the
	 *	program in failure.
	 */
	if (argc != 3) {
		fprintf(stderr, "usage: %s [source] [target]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/*	
	 *	Personally, I don't find argv[1] and argv[2] to be very
	 *	readable, so I'll use these two variables below.
	 */
	source_filepath = argv[1];
	target_filepath = argv[2];
	
	/*	
	 *	Open the source file for only reading, if an error occurs then
	 *	alert the user through stderr and exit the program in failure.
	 */
	if ((source_fd = open(source_filepath, O_RDONLY)) < 0) {
		fprintf(stderr, "tcp: unable to read '%s': %s\n",
		        source_filepath, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/*	
	 *	Since we have already opened the source file for reading, we may
	 *	now use its file descriptor to request its permissions. This is
	 *	important so that we assign the target destination the same
	 *	permissions as the source file. Once again, if an error occurs
	 *	while fetching the permissions, we will notify the user and exit
	 *	the program in failure.
	 */
	if (fstat(source_fd, &stat_buffer) < 0) {
		fprintf(stderr, "tcp: unable to get file status of '%s': %s\n",
		        source_filepath, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/*	
	 *	Once again, I've assigned the two following variables to
	 *	increase the program's readability.
	 */
	source_mode = stat_buffer.st_mode;
	source_inode = stat_buffer.st_ino;
	
	/*	
	 *	We must compare the inode attributes of both the source file and
	 *	the target destination to determine that they are not the same
	 *	file. If the files were identical and this test were not
	 *	performed, the O_TRUNC option passed to open(2) below would
	 *	truncate the file before attempting to read any data into the
	 *	buffer for writing. Ultimately, this would result in the
	 *	contents of the source file to be deleted - making the user sad.
	 *	
	 *	As a note, this request to stat(1) will fail if the target
	 *	destination is a directory, as such the error is ignored if that
	 *	is the case.
	 */
	if (stat(target_filepath, &stat_buffer) >= 0) {
		/* If the target destination is a file which already exists. */
		target_inode = stat_buffer.st_ino;
		
		/*	
		 *	If the source and target destination are identical files,
		 *	the program will exit in success, since the target contains
		 *	identical contents as the source.
		 */
		if (source_inode == target_inode) {
			exit(EXIT_SUCCESS);
		}
	} else {
		/* If an error occured while requesting stat(1) */
		switch (errno) {
			case ENOENT:
				/* Ignore the error if the file does not exist yet. */
				break;
			default:
				/* Otherwise, alert the user of the error and exit. */
				fprintf(stderr, "tcp: unable to get file status of '%s': %s\n",
				       target_filepath, strerror(errno));
				exit(EXIT_FAILURE);
		}
	}
	
	/*	
	 *	Because directories are supported as target destinations, we
	 *	must check if the provided target is a directoriy using open(2)
	 *	with the O_DIRECTORY option. If the target is indeed a
	 *	directory, then we create a new file with the same name as the
	 *	source inside the destination.
	 *	
	 *	Additionally, if the absolute path of the current working
	 *	directory and the target destination are identical, then we exit
	 *	the program in success because the source is already contained
	 *	within the target.
	 */
	if ((directory_fd = open(target_filepath, O_DIRECTORY)) >= 0) {
		/*	
		 *	Get the source absolute path, alert the user if a problem
		 *	occurs and exit on failure.
		 */
		if ((source_absolute_path = realpath(source_filepath,
		     source_realpath_buffer)) == NULL) {
			fprintf(stderr, "tcp: could not resolve source path: %s\n",
			        strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		/*	
		 *	Resolve the absolute path of the target directory, if an
		 *	error occurs, alert the user and exit the program in
		 *	failure.
		 */
		if ((target_absolute_path = realpath(target_filepath,
		     target_realpath_buffer)) == NULL) {
			fprintf(stderr, "tcp: could not resolve target path: %s\n",
			        strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		/*	
		 *	If the paths are identical, then the source file already
		 *	exists in the target directory - similar to the inode
		 *	comparison above - exit the program in success.
		 */
		if (strcmp(dirname(source_absolute_path),
		           target_absolute_path) == 0) {
			exit(EXIT_SUCCESS);
		}
		
		
		/*	
		 *	Attempt to create the new file in the target directory. If
		 *	an error occurs, alert the user and exit in failure.
		 */
		if ((target_fd = openat(directory_fd, basename(source_filepath),
		     O_WRONLY | O_CREAT | O_TRUNC, source_mode)) < 0) {
			fprintf(stderr, "tcp: could not create '%s' in '%s': %s\n",
			        target_filepath, basename(source_filepath),
			        strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		/*	
		 *	Close the target destination directory, prompt the user if
		 *	an error occurs and exit in failure.
		 */
		if (close(directory_fd) < 0) {
			fprintf(stderr, "tcp: unable to close target directory: %s\n",
			        strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else if ((target_fd = open(target_filepath, O_WRONLY | O_CREAT |
				O_TRUNC, source_mode)) < 0) {
		/*	
		 *	If an error occurs trying to open the target destination
		 *	file, then alert the user and exit the program in failure.
		 */
		fprintf(stderr, "tcp: could not open target destination '%s': %s\n",
		        target_filepath, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	
	/*	
	 *	Now that we finally have a file descriptor to the source and
	 *	target destination, we can read data into the buffer and write
	 *	it to the target.
	 */
	while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
		/*	
		 *	If an error occurs writing data to the target destination,
		 *	alert the user and exit the program.
		 */
		if (write(target_fd, buffer, bytes_read) != bytes_read) {
			fprintf(stderr, "tcp: write to '%s' failed: %s\n",
			        target_filepath, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	
	/*	
	 *	If the number of bytes read is negative, it indicates that
	 *	read(2) encountered an error while reading the file. Display the
	 *	error to the user and exit the program in failure.
	 */
	if (bytes_read < 0) {
		fprintf(stderr, "tcp: an error occured while reading '%s': %s\n",
		        source_filepath, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/*	
	 *	If any error occur while closing the source or target
	 *	descriptors, prompt the user and exit in failure.
	 */
	if (close(source_fd) < 0) {
		fprintf(stderr, "tcp: could not close '%s': %s\n",
		        source_filepath, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if (close(target_fd) < 0) {
		fprintf(stderr, "tcp: could not close '%s': %s\n",
		        target_filepath, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	
	/*	
	 *	If the program made it this far then nothing exploded, yay!
	 *	
	 *	Obligatory GIF:
	 *	i.imgur.com/6zeve8L.gif
	 */
	return EXIT_SUCCESS;
}
