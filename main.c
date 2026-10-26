#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "file_tree.h"

// Note: tilde is not expanded in this path. That is done by the shell.
#define DEFAULT_ROOT_DIRECTORY "/home/ferenc/Music"

int main(int argc, char** argv) {
  char* path = DEFAULT_ROOT_DIRECTORY;
  DIR* dir;
  
  /* Parse args */
  if (argc > 1) {
    path = argv[1];
  }

  const char* extensions[3] = {".cue", ".CUE", ".flac"};
  FileTree_t tree = filetree_construct(path, extensions, 3);

  filetree_draw(tree, 0);
  
  printf(".CUEstav music player\n");
  return EXIT_SUCCESS;
}
