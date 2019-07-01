#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../lib.h"
#include "ini.h"

#define READ_BUFSIZE 2048

static struct ini *ini;
static struct unit *curr_unit;

void
process_line(int lineno, const char *line)
{
    union {
        struct unit *unit;
        struct item *item;
    } uori;

    switch (ini_parse_line(line, (void *)&uori)) {
        case LINE_TYPE_NONE:
            /*printf("NONE\n");*/
            break;
        case LINE_TYPE_UNIT:
            /*printf("UNIT\n");*/
            curr_unit = uori.unit;
            ini_add_unit(ini, curr_unit);
            break;
        case LINE_TYPE_ITEM:
            /*printf("ITEM\n");*/
            ini_add_item(curr_unit, uori.item);
            break;
    }
}

void
iterate_line(int fd, void (*func)(int lineno, const char *line))
{
    int cnt, lineno;
    char *c, *buf_end, buf[READ_BUFSIZE + 1];
    char *avail_buf_start, *line_start, *next_line_start;

    myassert(fd >= 0, "fd");
    avail_buf_start = buf;
    lineno = 0;

    for (;;) {
read:
        cnt = read(fd, avail_buf_start, READ_BUFSIZE - (avail_buf_start - buf));
        if (cnt == -1) {
            if (errno == EINTR)
                goto read;
            else
                errExit("read");
        }
        if (cnt == 0) {
            // When reach end of file, there may be remaining data in the buffer.
            if (avail_buf_start > buf) {
                *avail_buf_start = '\n';
                func(++lineno, buf);
            }
            return ;
        }

        buf_end = avail_buf_start + cnt;
        next_line_start = buf;

        // iterate each line, and put incomplete line to start of buffer.
        for (;;) {
            line_start = next_line_start;

            // try to find a complete line in buffer
            c = line_start;
            for (;;) {
                if (c >= buf_end) {
                    // remaining data is not a complete line

                    if (buf == line_start)
                        fatal("line %d is too long\n", lineno + 1);

                    // copy remaining data to start of buffer
                    c = line_start, avail_buf_start = buf;
                    while (c < buf_end) {
                        *avail_buf_start++ = *c++;
                    }
                    goto read;
                }
                if (*c == '\n') {
                    lineno++;
                    next_line_start = c + 1;
                    break;
                }
                c++;
            }

            // got a complete line
            func(lineno, line_start);
        }
    }
}

int
main(int argc, char *argv[])
{
    int fd;

    if (argc <= 1) {
        dprintf(2, "Usage: %s [filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ini = ini_init();

    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
        errExit("open");

    iterate_line(fd, process_line);
    ini_foreach(ini, ini_print_unit);
}
