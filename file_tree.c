#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#include "file_tree.h"
#include "array.h"

FileView* get_files(const char* path) {
  // Directory stream
  DIR* dir;
  // Directory entry pointer
  struct dirent* dp;    
  // Buffer for stat (contains file metadata)
  struct stat statbuf;
  // Create a directory file descriptor from the `path`
  int dfd = open(path, O_RDONLY);
  // And a file descriptor for the files in the above dir
  int ffd;

  // Using fdopendir over opendir to avoid race conditions
  dir = fdopendir(dfd);

  // Buffer for resolving paths.
  char buf[PATH_MAX + 1];

  FileView* files = NULL;
  
  if (dir == NULL) {
    perror("fdopendir");
    return NULL;
  }

  while ((dp = readdir(dir))) {
    // Clear path buffer
    memset(buf, 0, sizeof(buf));

    ffd = openat(dfd, dp->d_name, O_RDONLY);
    if (ffd == -1) {
      perror ("openat");
      return NULL;
    }

    // Fill the stat buffer
    fstat(ffd, &statbuf);
    // We don't care about the file descriptor anymore
    close(ffd);

    FileView file = {
      .isRegular = true,
      .type = OTHER,
    };

    // Now we can use the S_IS... macros to find the type of the file
    // We only care about directories and regular files
    if (S_ISREG(statbuf.st_mode)) {
      int dot_index = -1;
      for (int i = strlen(dp->d_name); i >= 0; i--) {
        if (dp->d_name[i] == '.') {
          dot_index = i;
          break;
        }
      }

      if (dot_index != -1) {
        char* extension = dp->d_name + dot_index;
        if (strcasecmp(extension, ".cue") == 0) {
          file.type = CUE;
        }
      }
    } else if (S_ISDIR(statbuf.st_mode)) {
      file.isRegular = false;
    } else {
      // Not a directory nor file. Not including this
      continue;
    }
    
    file.path = malloc(sizeof(char) * PATH_MAX + 1);
    // Construct absolute path
    make_path(path, buf, dp->d_name);
    strcpy(file.path, buf);

    arr_push(files, file);
  }  
  (void) closedir(dir);

  return files;
}

void filetree_free(FileTree_t tree) {
  if (tree->right != NULL) {
    filetree_free(tree->right);
  }
  if (tree -> bottom != NULL) {
    filetree_free(tree->bottom);
  }

  free(tree->path);
}

// Helper to concatenate cwd with relative path and insert slash if necessary
void make_path(const char* cwd, char* buf, char* relative) {
    // First copy the current working directory path into the buffer
    strcpy(buf, cwd);
    // Insert a slash if there isn't one
    if (buf[strlen(buf) - 1] != '/' && relative[0] != '/')
       buf[strlen(buf)] = '/'; 

    strcat(buf, relative);
}

bool check_extension(const char* filename, const char* extension) {
  int l1 = strlen(filename);
  int l2 = strlen(extension);
  if (l1 < l2 || l1 == 0 || l2 == 0) return false;

  for (int i1 = l1 - 1, i2 = l2 - 1; i1 > 0 && i2 > 0; i1--, i2--) {
    if (filename[i1] != extension[i2])
      return false;

    if (filename[i1] == '.') break;
  }

  return true;
}

void filetree_draw(FileTree_t tree, int depth) {
  if (tree == NULL) return;

  FileTree_t current = tree;
  while (current != NULL) {
    for (int i = 0; i < depth; i++) {
      printf("| ");
    }

    puts(current->path);
    if (current->bottom != NULL) {
      filetree_draw(current->bottom, depth + 1);
    }
    current = current->right;
  }
}

FileTree_t filetree_construct(const char* path, const char** extensions, int n_extensions) {
  // Directory stream
  DIR* dir;
  // Directory entry pointer
  struct dirent* dp;    
  // Buffer for stat (contains file metadata)
  struct stat statbuf;
  // Create a directory file descriptor from the `path`
  int dfd = open(path, O_RDONLY);
  // And a file descriptor for the files in the above dir
  int ffd;

  // Using fdopendir over opendir to avoid race conditions
  dir = fdopendir(dfd);

  // Buffer for resolving paths.
  char buf[PATH_MAX + 1];

  if (dir == NULL) {
    perror("fdopendir");
    return NULL;
  }

  FileTree_t root = NULL;
  FileTree_t prev = NULL;
  FileTree_t current = NULL;

  while ((dp = readdir(dir))) {
    // Clear path buffer
    memset(buf, 0, sizeof(buf));

    ffd = openat(dfd, dp->d_name, O_RDONLY);
    if (ffd == -1) {
      perror ("openat");
      return NULL;
    }

    // Skip hidden directories and . / ..
    if (dp->d_name[0] == '.')
      continue;
    
    // Fill the stat buffer
    fstat(ffd, &statbuf);
    // We don't care about the file descriptor anymore
    close(ffd);

    // Now we can use the S_IS... macros to find the type of the file
    // We only care about directories and regular files
    bool matches_any = false;
    if (S_ISREG(statbuf.st_mode)) {
      for (int i = 0; i < n_extensions; i++) {
        if (check_extension(dp->d_name, extensions[i])) {
          matches_any = true;
          break;
        }
      }

      if (!matches_any) continue;
    }

    current = malloc(sizeof(struct FileTree));
    current->path = malloc(sizeof(char) * PATH_MAX + 1);
    current->isFile = true;
    current->bottom = NULL;
    current->right = NULL;

    // Construct absolute path
    make_path(path, buf, dp->d_name);
    strcpy(current->path, buf);

    // Link the current node into the previous
    if (prev != NULL) {
      prev->right = current;
    } else {
      root = current;
    }
    prev = current;

    if (S_ISDIR(statbuf.st_mode)) {
      current->isFile = false;

      // Recur into the subdirectory
      current->bottom = filetree_construct(buf, extensions, n_extensions);
    }
  }  
  (void) closedir(dir);

  return root;
}
