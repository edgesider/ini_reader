#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "../lib.h"

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

struct ini *
ini_init()
{
    struct ini *ini;

    ini = malloc(sizeof(struct ini));
    ini->unit_num = 0;
    ini->first_unit = NULL;
    ini->last_unit = NULL;
}

struct unit *
ini_new_unit(char *title)
{
    struct unit *unit;

    unit = malloc(sizeof(struct unit));
    unit->title = title;
    unit->item_num = 0;
    unit->first_item = NULL;
    unit->next_unit = NULL;
}

int
ini_add_unit(struct ini *ini, struct unit *unit)
{
    if (ini->last_unit == NULL) {
        ini->first_unit = ini->last_unit = unit;
    }
    else {
        ini->last_unit->next_unit = unit;
        ini->last_unit = unit;
    }
    return ++ini->unit_num;
}

struct item *
ini_new_item(char *key, char *value)
{
    struct item *item;

    item = malloc(sizeof(struct item));
    item->key = key;
    item->value = value;
    item->next_item = NULL;
    return item;
}

int
ini_add_item(struct unit *unit, struct item *item)
{
    if (unit->last_item == NULL) {
        unit->first_item = unit->last_item = item;
    }
    else {
        unit->last_item->next_item = item;
        unit->last_item = item;
    }

    return ++unit->item_num;
}

void
ini_foreach_unit(struct unit *unit, void (*func)(struct item *))
{
    struct item *item;

    item = unit->first_item;
    while (item != NULL) {
        func(item);
        item = item->next_item;
    }
}

void
ini_foreach(struct ini *ini, void (*func)(struct unit *))
{
    struct unit *unit;

    unit = ini->first_unit;;
    while (unit != NULL) {
        func(unit);
        unit = unit->next_unit;
    }
}

void
ini_print_item(struct item *item)
{
    char *key, *value;

    printf("ITEM:" "    ");
    key = item->key, value = item->value;
    for (int i = 0; i < strlen(key); i++) {
        if (key[i] == '=')
            printf("\\");
        printf("%c", key[i]);
    }
    printf("=");
    for (int i = 0; i < strlen(value); i++) {
        if (value[i] == '=')
            printf("\\");
        printf("%c", value[i]);
    }
    printf("\n");
}

void
ini_print_unit(struct unit *unit)
{
    char *title;

    printf("UNIT:" "[");
    title = unit->title;
    for (int i = 0; i < strlen(title); i++) {
        if (title[i] == '[' || title[i] == ']')
            printf("\\");
        printf("%c", title[i]);
    }
    printf("]\n");

    ini_foreach_unit(unit, ini_print_item);
}

enum line_type { LINE_TYPE_UNIT, LINE_TYPE_ITEM, LINE_TYPE_NONE };

enum line_type
ini_parse_line(const char *line_in, void **unit_or_item)
{
    const char *c, *end_c;
    char *title, *key, *value, *line;
    int line_len;
    struct unit *unit;
    struct item *item;

    c = line_in, end_c = line_in;
    for (;;) {
        if (*end_c != '\0' && *end_c != '\n') {
            end_c++;
            continue;
        }
        break;
    }

    // jump empty character
    while (c < end_c) {
        if (!isspace(*c))
            break;
        c++;
    }
    while (end_c > c) {
        if (!isspace(*c))
            break;
        end_c--;
    }

    if (c == end_c)
        return LINE_TYPE_NONE;

    line_len = end_c - c;
    line = malloc(line_len + 1);
    if (line == 0)
        errExit("malloc");
    for (int i = 0; i < line_len; i++)
        line[i] = c[i];
    line[line_len] = '\0';
    c = line, end_c = line + line_len;

    if (*c == '[') {
        // unit
        int idx = 0, got_escape = 0;
        c++;
        title = malloc(MAX_TITLE_LEN);
        for (; c < end_c; c++) {
            if (!got_escape) {
                if (*c == '\\') {
                    got_escape = 1;
                    continue;
                }
                else if (*c == ']')
                    break;
            }
            else {
                got_escape = 0;
            }
            title[idx++] = *c;
        }

        // not got ']'
        if (c == end_c) {
            dprintf(2, "unterminated title line: %s\n", line);
            exit(EXIT_FAILURE);
        }

        title[idx] = '\0';
        unit = ini_new_unit(title);
        *unit_or_item = unit;
        return LINE_TYPE_UNIT;
    }
    else {
        int idx = 0, got_escape = 0, got_equal_sign = 0;

        key = malloc(MAX_KEY_LEN);
        value = malloc(MAX_VALUE_LEN);
        key[0] = 0, value[0] = 0;
        for (idx = 0; c < end_c; c++) {
            if (!got_escape) {
                if (*c == '\\') {
                    got_escape = 1;
                    continue;
                }
                else if (*c == '=') {
                    got_equal_sign = 1;
                    break;
                }
            }
            else
                got_escape = 0;
            key[idx++] = *c;
        }
        while (idx-- > 0) {
            if (!isspace(key[idx]))
                break;
        }
        key[idx+1] = '\0';

        // parse value
        c++;
        while (c < end_c) {
            if (!isspace(*c))
                break;
            c++;
        }
        for (idx = 0; c < end_c; c++) {
            if (!got_escape) {
                if (*c == '\\') {
                    got_escape = 1;
                    continue;
                }
            }
            else
                got_escape = 0;
            value[idx++] = *c;
        }

        if (!got_equal_sign) {
            dprintf(2, "unrecognized format: %s\n", line);
            exit(EXIT_FAILURE);
        }

        item = ini_new_item(key, value);
        *unit_or_item = item;
        return LINE_TYPE_ITEM;
    }
}

void
test()
{
    struct ini *ini;
    struct unit *unit;
    struct item *item;

    union {
        struct unit *unit;
        struct item *item;
    } uori;

    ini = ini_init();
    unit = ini_new_unit("T=[ ]itle1");
    item = ini_new_item("key=1", "value1");
    ini_add_unit(ini, unit);
    ini_add_item(unit, item);
    item = ini_new_item("key2", "va=lue2");
    ini_add_item(unit, item);

    ini_foreach(ini, ini_print_unit);

    char *lines[] = {
        "abc=123",
        "[title1]",
        " abc = 321 ",
        " [ \\\\ title \\] ]",
        "  ",
        " \t ",
    };

    int linenum = sizeof(lines) / sizeof(char *);

    for (int i = 0; i < linenum; i++) {
        switch (ini_parse_line(lines[i], (void *)&uori)) {
            case LINE_TYPE_NONE:
                printf("NONE\n");
                break;
            case LINE_TYPE_UNIT:
                printf("UNIT: ");
                ini_print_unit(uori.unit);
                break;
            case LINE_TYPE_ITEM:
                printf("ITEM: ");
                ini_print_item(uori.item);
                break;
        }
    }
}
