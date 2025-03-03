/*
 * Copyright 2021 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// FIXME: Merge with other existing close and open tests.

int main() {
  // Test creating a new file and writing and reading from it.
  errno = 0;
  int fd = open("/test", O_RDWR | O_CREAT, 0777);

  // Check that the file type is correct on mode.
  struct stat file;
  fstat(fd, &file);

  assert((file.st_mode & S_IFMT) == S_IFREG);
  printf("mode %i\n", file.st_mode);
  assert(file.st_mode == (S_IRWXUGO | S_IFREG));

  assert(errno == 0);
  const char* msg = "Test\n";
  errno = 0;
  write(fd, msg, strlen(msg));
  assert(errno == 0);
  // Attempt to open another FD to the file just created.
  errno = 0;
  int test = open("/test", O_RDWR);
  assert(errno == 0);
  char buf[100] = {};
  errno = 0;
  read(test, buf, sizeof(buf));
  assert(errno == 0);
  printf("%s", buf);
  close(fd);
  close(test);

  // Try to create an existing file with O_EXCL and O_CREAT.
  errno = 0;
  int fd2 = open("/dev/stdin", O_RDWR | O_CREAT | O_EXCL);
  printf("Errno: %s\n", strerror(errno));
  assert(errno == EEXIST);

  // Try to open a file with O_DIRECTORY.
  errno = 0;
  int fd3 = open("/dev/stdin", O_RDWR | O_DIRECTORY);
  printf("Errno: %s\n", strerror(errno));
  assert(errno == ENOTDIR);

  // Try to open a directory with O_DIRECTORY.
  errno = 0;
  int fd4 = open("/dev", O_RDONLY | O_DIRECTORY);
  printf("Errno: %s\n", strerror(errno));
  assert(errno == 0);

  // Test zero size reads and writes.
  char buf2[100] = {};
  int fd5 = open("/newFile", O_RDWR | O_CREAT);
  errno = 0;
  printf("Read %zi bytes\n", read(fd5, buf2, 0));
  assert(errno == 0);
  printf("Wrote %zi bytes\n", write(fd5, msg, 0));
  assert(errno == 0);

  // Test large size reads and writes multiple times.
  char buf3[100] = {};
  int fd6 = open("/testFile", O_RDWR | O_CREAT);
  errno = 0;
  printf("Wrote %zi bytes\n", write(fd6, msg, strlen(msg) + 20));
  printf("Wrote %zi bytes\n", write(fd6, msg, strlen(msg)));
  printf("Wrote %zi bytes\n", write(fd6, msg, strlen(msg) + 30));
  printf("Read %zi bytes\n", read(fd6, buf, 10000));

  int fd7 = open("/testFile", O_RDWR);
  assert(errno == 0);
  printf("Read %zi bytes\n", read(fd7, buf3, sizeof buf3));
  printf("File contents: %s", buf3);
  assert(errno == 0);

  // TODO: use seek to test out of bounds read.

  return 0;
}
