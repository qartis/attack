enum patchouli_err {
    PATCHOULI_OK,
    PATCHOULI_ERR_CURL,
    PATCHOULI_ERR_FS,
    PATCHOULI_ERR_EXEC,
    PATCHOULI_ERR_INIT,
};

struct patchouli_t {
    const char *url;
    const char *file;
    const char *http_post_vars;
    const char *patch_file;
    CURL *easy;
    int error;
};

enum patchouli_err patchouli_process(int argc, char **argv,
                                     const char *patch_file);
enum patchouli_err patchouli_init(struct patchouli_t *p);
enum patchouli_err patchouli_set_url(struct patchouli_t *p, const char *url);
enum patchouli_err patchouli_set_patch_file(struct patchouli_t *p,
                                            const char *file);
enum patchouli_err patchouli_request_patch(struct patchouli_t *p);
enum patchouli_err patchouli_set_vars(struct patchouli_t *p, const char *vars);
