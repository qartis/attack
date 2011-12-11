#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bzlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <libgen.h>
#include <curl/curl.h>

#include "patchouli.h"

#define DEBUG(...) do { \
    fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\n"); } \
    while (0)

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef S_IRWXU
#define PERMS S_IRWXU|S_IRWXG|S_IRWXO
#else
#define PERMS 0666
#endif

#define ID_LEN 16

enum patchouli_err patchouli_init(struct patchouli_t *p)
{
    p->url = NULL;
    p->easy = curl_easy_init();
    return PATCHOULI_OK;
}

enum patchouli_err patchouli_set_url(struct patchouli_t *p, const char *url)
{
    p->url = strdup(url);
    return PATCHOULI_OK;
}

enum patchouli_err patchouli_set_patch_file(struct patchouli_t *p,
                                            const char *file)
{
    p->patch_file = strdup(file);
    return PATCHOULI_OK;
}

enum patchouli_err patchouli_set_vars(struct patchouli_t *p, const char *vars)
{
    p->http_post_vars = strdup(vars);
    return PATCHOULI_OK;
}

enum patchouli_err patchouli_request_patch(struct patchouli_t *p)
{
    if (p == NULL || p->url == NULL) {
        return PATCHOULI_ERR_INIT;
    }

    FILE *patch_file = fopen(p->patch_file, "wb");
    if (patch_file == NULL) {
        p->error = errno;
        return PATCHOULI_ERR_FS;
    }

