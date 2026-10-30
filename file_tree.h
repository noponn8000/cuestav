#include<stdbool.h>

typedef struct FileTree* FileTree_t;

typedef enum {
  CUE,
  OTHER
} FileType;

typedef struct {
  bool isRegular;
  FileType type;
  char* path;
} FileView;

struct FileTree {
  bool isFile;
  char* path;
  FileTree_t right;
  FileTree_t  bottom;
};

void make_path(const char* cwd, char* buf, char* relative);
FileType get_file_type(const char* filename);
FileView* get_files(const char* path);

void filetree_free(FileTree_t tree);
bool check_extension(const char* filename, const char* extension);
void filetree_draw(FileTree_t tree, int depth);
FileTree_t filetree_construct(const char* path, const char** extensions, int n_extensions);
