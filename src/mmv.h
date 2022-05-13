#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef u_int32_t Fnv32_t;

struct StrPairNode
{
    char *src;
    char *dest;
    struct StrPairNode *next;
};

struct StrPairNode *add_strpair_node(struct StrPairNode *cur_node, const char *new_src);
int attempt_strnode_map_insert(char *str, struct StrPairNode *map[], int map_size);
int get_fnv_32a_str_hash(char *str, int map_size);
void free_map(struct StrPairNode *map[], const int keyarr[], int keyarr_len);
void free_pair_ll(struct StrPairNode *node);
FILE *__attribute__((malloc)) get_tmp_path_fptr(char *tmp_path);
struct StrPairNode *init_pair_node(const char *src_str);
int map_find_src_pos(struct StrPairNode *map[], int hash, const char *str);
void open_file(char *path, const char *mode, FILE **fptr);
void open_tmp_file_in_editor(const char *path);
void read_new_names_from_tmp_file(char tmp_path[], struct StrPairNode *map[], const int keyarr[], int keyarr_len);
void rename_files(struct StrPairNode *map[], const int keyarr[], int keyarr_len);
void rename_path_pair(const char *src, const char *dest);
void rm_path(char *path);
void write_map_to_fptr(FILE *fptr, struct StrPairNode *map[], const int keys[], int num_keys);
void write_old_names_to_tmp_file(char tmp_path[], struct StrPairNode *map[], const int keyarr[], int keyarr_len);
