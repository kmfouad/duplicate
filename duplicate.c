//fast -lxxhash
// duplicate -- find all duplicates in a directory
// depends on xxhash and uthash
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <xxhash.h>
#include <uthash.h>

#define SMALL_HASH_BYTES 4096

struct filehash {
    XXH64_hash_t digest;
    char *path;
    UT_hash_handle hh;
};

struct filehash *filehashes = NULL;

#undef HASH_FUNCTION
#undef HASH_KEYCMP
#define HASH_FUNCTION(s,len,hashv) ((hashv) = *s)
#define HASH_KEYCMP(a,b,len) (*(XXH64_hash_t*)a != *(XXH64_hash_t*)b)

static void
die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static inline size_t
filesize(const char *path) {
    size_t size;
    FILE *file;

    file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    return size;
}

static XXH64_hash_t
checksum(const char *path, int nbytes) {
    size_t size;
    XXH64_hash_t hash;
    char *buffer;
    FILE *file;

    fflush(stdout);
    file = fopen(path, "r");
    if(!file)
        die("Failed to open file");
    if(nbytes > 0) {
        size = nbytes;
    } else {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);
    }
    buffer = malloc(size+1);
    if(!buffer)
        die("Out of memory");
    size = fread(buffer, 1, size, file);
    hash = XXH64(buffer, size, 0);
    free(buffer);
    fclose(file);
    return hash;
}

static int
process_dir(const char *parent_path) {
    DIR *dir;
    static struct dirent *d;

    dir = eopendir(parent_path);
    while((d = readdir(dir))) {
        char path[PATH_MAX];

        if(d->d_name[0] == '.')
            continue;

        snprintf(path, sizeof(path), "%s/%s", parent_path, d->d_name);

        if(d->d_type == DT_DIR) {
            process_dir(path);
        } else {
            struct filehash *f, *suspect;
            f = malloc(sizeof(struct filehash));
            if(!f)
                die("Out of memory");
            f->digest = checksum(path, SMALL_HASH_BYTES);
            if(!f->digest) {
                free(f);
                continue;
            }
            f->path = strdup(path);
            HASH_FIND(hh,filehashes,&f->digest,sizeof(XXH64_hash_t),suspect);
            if(!suspect) {
                HASH_ADD(hh,filehashes,digest,sizeof(XXH64_hash_t),f);
            } else if(filesize(path) == filesize(suspect->path)) {
                XXH64_hash_t c1 = checksum(path, 0);
                XXH64_hash_t c2 = checksum(suspect->path, 0);
                if(c1 == c2) {
                    printf("rm %s # %s\n", path, suspect->path);
                    continue;
                }
            }
        }
    }
    closedir(dir);
    return 0;
}

int
main(int argc, char **argv) {
    if(argc < 2)
        return 1;
    for(int i = 1; i < argc; i++) {
        size_t len;
        len = strlen(argv[i]);
        while(argv[i][--len] == '/')
            argv[i][len] = '\0';
        process_dir(argv[i]);
    }
}
