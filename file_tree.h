#include<stdbool.h>

typedef struct FileTree* FileTree_t;

struct FileTree {
  bool isFile;
  char* path;
  FileTree_t right;
  FileTree_t  bottom;
};

void filetree_free(FileTree_t tree);
void make_path(const char* cwd, char* buf, char* relative);
bool check_extension(const char* filename, const char* extension);
void filetree_draw(FileTree_t tree, int depth);
FileTree_t filetree_construct(const char* path, const char** extensions, int n_extensions);
