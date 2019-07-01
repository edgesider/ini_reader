#ifndef _INI_H_
#define _INI_H_

#define MAX_TITLE_LEN   256
#define MAX_KEY_LEN     256
#define MAX_VALUE_LEN   256

struct unit;
struct item;
struct ini;

struct ini {
    int unit_num;
    struct unit *first_unit;
    struct unit *last_unit;
};

struct unit {
    char *title;
    int item_num;
    struct item *first_item;
    struct item *last_item;
    struct unit *next_unit;
};

struct item {
    char *key;
    char *value;
    struct item *next_item;
};

enum line_type { LINE_TYPE_UNIT, LINE_TYPE_ITEM, LINE_TYPE_NONE };

struct ini *ini_init();
struct unit *ini_new_unit(char *title);
int ini_add_unit(struct ini *ini, struct unit *unit);
struct item *ini_new_item(char *key, char *value);
int ini_add_item(struct unit *unit, struct item *item);

void ini_foreach_unit(struct unit *unit, void (*func)(struct item *));
void ini_foreach(struct ini *ini, void (*func)(struct unit *));

void ini_print_item(struct item *item);
void ini_print_unit(struct unit *unit);

enum line_type ini_parse_line(const char *line_in, void **unit_or_item);

#endif
