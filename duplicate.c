#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <uthash.h>

#define SMALL 4096
#define MULT 3

struct hash {
   size_t size;
   char *path;
   UT_hash_handle hh;
};

struct hash *hashes = NULL;

void *
emalloc(size_t size) {
   void *tmp = malloc(size);
   if(!tmp) {
      fprintf(stderr, "duplicate: Out of memory\n");
      exit(1);
   }
   return tmp;
}

int
is_same_file(const char *path1, const char *path2, size_t size) {
   static char buf1[SMALL+1], buf2[SMALL+1];
   FILE *f1 = fopen(path1, "r");
   FILE *f2 = fopen(path2, "r");
   int b = 0;
   if(size <= SMALL) {
      fread(buf1, 1, size, f1);
      fread(buf2, 1, size, f2);
      if(strncmp(buf1, buf2, size) == 0)
         b = 1;
   } else {
      fread(buf1, 1, SMALL, f1);
      fread(buf2, 1, SMALL, f2);
      if(strncmp(buf1, buf2, SMALL) == 0) {
         int start = SMALL;
         char *bbuf1, *bbuf2;
         bbuf1 = emalloc(size-SMALL+1);
         bbuf2 = emalloc(size-SMALL+1);
         int n;
         for(;;start *= MULT) {
            n = fread(bbuf1, 1, start, f1);
            if(n <= 0) {
               b = 1;
               break;
            }
            fread(bbuf2, 1, start, f2);
            if(strncmp(bbuf1, bbuf2, n) != 0)
               break;
         }
         free(bbuf1);
         free(bbuf2);
      }
   }
   fclose(f1);
   fclose(f2);
   return b;
}

void
process_dir(char *path, int len) {
   size_t dirsize = 20;
   char **dirs = emalloc(dirsize*sizeof(char*));
   size_t dirn = 0;
   static struct dirent *ent;
   static struct stat st;

   DIR *dir = opendir(".");
   while(ent = readdir(dir)) {
      if(ent->d_type == DT_DIR && strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
         if(dirn >= dirsize) {
            dirs = realloc(dirs, (dirsize *= 2) * sizeof(char*));
            if(!dirs) {
               fprintf(stderr, "duplicate: Out of memory\n");
               exit(1);
            }
         }
         dirs[dirn++] = strdup(ent->d_name);
      } else if(ent->d_type == DT_REG) {
         struct hash *suspect;
         stat(ent->d_name, &st);
         if(!st.st_size)
            continue;
         HASH_FIND_INT(hashes, &st.st_size, suspect);
         if(suspect) {
            if(is_same_file(ent->d_name, suspect->path, st.st_size))
               printf("rm %s%s # %s\n", path, ent->d_name, suspect->path);
         } else {
            struct hash *new = emalloc(sizeof(struct hash));
            new->path = emalloc(len+strlen(ent->d_name)+1);
            new->size = st.st_size;
            strcpy(new->path, path);
            strcpy(new->path+len, ent->d_name);
            HASH_ADD_INT(hashes, size, new);
         }
      }
   }
   for(int i = dirn-1; i >= 0; i--) {
      size_t ent_len = strlen(dirs[i]);
      strcpy(path+len, dirs[i]);
      strcpy(path + len + ent_len, "/");
      chdir(dirs[i]);
      free(dirs[i]);
      process_dir(path, len+ent_len+1);
   }
   closedir(dir);
   free(dirs);
   chdir("..");
}

int
main(int argc, char **argv) {
   if(argc < 2)
      return 1;
   char *path = emalloc(PATH_MAX+1);
   realpath(argv[1], path);
   size_t len = strlen(path);
   path[len++] = '/';
   path[len] = '\0';
   chdir(path);
   process_dir(path, len);
   free(path);
}