    curl_easy_setopt(p->easy, CURLOPT_URL, p->url);
    curl_easy_setopt(p->easy, CURLOPT_POSTFIELDS, p->http_post_vars);
    curl_easy_setopt(p->easy, CURLOPT_WRITEDATA, patch_file);
    curl_easy_setopt(p->easy, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(p->easy, CURLOPT_NOPROGRESS, 1L);
    //curl_easy_setopt(p->easy, CURLOPT_VERBOSE, 1L);
    CURLcode res = curl_easy_perform(p->easy);

    if (res != CURLE_OK) {
        goto err;
    }

    long response;
    res = curl_easy_getinfo(p->easy, CURLINFO_RESPONSE_CODE, &response);

    if (res != CURLE_OK) {
        goto err;
    }
    if (response == 304L) {
        printf("empty patch\n");
        remove(p->patch_file);
    } else if (response == 200L) {
        printf("got patch");
    } else {
        printf("wtf strange response %ld\n", response);
        goto err;
    }
    return PATCHOULI_OK;

err:
    p->error = res;
    remove(p->file);
    curl_easy_cleanup(p->easy);
    return PATCHOULI_ERR_CURL;
}

char *uniqid()
{
    char id[ID_LEN + 1];
    id[0] = '.';
    int i;
    for (i = 1; i < ID_LEN; i++) {
        id[i] = 'a' + rand() % 16;
    }
    id[ID_LEN] = '\0';
    return strdup(id);
}

static int copyfile(const char *from, const char *to)
{
    int f, t;
    char buf[4096];
    ssize_t len;

    f = open(from, O_RDONLY | O_BINARY);
    if (-1 == f) {
        DEBUG("can't open %s: %s", from, strerror(errno));
        return 1;
    }

    t = open(to, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, PERMS);
    if (-1 == t) {
        DEBUG("can't create %s: %s", to, strerror(errno));
        goto abort;
    }

    for (;;) {
        len = read(f, buf, sizeof(buf));
        if (len < 0) {
            DEBUG("failed to read file: %s", strerror(errno));
            goto abort;
        } else if (len > 0) {
            if (write(t, buf, len) < 1) {
                DEBUG("failed to write %ld bytes: %s", len, strerror(errno));
            }
        } else
            break;
    }

    DEBUG("done copying file");
    close(f);
    close(t);
    chmod(to, S_IRUSR | S_IWUSR | S_IXUSR);
    return 0;

abort:
    DEBUG("aborting patch");
    close(f);
    close(t);
    unlink(to);
    return 1;
}

static off_t offtin(uint8_t * buf)
{
    off_t y;

    y = buf[7] & 0x7F;
    y = y * 256;
    y += buf[6];
    y = y * 256;
    y += buf[5];
    y = y * 256;
    y += buf[4];
    y = y * 256;
    y += buf[3];
    y = y * 256;
    y += buf[2];
    y = y * 256;
    y += buf[1];
    y = y * 256;
    y += buf[0];

    if (buf[7] & 0x80)
        y = -y;

    return y;
}

/* copied almost verbatim from bsdiff */
char *apply_patch(const char *patch_path, const char *binary_path)
{
    FILE *f, *cpf, *dpf, *epf;
    BZFILE *cpfbz2, *dpfbz2, *epfbz2;
    int cbz2err, dbz2err, ebz2err;
    ssize_t oldsize = 0, newsize;
    ssize_t bzctrllen, bzdatalen;
    unsigned char header[32], buf[8];
    unsigned char *old = NULL, *new = NULL;
    off_t oldpos, newpos;
    off_t ctrl[3];
    off_t lenread;
    int i;
    int fd;

    char *tempfile = uniqid();

    if (copyfile(binary_path, tempfile)) {
        remove(patch_path);
        return NULL;
    }

    /* Open patch file */
    if ((f = fopen(patch_path, "rb")) == NULL) {
        goto bail;
    }

    if (fread(header, 1, 32, f) < 32) {
        goto bail;
    }

    /* Check for appropriate magic */
    if (memcmp(header, "BSDIFF40", 8) != 0) {
        goto bail;
    }

    /* Read lengths from header */
    bzctrllen = offtin(header + 8);
    bzdatalen = offtin(header + 16);
    newsize = offtin(header + 24);
    if ((bzctrllen < 0) || (bzdatalen < 0) || (newsize < 0)) {
        goto bail;
    }

    /* Close patch file and re-open it via libbzip2 at the right places */
    if (fclose(f)) {
        goto bail;
    }
    if ((cpf = fopen(patch_path, "rb")) == NULL) {
        goto bail;
    }
    if (fseek(cpf, 32, SEEK_SET)) {
        goto bail;
    }
    if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL) {
        goto bail;
    }
    if ((dpf = fopen(patch_path, "rb")) == NULL) {
        goto bail;
    }
    if (fseek(dpf, 32 + bzctrllen, SEEK_SET)) {
        goto bail;
    }
    if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL) {
        goto bail;
    }
    if ((epf = fopen(patch_path, "rb")) == NULL) {
        goto bail;
    }
    if (fseek(epf, 32 + bzctrllen + bzdatalen, SEEK_SET)) {
        goto bail;
    }
    if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL) {
        goto bail;
    }

    fd = open(tempfile, O_RDONLY | O_BINARY, 0);
    if (fd < 0) {
        goto bail;
    }

    if ((oldsize = lseek(fd, 0, SEEK_END)) == -1) {
        goto bail;
    }

    if ((old = malloc(oldsize + 1)) == NULL) {
        goto bail;
    }

    if (lseek(fd, 0, SEEK_SET) != 0) {
        goto bail;
    }

    if (read(fd, old, oldsize) != oldsize) {
        DEBUG("WHOOPS %s", tempfile);
        goto bail;
    }

    if (close(fd) == -1) {
        goto bail;
    }

    if ((new = malloc(newsize + 1)) == NULL) {
        DEBUG("malloc");
        goto bail;
    }

    oldpos = 0;
    newpos = 0;
    while (newpos < newsize) {
        /* Read control data */
        for (i = 0; i <= 2; i++) {
            lenread = BZ2_bzRead(&cbz2err, cpfbz2, buf, 8);
            if ((lenread < 8)
                || ((cbz2err != BZ_OK) && (cbz2err != BZ_STREAM_END))) {
                DEBUG("corrupt patch");
                goto bail;
            }
            ctrl[i] = offtin(buf);
        };

        /* Sanity-check */
        if (newpos + ctrl[0] > newsize) {
            DEBUG("corrupt patch");
            goto bail;
        }

        /* Read diff string */
        lenread = BZ2_bzRead(&dbz2err, dpfbz2, new + newpos, ctrl[0]);
        if ((lenread < ctrl[0]) ||
            ((dbz2err != BZ_OK) && (dbz2err != BZ_STREAM_END))) {
            DEBUG("corrupt patch: %ld (error code %d)", lenread, dbz2err);
            goto bail;
        }

        /* Add old data to diff string */
        for (i = 0; i < ctrl[0]; i++)
            if ((oldpos + i >= 0) && (oldpos + i < oldsize))
                new[newpos + i] += old[oldpos + i];

        /* Adjust pointers */
        newpos += ctrl[0];
        oldpos += ctrl[0];

        /* Sanity-check */
        if (newpos + ctrl[1] > newsize) {
            goto bail;
        }

        /* Read extra string */
        lenread = BZ2_bzRead(&ebz2err, epfbz2, new + newpos, ctrl[1]);
        if ((lenread < ctrl[1]) ||
            ((ebz2err != BZ_OK) && (ebz2err != BZ_STREAM_END))) {
            goto bail;
        }

        /* Adjust pointers */
        newpos += ctrl[1];
        oldpos += ctrl[2];
    };

    /* Clean up the bzip2 reads */
    BZ2_bzReadClose(&cbz2err, cpfbz2);
    BZ2_bzReadClose(&dbz2err, dpfbz2);
    BZ2_bzReadClose(&ebz2err, epfbz2);
    if (fclose(cpf) || fclose(dpf) || fclose(epf)) {
        goto bail;
    }

    /* Write the new file */
    fd = open(tempfile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0666);
    if (fd < 0) {
        goto bail;
    }

    if (write(fd, new, newsize) != newsize) {
        goto bail;
    }

    if (close(fd) == -1) {
        goto bail;
    }

    free(new);
    free(old);

    return strdup(tempfile);

