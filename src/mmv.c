#include "./mmv.h"

int write_strarr_to_tmpfile(struct Set *set, char tmp_path_template[])
{
    int *i;

    FILE *tmp_fptr = open_tmpfile_fptr(tmp_path_template);
    if (tmp_fptr == NULL)
    {
        fprintf(stderr, "mmv: failed to open \"%s\": %s\n", tmp_path_template, strerror(errno));
        return errno;
    }

    for (i = set_begin(set); i < set_end(set); i = set_next(i))
        fprintf(tmp_fptr, "%s\n", *get_set_pos(set, i));

    fclose(tmp_fptr);

    return 0;
}

FILE *open_tmpfile_fptr(char *tmp_path)
{
    int tmp_fd = mkstemp(tmp_path);
    if (tmp_fd == -1)
    {
        perror("mmv: could not create a temporary file");
        return NULL;
    }

    return fdopen(tmp_fd, "w");
}

int edit_tmpfile(const char *path)
{
    int ret;
    char *editor_name = getenv("EDITOR");
    if (editor_name == NULL)
        editor_name = "nano";

    // provide space for "$EDITOR <path>\0", e.g. "nano test.txt\0"
    const size_t cmd_len = strlen(editor_name) + strlen(path) + 2;

    char *edit_cmd = malloc(sizeof(edit_cmd) * cmd_len);
    if (edit_cmd == NULL)
    {
        perror("mmv: failed to allocate memory for $EDITOR command string");
        return errno;
    }

    ret = snprintf(edit_cmd, cmd_len, "%s %s", editor_name, path);
    if (ret < 0 || ret > (int)cmd_len)
    {
        perror("mmv: couldn't create $EDITOR command string");
        free(edit_cmd);
        return errno;
    }

#if DEBUG == 0
    if (system(edit_cmd) != 0)
    {
        fprintf(stderr, "mmv: \'%s\' returned non-zero exit status: %d\n", editor_name, ret);
        free(edit_cmd);
        return errno;
    }
#endif

    free(edit_cmd);

    return 0;
}

struct Set *init_dest_set(unsigned int num_keys, char path[])
{
    // size of destination array only needs to be, at
    // maximum, the number of keys in the source set
    char **dest_arr = malloc(sizeof(char *) * num_keys);
    if (dest_arr == NULL)
        return NULL;

    int dest_size = 0;

    if (read_tmpfile_strs(dest_arr, &dest_size, num_keys, path) != 0)
    {
        free_strarr(dest_arr, dest_size);
        return NULL;
    }

    struct Set *set = set_init(false, dest_size, dest_arr, true);

    free_strarr(dest_arr, dest_size);

    return set;
}

int read_tmpfile_strs(char **dest_arr, int *dest_size, unsigned int num_keys, char path[])
{
    char cur_str[PATH_MAX], *read_ptr = "";
    size_t i = 0;

    FILE *tmp_fptr = fopen(path, "r");
    if (tmp_fptr == NULL)
    {
        fprintf(stderr, "mmv: failed to open \"%s\" in \"r\" mode: %s\n", path, strerror(errno));
        return errno;
    }

    while (read_ptr != NULL && i < num_keys)
    {
        read_ptr = fgets(cur_str, PATH_MAX, tmp_fptr);

        if (read_ptr != NULL && strcmp(cur_str, "\n") != 0)
        {
            cur_str[strlen(cur_str) - 1] = '\0';

            cpy_str_to_arr(&dest_arr[(*dest_size)], cur_str);
            (*dest_size)++;

            i++;
        }
    }

    fclose(tmp_fptr);

    return 0;
}

void free_strarr(char **arr, int arr_size)
{
    for (int i = 0; i < arr_size; i++)
        free(arr[i]);

    free(arr);
}

int rename_paths(struct Set *src_set, struct Set *dest_set, struct Opts *opts)
{
    int *i, *j;
    char *src_str, *dest_str;


    for (i = set_begin(src_set), j = set_begin(dest_set); i < set_end(src_set) && j < set_end(dest_set);
         i = set_next(i), j = set_next(j))
    {
        src_str  = *get_set_pos(src_set, i);
        dest_str = *get_set_pos(dest_set, j);

        if (!is_invalid_key(j))
            rename_path(src_str, dest_str, opts);

        else
            fprintf(stderr, "mmv: duplicate dest found for src '%s'. No mv conducted.\n", src_str);
    }

    return 0;
}

void rename_path(const char *src, const char *dest, struct Opts *opts)
{
    if (strcmp(src, dest) == 0)
        return;

    if (rename(src, dest) == -1)
        fprintf(stderr, "mmv: \'%s\' to \'%s\': %s\n", src, dest, strerror(errno));

    else if (opts->verbose)
        printf("  '%s' -> '%s'\n", src, dest);
}

void rm_path(char *path)
{
    if (remove(path) == -1)
        fprintf(stderr, "mmv: failed to delete \'%s\': %s\n", path, strerror(errno));
}

int rm_cycles(struct Set *src_set, struct Set *dest_set, struct Opts *opts)
{
    int *i, *j, is_dupe;
    unsigned long int u_key;
    char *src_str, *dest_str;
    char **cur_src_pos;

    for (i = set_begin(src_set), j = set_begin(dest_set); i < set_end(src_set) && j < set_end(dest_set);
         i = set_next(i), j = set_next(j))
    {
        src_str  = *get_set_pos(src_set, i);
        dest_str = *get_set_pos(dest_set, j);

        if (!is_invalid_key(j) && strcmp(src_str, dest_str) != 0)
        {
            u_key           = (unsigned int)*j;
            is_dupe         = is_duplicate_element(dest_str, src_set, &u_key);
            char tmp_path[] = "mmv_cycle_XXXXXX";

            if (is_dupe == 0)
            {
                // create temporary name using the current name
                int tmp_fd = mkstemp(tmp_path);
                if (tmp_fd == -1)
                {
                    perror("mmv: could not create a temporary file");
                    return -1;
                }

                cur_src_pos = get_set_pos(src_set, j);

                // rename to temporary name
                rename_path(*cur_src_pos, tmp_path, opts);

                // update str in src map to temp_str
                free(*cur_src_pos);
                cpy_str_to_arr(cur_src_pos, tmp_path);
            }
        }
    }

    return 0;
}
