#define FUSE_USE_VERSION 26
#define PATH_MAX 4096
#include <fuse.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../libWad/Wad.h"

struct wad_state {
    Wad *a;
};

#define WAD_OBJ ((struct wad_state *) fuse_get_context()->private_data)

static int wad_getattr(const char *path, struct stat *st) {
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    if (WAD_OBJ->a->isDirectory(path)) {
        st->st_mode = S_IFDIR | 0555;
        st->st_nlink = 2;
    } else if (WAD_OBJ->a->isContent(path)) {
        st->st_mode = S_IFREG | 0444;
        st->st_nlink = 1;
        st->st_size = WAD_OBJ->a->getSize(path);
    } else {
        return -ENOENT;
    }
    return 0;
}

void *wad_init(struct fuse_conn_info *conn) {
    fuse_get_context();
    return WAD_OBJ;
}

int wad_open(const char *path, struct fuse_file_info *fi) {
    if (WAD_OBJ->a->isContent(path)) {
        fi->fh = 2;
        return 0;
    }
    return -ENOENT;
}

int wad_opendir(const char *path, struct fuse_file_info *fi){
    if (WAD_OBJ->a->isDirectory(path)){
        fi->fh = 2;
        return 0;
    }
    return -ENOENT;
}

int wad_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    if (WAD_OBJ->a->isContent(path)){
        int x = WAD_OBJ->a->getContents(path, buf, size, offset);
        return x;
    }
    return -ENOENT;
}

int wad_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                struct fuse_file_info *fi){
    vector<string> direc;
    struct stat s1, *st = &s1;
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    st->st_mode = S_IFDIR | 0555;
    st->st_nlink = 2;
    if (WAD_OBJ->a->isDirectory(path)){
        filler(buf, ".", &s1, 0);
        filler(buf, "..", &s1, 0);
        WAD_OBJ->a->getDirectory(path, &direc);
        for (vector<std::string>::size_type i = 0; i < direc.size(); i++){
            const char * c = direc.at(i).c_str();
            filler(buf, c, NULL, 0);
        }
        return 0;
    }
    return -ENOENT;
}

int wad_release(const char *path, struct fuse_file_info *fi){
    if (WAD_OBJ->a->isContent(path)){
        fi->fh = 0;
        return 0;
    }
    return -ENOENT;
}

int wad_releasedir(const char *path, struct fuse_file_info *fi) {
    if (WAD_OBJ->a->isDirectory(path)) {
        fi->fh = 0;
        return 0;
    }
    return -ENOENT;
}

int wad_mknod(const char *path, mode_t mode, dev_t dev) {
    WAD_OBJ->a->createFile(path);
    return 0;
}

int wad_mkdir(const char* path, mode_t mode) {
    WAD_OBJ->a->createDirectory(path);
    return 0;
}

int wad_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int written = WAD_OBJ->a->writeToFile(path, buf, size, offset);
    if (written < 0) {
        return -EIO;
    }
    return written;
}

void wad_destroy(void* userdata) {}

struct fuse_operations wad_oper = {
        .getattr = wad_getattr,
        .mknod = wad_mknod,
        .mkdir = wad_mkdir,
        .open = wad_open,
        .read = wad_read,
        .write = wad_write,
        .release = wad_release,
        .opendir = wad_opendir,
        .readdir = wad_readdir,
        .releasedir = wad_releasedir,
        .init = wad_init,
        .destroy = wad_destroy
};

int main(int argc, char *argv[]) {
    struct wad_state wad;
    if (argc < 3) {
        fprintf(stderr, "Too few arguments.\n");
        abort();
    }
    wad.a = Wad::loadWad(argv[argc-2]);
    if (wad.a == nullptr) {
        perror("Could not load wadobj");
        abort();
    }
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    int end = fuse_main(argc, argv, &wad_oper, &wad);
    return end;
}