bail:
    remove(tempfile);

    if (new)
        free(new);
    if (old)
        free(old);
    if (tempfile)
        free(tempfile);

    return NULL;
}

enum patchouli_err patchouli_process(int argc, char **argv,
                                     const char *patch_file)
{
    int retval;
    FILE *patch = fopen(patch_file, "rb");
    if (argc > 1 && strcmp(argv[1], "-patchouli-stepone") == 0) {
        char *proper = argv[2];
        char *new_name = uniqid();
        if ((retval = rename(proper, new_name))) {
            goto err;
        }
        char *argv2[] =
            { new_name, (char *)"-patchouli-steptwo", argv[0], proper, NULL };
        execv(new_name, argv2);
        goto err;
    } else if (argc > 1 && 0 == strcmp(argv[1], "-patchouli-steptwo")) {
        DEBUG("step two, renaming the new binary to the original location");
        char *binary_name = argv[2];
        char *original_name = argv[3];
        if ((retval = rename(binary_name, original_name))) {
            goto err;
        }
        char *argv2[] =
            { original_name, (char *)"-patchouli-stepthree", argv[0], NULL };
        execv(original_name, argv2);
        goto err;
    } else if (argc > 1 && 0 == strcmp(argv[1], "-patchouli-stepthree")) {
        DEBUG("step three! removing renamed old binary %s", argv[2]);
        if ((retval = remove(argv[2]))) {
            goto err;
        }
        DEBUG("upgrade process done, new binary now loaded.. enjoy");
        return PATCHOULI_OK;
    } else if (patch != NULL) {
        fclose(patch);
        DEBUG("patch exists, starting the rename sequence");
        char *tempfile = apply_patch(patch_file, argv[0]);
        if (tempfile == NULL) {
            DEBUG("patch procedure messed up, bailing");
            goto err;
        } else {
            DEBUG("deleting patch, no more use");
            if ((retval = remove(patch_file))) {
                goto err;
            }
        }
        char *argv2[] = { NULL, (char *)"-patchouli-stepone", argv[0], NULL };
        argv2[0] = strdup(tempfile);
        execv(tempfile, argv2);
        DEBUG("error on execv: %s", strerror(errno));
        return PATCHOULI_ERR_EXEC;
    }
    DEBUG("no patch, running normally");
    return PATCHOULI_OK;

err:
    if (argc > 2) {
        remove(argv[2]);
    }
    if (argc > 3) {
        remove(argv[3]);
    }
    remove(patch_file);
    DEBUG("cleaned up, removed all temp files and patch");
    return PATCHOULI_ERR_FS;
}
