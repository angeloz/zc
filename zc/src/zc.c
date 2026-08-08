/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026, zc contributors */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if defined(__COSMOPOLITAN__)
#define ZC_BUNDLED_KILO_NAME      "zc-kilo.com"
#define ZC_FORCE_BUNDLED_EDITOR   1
#else
#define ZC_BUNDLED_KILO_NAME      "zc-kilo"
#define ZC_FORCE_BUNDLED_EDITOR   0
#endif

#define ZC_VERSION "0.1.0"
#define STATUS_LEN 256
#define PROMPT_LEN PATH_MAX
#define INPUT_CHUNK 32
#define MIN_SCREEN_ROWS 8
#define MIN_SCREEN_COLS 40
#define CTRL_KEY(k) ((k) & 0x1f)

enum
{
    KEY_NULL = 0,
    KEY_CTRL_L = 12,
    KEY_ENTER = 13,
    KEY_ESCAPE = 27,
    KEY_BACKSPACE = 127,
    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10
};

typedef struct
{
    char *data;
    size_t len;
    size_t cap;
} AppendBuffer;

typedef struct
{
    char *name;
    struct stat st;
    bool is_dir;
    bool is_link;
    bool marked;
} Entry;

typedef struct
{
    char cwd[PATH_MAX];
    Entry *entries;
    size_t len;
    size_t cap;
    size_t selected;
    size_t scroll;
} Panel;

typedef struct
{
    struct termios original_termios;
    bool raw_mode;
    int screen_rows;
    int screen_cols;
    int active_panel;
    bool running;
    char status[STATUS_LEN];
    char kilo_cmd[PATH_MAX];
    Panel panels[2];
} App;

static App g_app;

static bool join_path (char *dst, size_t dst_size, const char *dir, const char *name);
static bool resolve_existing_path (char *dst, size_t dst_size, const char *path);
static bool resolve_executable_dir (const char *argv0, char *dst, size_t dst_size);
static void panel_move_selection (Panel *panel, int delta);

static void
die (const char *message)
{
    write (STDOUT_FILENO, "\x1b[2J\x1b[H\x1b[?25h", 16);
    perror (message);
    exit (EXIT_FAILURE);
}

static void
ab_init (AppendBuffer *ab)
{
    ab->data = NULL;
    ab->len = 0;
    ab->cap = 0;
}

static void
ab_free (AppendBuffer *ab)
{
    free (ab->data);
    ab->data = NULL;
    ab->len = 0;
    ab->cap = 0;
}

static void
ab_reserve (AppendBuffer *ab, size_t extra)
{
    size_t needed;
    char *new_data;

    needed = ab->len + extra + 1;
    if (needed <= ab->cap)
        return;

    if (ab->cap == 0)
        ab->cap = 256;
    while (ab->cap < needed)
        ab->cap *= 2;

    new_data = realloc (ab->data, ab->cap);
    if (new_data == NULL)
        die ("realloc");
    ab->data = new_data;
}

static void
ab_append (AppendBuffer *ab, const char *s, size_t len)
{
    ab_reserve (ab, len);
    memcpy (ab->data + ab->len, s, len);
    ab->len += len;
    ab->data[ab->len] = '\0';
}

static void
ab_append_cstr (AppendBuffer *ab, const char *s)
{
    ab_append (ab, s, strlen (s));
}

static void
ab_appendf (AppendBuffer *ab, const char *fmt, ...)
{
    va_list ap;
    va_list ap_copy;
    int needed;

    va_start (ap, fmt);
    va_copy (ap_copy, ap);
    needed = vsnprintf (NULL, 0, fmt, ap_copy);
    va_end (ap_copy);

    if (needed < 0)
        die ("vsnprintf");

    ab_reserve (ab, (size_t) needed);
    vsnprintf (ab->data + ab->len, ab->cap - ab->len, fmt, ap);
    va_end (ap);
    ab->len += (size_t) needed;
}

static void
set_status (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (g_app.status, sizeof (g_app.status), fmt, ap);
    va_end (ap);
}

static int
read_byte (void)
{
    char c;
    ssize_t nread;

    while ((nread = read (STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
            die ("read");
    }

    return (unsigned char) c;
}

static int
read_key (void)
{
    int c;
    char seq[INPUT_CHUNK];

    c = read_byte ();
    if (c != KEY_ESCAPE)
        return c;

    if (read (STDIN_FILENO, &seq[0], 1) != 1)
        return KEY_ESCAPE;
    if (read (STDIN_FILENO, &seq[1], 1) != 1)
        return KEY_ESCAPE;

    if (seq[0] == '[')
    {
        if (seq[1] >= '0' && seq[1] <= '9')
        {
            int i;

            i = 2;
            while (i < INPUT_CHUNK - 1)
            {
                if (read (STDIN_FILENO, &seq[i], 1) != 1)
                    return KEY_ESCAPE;
                if (seq[i] == '~')
                    break;
                i++;
            }

            if (seq[1] == '1' && seq[2] == '~')
                return KEY_HOME;
            if (seq[1] == '3' && seq[2] == '~')
                return KEY_DELETE;
            if (seq[1] == '4' && seq[2] == '~')
                return KEY_END;
            if (seq[1] == '5' && seq[2] == '~')
                return KEY_PAGE_UP;
            if (seq[1] == '6' && seq[2] == '~')
                return KEY_PAGE_DOWN;
            if (seq[1] == '1' && seq[2] == '1' && seq[3] == '~')
                return KEY_F1;
            if (seq[1] == '1' && seq[2] == '2' && seq[3] == '~')
                return KEY_F2;
            if (seq[1] == '1' && seq[2] == '5' && seq[3] == '~')
                return KEY_F5;
            if (seq[1] == '1' && seq[2] == '7' && seq[3] == '~')
                return KEY_F6;
            if (seq[1] == '1' && seq[2] == '8' && seq[3] == '~')
                return KEY_F7;
            if (seq[1] == '1' && seq[2] == '9' && seq[3] == '~')
                return KEY_F8;
            if (seq[1] == '2' && seq[2] == '0' && seq[3] == '~')
                return KEY_F9;
            if (seq[1] == '2' && seq[2] == '1' && seq[3] == '~')
                return KEY_F10;
            return KEY_ESCAPE;
        }

        switch (seq[1])
        {
        case 'A':
            return KEY_ARROW_UP;
        case 'B':
            return KEY_ARROW_DOWN;
        case 'C':
            return KEY_ARROW_RIGHT;
        case 'D':
            return KEY_ARROW_LEFT;
        case 'H':
            return KEY_HOME;
        case 'F':
            return KEY_END;
        default:
            return KEY_ESCAPE;
        }
    }

    if (seq[0] == 'O')
    {
        switch (seq[1])
        {
        case 'P':
            return KEY_F1;
        case 'Q':
            return KEY_F2;
        case 'R':
            return KEY_F3;
        case 'S':
            return KEY_F4;
        default:
            return KEY_ESCAPE;
        }
    }

    return KEY_ESCAPE;
}

static void
disable_raw_mode (void)
{
    if (!g_app.raw_mode)
        return;

    if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &g_app.original_termios) == -1)
        die ("tcsetattr");
    g_app.raw_mode = false;
}

static void
enable_raw_mode (void)
{
    struct termios raw;

    if (tcgetattr (STDIN_FILENO, &g_app.original_termios) == -1)
        die ("tcgetattr");

    raw = g_app.original_termios;
    raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cflag |= (tcflag_t) (CS8);
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die ("tcsetattr");

    g_app.raw_mode = true;
}

static void
clear_screen (void)
{
    write (STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
}

static void
refresh_size (void)
{
    struct winsize ws;

    if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        g_app.screen_rows = 24;
        g_app.screen_cols = 80;
        return;
    }

    g_app.screen_rows = ws.ws_row;
    g_app.screen_cols = ws.ws_col;
}

static void
free_entries (Panel *panel)
{
    size_t i;

    for (i = 0; i < panel->len; i++)
        free (panel->entries[i].name);
    free (panel->entries);
    panel->entries = NULL;
    panel->len = 0;
    panel->cap = 0;
    panel->selected = 0;
    panel->scroll = 0;
}

static void
panel_reserve (Panel *panel)
{
    Entry *entries;
    size_t new_cap;

    if (panel->len < panel->cap)
        return;

    new_cap = panel->cap == 0 ? 64 : panel->cap * 2;
    entries = realloc (panel->entries, sizeof (*entries) * new_cap);
    if (entries == NULL)
        die ("realloc");

    panel->entries = entries;
    panel->cap = new_cap;
}

static int
entry_compare (const void *left, const void *right)
{
    const Entry *a;
    const Entry *b;
    int name_cmp;

    a = left;
    b = right;

    if (strcmp (a->name, "..") == 0 && strcmp (b->name, "..") == 0)
        return 0;
    if (strcmp (a->name, "..") == 0)
        return -1;
    if (strcmp (b->name, "..") == 0)
        return 1;

    if (a->is_dir != b->is_dir)
        return a->is_dir ? -1 : 1;

    name_cmp = strcasecmp (a->name, b->name);
    if (name_cmp != 0)
        return name_cmp;

    return strcmp (a->name, b->name);
}

static bool
copy_string (char *dst, size_t dst_size, const char *src)
{
    int rc;

    rc = snprintf (dst, dst_size, "%s", src);
    return rc >= 0 && (size_t) rc < dst_size;
}

static bool
compute_sibling_path (const char *argv0, const char *binary_name, char *dst, size_t dst_size)
{
    char exec_dir[PATH_MAX];

    return resolve_executable_dir (argv0, exec_dir, sizeof (exec_dir))
           && join_path (dst, dst_size, exec_dir, binary_name);
}

static bool
find_executable_in_path (const char *name, char *dst, size_t dst_size)
{
    const char *path_env;
    const char *segment;

    if (strchr (name, '/') != NULL)
        return resolve_existing_path (dst, dst_size, name);

    path_env = getenv ("PATH");
    if (path_env == NULL || path_env[0] == '\0')
        return false;

    segment = path_env;
    while (*segment != '\0')
    {
        const char *end = strchr (segment, ':');
        char candidate[PATH_MAX];
        size_t len;

        if (end == NULL)
            end = segment + strlen (segment);
        len = (size_t) (end - segment);

        if (len == 0)
        {
            if (snprintf (candidate, sizeof (candidate), "./%s", name) >= 0
                && access (candidate, X_OK) == 0
                && copy_string (dst, dst_size, candidate))
                return true;
        }
        else if (len < sizeof (candidate))
        {
            int rc;

            memcpy (candidate, segment, len);
            candidate[len] = '\0';
            rc = snprintf (candidate, sizeof (candidate), "%.*s/%s", (int) len, segment, name);
            if (rc >= 0 && (size_t) rc < sizeof (candidate) && access (candidate, X_OK) == 0
                && copy_string (dst, dst_size, candidate))
                return true;
        }

        segment = *end == ':' ? end + 1 : end;
    }

    return false;
}

static bool
resolve_executable_dir (const char *argv0, char *dst, size_t dst_size)
{
    char resolved[PATH_MAX];
    char *slash;

    if (argv0 == NULL || argv0[0] == '\0')
        return false;

    if (strchr (argv0, '/') != NULL)
    {
        if (!resolve_existing_path (resolved, sizeof (resolved), argv0))
            return false;
    }
    else if (!find_executable_in_path (argv0, resolved, sizeof (resolved)))
    {
        return false;
    }

    slash = strrchr (resolved, '/');
    if (slash == NULL)
        return false;
    if (slash == resolved)
        slash[1] = '\0';
    else
        *slash = '\0';

    return copy_string (dst, dst_size, resolved);
}

static bool
discover_default_kilo (const char *argv0, char *dst, size_t dst_size)
{
    char sibling[PATH_MAX];

    if (compute_sibling_path (argv0, ZC_BUNDLED_KILO_NAME, sibling, sizeof (sibling)))
    {
        if (ZC_FORCE_BUNDLED_EDITOR)
            return copy_string (dst, dst_size, sibling);
        if (access (sibling, X_OK) == 0 && copy_string (dst, dst_size, sibling))
            return true;
    }

    if (ZC_FORCE_BUNDLED_EDITOR)
        return copy_string (dst, dst_size, ZC_BUNDLED_KILO_NAME);
    return copy_string (dst, dst_size, "kilo");
}

static bool
join_path (char *dst, size_t dst_size, const char *dir, const char *name)
{
    int rc;

    if (strcmp (dir, "/") == 0)
        rc = snprintf (dst, dst_size, "/%s", name);
    else
        rc = snprintf (dst, dst_size, "%s/%s", dir, name);

    return rc >= 0 && (size_t) rc < dst_size;
}

static bool
parent_path (char *dst, size_t dst_size, const char *path)
{
    char tmp[PATH_MAX];
    char *slash;

    if (!copy_string (tmp, sizeof (tmp), path))
        return false;

    if (strcmp (tmp, "/") == 0)
        return copy_string (dst, dst_size, "/");

    slash = strrchr (tmp, '/');
    if (slash == NULL)
        return copy_string (dst, dst_size, ".");
    if (slash == tmp)
        return copy_string (dst, dst_size, "/");

    *slash = '\0';
    return copy_string (dst, dst_size, tmp);
}

static bool
resolve_existing_path (char *dst, size_t dst_size, const char *path)
{
    char *resolved;

    resolved = realpath (path, NULL);
    if (resolved == NULL)
        return false;

    if (!copy_string (dst, dst_size, resolved))
    {
        free (resolved);
        errno = ENAMETOOLONG;
        return false;
    }

    free (resolved);
    return true;
}

static bool
panel_push_entry (Panel *panel, const char *path, const char *name)
{
    Entry *entry;
    char full_path[PATH_MAX];

    panel_reserve (panel);
    entry = &panel->entries[panel->len];
    memset (entry, 0, sizeof (*entry));

    entry->name = strdup (name);
    if (entry->name == NULL)
        die ("strdup");

    if (strcmp (name, "..") == 0)
    {
        entry->is_dir = true;
    }
    else
    {
        if (!join_path (full_path, sizeof (full_path), path, name))
        {
            free (entry->name);
            errno = ENAMETOOLONG;
            return false;
        }
        if (lstat (full_path, &entry->st) == -1)
        {
            free (entry->name);
            return false;
        }
        entry->is_dir = S_ISDIR (entry->st.st_mode);
        entry->is_link = S_ISLNK (entry->st.st_mode);
    }

    panel->len++;
    return true;
}

static bool
panel_entry_was_marked (const Panel *panel, const char *name)
{
    size_t i;

    for (i = 0; i < panel->len; i++)
    {
        if (panel->entries[i].marked && strcmp (panel->entries[i].name, name) == 0)
            return true;
    }

    return false;
}

static bool
panel_reload (Panel *panel)
{
    Panel updated;
    DIR *dir;
    struct dirent *direntp;

    memset (&updated, 0, sizeof (updated));
    if (!copy_string (updated.cwd, sizeof (updated.cwd), panel->cwd))
    {
        set_status ("Path too long: %s", panel->cwd);
        return false;
    }

    dir = opendir (panel->cwd);
    if (dir == NULL)
    {
        set_status ("Cannot open %s: %s", panel->cwd, strerror (errno));
        return false;
    }

    if (strcmp (panel->cwd, "/") != 0)
        panel_push_entry (&updated, panel->cwd, "..");

    while ((direntp = readdir (dir)) != NULL)
    {
        if (strcmp (direntp->d_name, ".") == 0 || strcmp (direntp->d_name, "..") == 0)
            continue;
        if (!panel_push_entry (&updated, panel->cwd, direntp->d_name))
            set_status ("Skipping entry in %s: %s", panel->cwd, strerror (errno));
    }

    closedir (dir);
    qsort (updated.entries, updated.len, sizeof (*updated.entries), entry_compare);
    for (size_t i = 0; i < updated.len; i++)
        updated.entries[i].marked = panel_entry_was_marked (panel, updated.entries[i].name);
    updated.selected = panel->selected < updated.len ? panel->selected : 0;
    updated.scroll = 0;

    free_entries (panel);
    *panel = updated;
    return true;
}

static bool
panel_load_path (Panel *panel, const char *path)
{
    char resolved[PATH_MAX];

    if (!resolve_existing_path (resolved, sizeof (resolved), path))
    {
        set_status ("Cannot resolve %s: %s", path, strerror (errno));
        return false;
    }

    if (!copy_string (panel->cwd, sizeof (panel->cwd), resolved))
    {
        set_status ("Path too long: %s", resolved);
        return false;
    }

    return panel_reload (panel);
}

static Entry *
panel_selected_entry (Panel *panel)
{
    if (panel->len == 0 || panel->selected >= panel->len)
        return NULL;
    return &panel->entries[panel->selected];
}

static size_t
panel_marked_count (const Panel *panel)
{
    size_t count = 0;

    for (size_t i = 0; i < panel->len; i++)
    {
        if (panel->entries[i].marked && strcmp (panel->entries[i].name, "..") != 0)
            count++;
    }

    return count;
}

static void
panel_clear_marks (Panel *panel)
{
    for (size_t i = 0; i < panel->len; i++)
        panel->entries[i].marked = false;
}

static void
panel_toggle_selected_mark (Panel *panel)
{
    Entry *entry = panel_selected_entry (panel);
    size_t marked_count;

    if (entry == NULL || strcmp (entry->name, "..") == 0)
    {
        set_status ("Cannot mark this entry");
        return;
    }

    entry->marked = !entry->marked;
    marked_count = panel_marked_count (panel);
    set_status ("%zu item%s marked", marked_count, marked_count == 1 ? "" : "s");

    if (panel->selected + 1 < panel->len)
        panel_move_selection (panel, 1);
}

static void
panel_toggle_all_marks (Panel *panel)
{
    bool should_mark = false;
    size_t marked_count;

    for (size_t i = 0; i < panel->len; i++)
    {
        if (strcmp (panel->entries[i].name, "..") != 0 && !panel->entries[i].marked)
        {
            should_mark = true;
            break;
        }
    }

    for (size_t i = 0; i < panel->len; i++)
    {
        if (strcmp (panel->entries[i].name, "..") != 0)
            panel->entries[i].marked = should_mark;
    }

    marked_count = panel_marked_count (panel);
    set_status ("%zu item%s marked", marked_count, marked_count == 1 ? "" : "s");
}

static size_t
panel_collect_target_indexes (const Panel *panel, size_t **out_indexes)
{
    size_t count;
    size_t *indexes;

    *out_indexes = NULL;
    count = panel_marked_count (panel);
    if (count == 0)
    {
        if (panel->len == 0 || panel->selected >= panel->len
            || strcmp (panel->entries[panel->selected].name, "..") == 0)
            return 0;

        indexes = malloc (sizeof (*indexes));
        if (indexes == NULL)
            die ("malloc");
        indexes[0] = panel->selected;
        *out_indexes = indexes;
        return 1;
    }

    indexes = malloc (sizeof (*indexes) * count);
    if (indexes == NULL)
        die ("malloc");

    count = 0;
    for (size_t i = 0; i < panel->len; i++)
    {
        if (panel->entries[i].marked && strcmp (panel->entries[i].name, "..") != 0)
            indexes[count++] = i;
    }

    *out_indexes = indexes;
    return count;
}

static bool
entry_full_path (const Panel *panel, const Entry *entry, char *dst, size_t dst_size)
{
    if (entry == NULL)
        return false;
    if (strcmp (entry->name, "..") == 0)
        return parent_path (dst, dst_size, panel->cwd);
    return join_path (dst, dst_size, panel->cwd, entry->name);
}

static void
panel_adjust_scroll (Panel *panel)
{
    size_t visible_rows;

    visible_rows = (size_t) (g_app.screen_rows - 4);
    if (visible_rows < 1)
        visible_rows = 1;

    if (panel->selected < panel->scroll)
        panel->scroll = panel->selected;
    else if (panel->selected >= panel->scroll + visible_rows)
        panel->scroll = panel->selected - visible_rows + 1;
}

static void
panel_move_selection (Panel *panel, int delta)
{
    if (panel->len == 0)
        return;

    if (delta < 0)
    {
        size_t amount = (size_t) (-delta);
        if (amount > panel->selected)
            panel->selected = 0;
        else
            panel->selected -= amount;
    }
    else
    {
        size_t amount = (size_t) delta;
        size_t max_index = panel->len - 1;

        if (panel->selected + amount > max_index)
            panel->selected = max_index;
        else
            panel->selected += amount;
    }

    panel_adjust_scroll (panel);
}

static void
format_size (char *dst, size_t dst_size, const Entry *entry)
{
    static const char suffixes[] = { 'B', 'K', 'M', 'G', 'T' };
    double value;
    size_t idx;

    if (strcmp (entry->name, "..") == 0 || entry->is_dir)
    {
        snprintf (dst, dst_size, "<DIR>");
        return;
    }

    value = (double) entry->st.st_size;
    idx = 0;
    while (value >= 1024.0 && idx + 1 < sizeof (suffixes))
    {
        value /= 1024.0;
        idx++;
    }

    if (idx == 0)
        snprintf (dst, dst_size, "%.0f%c", value, suffixes[idx]);
    else
        snprintf (dst, dst_size, "%.1f%c", value, suffixes[idx]);
}

static void
truncate_name (char *dst, size_t dst_size, const char *src, bool is_dir)
{
    size_t src_len;
    size_t needed;

    src_len = strlen (src);
    needed = src_len + (is_dir ? 1U : 0U);

    if (dst_size == 0)
        return;

    if (needed + 1 <= dst_size)
    {
        copy_string (dst, dst_size, src);
        if (is_dir)
        {
            dst[src_len] = '/';
            dst[src_len + 1] = '\0';
        }
        return;
    }

    if (dst_size <= 4)
    {
        snprintf (dst, dst_size, "%.*s", (int) (dst_size - 1), src);
        return;
    }

    snprintf (dst, dst_size, "%.*s~%s", (int) (dst_size - (is_dir ? 3 : 2)), src, is_dir ? "/" : "");
}

static void
draw_panel (AppendBuffer *ab, const Panel *panel, int x, int width, bool active)
{
    int body_rows;
    int row;
    char title[PATH_MAX + 16];

    body_rows = g_app.screen_rows - 4;
    if (body_rows < 1)
        body_rows = 1;

    ab_appendf (ab, "\x1b[%d;%dH", 1, x + 1);
    if (active)
        ab_append_cstr (ab, "\x1b[7m");
    snprintf (title, sizeof (title), " %s ", panel->cwd);
    if ((int) strlen (title) > width)
        title[width] = '\0';
    ab_appendf (ab, "%-*s", width, title);
    ab_append_cstr (ab, "\x1b[m");

    for (row = 0; row < body_rows; row++)
    {
        size_t idx;

        ab_appendf (ab, "\x1b[%d;%dH", row + 2, x + 1);
        idx = panel->scroll + (size_t) row;
        if (idx < panel->len)
        {
            const Entry *entry = &panel->entries[idx];
            char name[PATH_MAX];
            char sizebuf[32];
            int name_width;
            char mark;

            format_size (sizebuf, sizeof (sizebuf), entry);
            name_width = width - (int) strlen (sizebuf) - 5;
            if (name_width < 1)
                name_width = 1;
            truncate_name (name, sizeof (name), entry->name, entry->is_dir);
            mark = entry->marked ? '*' : ' ';

            if (active && idx == panel->selected)
                ab_append_cstr (ab, "\x1b[7m");
            else if (!active && idx == panel->selected)
                ab_append_cstr (ab, "\x1b[2m");

            ab_appendf (ab, "%c %-*.*s %*s", mark, name_width, name_width, name,
                        (int) strlen (sizebuf), sizebuf);

            if (active || idx == panel->selected)
                ab_append_cstr (ab, "\x1b[m");
        }
        else
        {
            ab_appendf (ab, "%-*s", width, "");
        }
    }
}

static void
draw_screen (const char *prompt_label, const char *prompt_value)
{
    AppendBuffer ab;
    char bottom_line[PATH_MAX + 128];
    int half;
    int prompt_row;
    int cursor_col;

    refresh_size ();
    if (g_app.screen_rows < MIN_SCREEN_ROWS || g_app.screen_cols < MIN_SCREEN_COLS)
    {
        clear_screen ();
        printf ("Terminal too small. Need at least %dx%d.\n", MIN_SCREEN_COLS, MIN_SCREEN_ROWS);
        fflush (stdout);
        return;
    }

    ab_init (&ab);
    ab_append_cstr (&ab, "\x1b[?25l");
    ab_append_cstr (&ab, "\x1b[H");

    half = g_app.screen_cols / 2;
    draw_panel (&ab, &g_app.panels[0], 0, half - 1, g_app.active_panel == 0);
    draw_panel (&ab, &g_app.panels[1], half, g_app.screen_cols - half, g_app.active_panel == 1);

    ab_appendf (&ab, "\x1b[%d;%dH|\x1b[%d;1H", 1, half, g_app.screen_rows - 1);
    ab_append_cstr (&ab, "\x1b[K");
    ab_appendf (&ab, "%-*.*s", g_app.screen_cols, g_app.screen_cols, g_app.status);
    ab_appendf (&ab, "\x1b[%d;1H", g_app.screen_rows);
    ab_append_cstr (&ab, "\x1b[K");
    if (prompt_label != NULL)
    {
        snprintf (bottom_line, sizeof (bottom_line), "%s%s", prompt_label,
                  prompt_value != NULL ? prompt_value : "");
        ab_appendf (&ab, "%.*s", g_app.screen_cols, bottom_line);
    }
    else
    {
        snprintf (bottom_line, sizeof (bottom_line),
                  "[F1] Help  [Ctrl-N] NewFile  [Space] Mark  [Tab] Switch  [F3/F4] View/Edit  "
                  "[F5/F6] Copy/Move  [F7] Mkdir  [F8] Delete  [F10/Ctrl-Q] Quit");
        ab_appendf (&ab, "%.*s", g_app.screen_cols, bottom_line);
    }

    if (prompt_label != NULL)
    {
        prompt_row = g_app.screen_rows;
        cursor_col = (int) strlen (prompt_label) + (prompt_value != NULL ? (int) strlen (prompt_value) : 0) + 1;
        if (cursor_col > g_app.screen_cols)
            cursor_col = g_app.screen_cols;
        ab_appendf (&ab, "\x1b[%d;%dH\x1b[?25h", prompt_row, cursor_col);
    }
    else
    {
        ab_append_cstr (&ab, "\x1b[?25h");
    }

    write (STDOUT_FILENO, ab.data, ab.len);
    ab_free (&ab);
}

static bool
prompt_input (const char *label, const char *initial, char *out, size_t out_size)
{
    size_t len;

    if (out_size == 0)
        return false;

    if (initial == NULL)
        initial = "";
    if (!copy_string (out, out_size, initial))
        return false;

    len = strlen (out);
    while (true)
    {
        int key;

        draw_screen (label, out);
        key = read_key ();

        if (key == KEY_ENTER)
            return true;
        if (key == KEY_ESCAPE)
            return false;
        if (key == KEY_BACKSPACE || key == KEY_DELETE || key == CTRL_KEY ('h'))
        {
            if (len > 0)
                out[--len] = '\0';
            continue;
        }

        if (key >= 32 && key < 127 && len + 1 < out_size)
        {
            out[len++] = (char) key;
            out[len] = '\0';
        }
    }
}

static bool
prompt_confirm (const char *question)
{
    while (true)
    {
        int key;

        draw_screen (question, " [y/N] ");
        key = read_key ();
        if (key == 'y' || key == 'Y')
            return true;
        if (key == 'n' || key == 'N' || key == KEY_ESCAPE || key == KEY_ENTER)
            return false;
    }
}

static void
show_help_screen (void)
{
    AppendBuffer ab;
    int row;

    refresh_size ();
    ab_init (&ab);
    ab_append_cstr (&ab, "\x1b[2J\x1b[H\x1b[?25l");

    row = 1;
    ab_appendf (&ab, "\x1b[%d;1Hzc keymap", row++);
    row++;
    ab_appendf (&ab, "\x1b[%d;1HF1         Help", row++);
    ab_appendf (&ab, "\x1b[%d;1HEnter      Open file or enter directory", row++);
    ab_appendf (&ab, "\x1b[%d;1HF3         View with zc-kilo --readonly", row++);
    ab_appendf (&ab, "\x1b[%d;1HF4         Edit with zc-kilo", row++);
    ab_appendf (&ab, "\x1b[%d;1HF5         Copy", row++);
    ab_appendf (&ab, "\x1b[%d;1HF6         Move", row++);
    ab_appendf (&ab, "\x1b[%d;1HF7         Create directory", row++);
    ab_appendf (&ab, "\x1b[%d;1HF8         Delete", row++);
    ab_appendf (&ab, "\x1b[%d;1HF10        Quit", row++);
    ab_appendf (&ab, "\x1b[%d;1HCtrl-N     Create empty file", row++);
    ab_appendf (&ab, "\x1b[%d;1HCtrl-Q     Quit", row++);
    ab_appendf (&ab, "\x1b[%d;1HSpace      Mark/unmark current entry", row++);
    ab_appendf (&ab, "\x1b[%d;1H*          Mark/unmark all entries", row++);
    ab_appendf (&ab, "\x1b[%d;1HTab        Switch panel", row++);
    ab_appendf (&ab, "\x1b[%d;1Hr          Refresh", row++);
    ab_appendf (&ab, "\x1b[%d;1Hn          Rename single selected item", row++);
    row++;
    ab_appendf (&ab, "\x1b[%d;1HPress any key to return", row);
    ab_append_cstr (&ab, "\x1b[?25h");

    write (STDOUT_FILENO, ab.data, ab.len);
    ab_free (&ab);
    (void) read_key ();
    set_status ("Help closed");
}

static int
copy_file_data (int in_fd, int out_fd)
{
    char buffer[8192];
    ssize_t bytes;

    while ((bytes = read (in_fd, buffer, sizeof (buffer))) > 0)
    {
        ssize_t offset = 0;

        while (offset < bytes)
        {
            ssize_t written = write (out_fd, buffer + offset, (size_t) (bytes - offset));
            if (written < 0)
                return -1;
            offset += written;
        }
    }

    return bytes < 0 ? -1 : 0;
}

static int
copy_path_recursive (const char *src, const char *dst)
{
    struct stat st;

    if (lstat (src, &st) == -1)
        return -1;

    if (S_ISDIR (st.st_mode))
    {
        DIR *dir;
        struct dirent *direntp;

        if (mkdir (dst, st.st_mode & 0777) == -1 && errno != EEXIST)
            return -1;

        dir = opendir (src);
        if (dir == NULL)
            return -1;

        while ((direntp = readdir (dir)) != NULL)
        {
            char src_child[PATH_MAX];
            char dst_child[PATH_MAX];
            int rc;

            if (strcmp (direntp->d_name, ".") == 0 || strcmp (direntp->d_name, "..") == 0)
                continue;
            if (!join_path (src_child, sizeof (src_child), src, direntp->d_name)
                || !join_path (dst_child, sizeof (dst_child), dst, direntp->d_name))
            {
                closedir (dir);
                errno = ENAMETOOLONG;
                return -1;
            }

            rc = copy_path_recursive (src_child, dst_child);
            if (rc == -1)
            {
                closedir (dir);
                return -1;
            }
        }

        closedir (dir);
        return 0;
    }

    if (S_ISLNK (st.st_mode))
    {
        char target[PATH_MAX];
        ssize_t len = readlink (src, target, sizeof (target) - 1);
        if (len < 0)
            return -1;
        target[len] = '\0';
        return symlink (target, dst);
    }

    if (S_ISREG (st.st_mode))
    {
        int in_fd;
        int out_fd;
        int rc;

        in_fd = open (src, O_RDONLY);
        if (in_fd == -1)
            return -1;

        out_fd = open (dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
        if (out_fd == -1)
        {
            close (in_fd);
            return -1;
        }

        rc = copy_file_data (in_fd, out_fd);
        close (in_fd);
        close (out_fd);
        return rc;
    }

    errno = ENOTSUP;
    return -1;
}

static int
delete_path_recursive (const char *path)
{
    struct stat st;

    if (lstat (path, &st) == -1)
        return -1;

    if (S_ISDIR (st.st_mode))
    {
        DIR *dir;
        struct dirent *direntp;

        dir = opendir (path);
        if (dir == NULL)
            return -1;

        while ((direntp = readdir (dir)) != NULL)
        {
            char child[PATH_MAX];

            if (strcmp (direntp->d_name, ".") == 0 || strcmp (direntp->d_name, "..") == 0)
                continue;
            if (!join_path (child, sizeof (child), path, direntp->d_name))
            {
                closedir (dir);
                errno = ENAMETOOLONG;
                return -1;
            }
            if (delete_path_recursive (child) == -1)
            {
                closedir (dir);
                return -1;
            }
        }

        closedir (dir);
        return rmdir (path);
    }

    return unlink (path);
}

static int
move_path_portable (const char *src, const char *dst)
{
    if (rename (src, dst) == 0)
        return 0;

    if (errno != EXDEV)
        return -1;

    if (copy_path_recursive (src, dst) == -1)
        return -1;

    return delete_path_recursive (src);
}

static bool
spawn_kilo (const char *path, bool readonly)
{
    pid_t pid;
    int status;

    if (ZC_FORCE_BUNDLED_EDITOR && access (g_app.kilo_cmd, X_OK) != 0)
    {
        set_status ("Bundled editor not found: %s", g_app.kilo_cmd);
        return false;
    }

    disable_raw_mode ();
    clear_screen ();
    fflush (stdout);

    pid = fork ();
    if (pid == -1)
    {
        enable_raw_mode ();
        set_status ("Cannot fork: %s", strerror (errno));
        return false;
    }

    if (pid == 0)
    {
        if (readonly)
            execlp (g_app.kilo_cmd, g_app.kilo_cmd, "--readonly", path, (char *) NULL);
        else
            execlp (g_app.kilo_cmd, g_app.kilo_cmd, path, (char *) NULL);
        fprintf (stderr, "Unable to exec %s: %s\n", g_app.kilo_cmd, strerror (errno));
        _exit (127);
    }

    while (waitpid (pid, &status, 0) == -1)
    {
        if (errno != EINTR)
            break;
    }

    enable_raw_mode ();
    if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
        set_status ("%s closed", g_app.kilo_cmd);
    else
        set_status ("%s exited with status %d", g_app.kilo_cmd,
                    WIFEXITED (status) ? WEXITSTATUS (status) : -1);
    return true;
}

static bool
selected_full_path (Panel *panel, char *dst, size_t dst_size)
{
    Entry *entry = panel_selected_entry (panel);

    return entry_full_path (panel, entry, dst, dst_size);
}

static bool
open_selection (bool readonly)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    Entry *entry = panel_selected_entry (panel);
    char path[PATH_MAX];

    if (entry == NULL)
        return false;

    if (entry->is_dir)
    {
        if (!selected_full_path (panel, path, sizeof (path)))
        {
            set_status ("Path too long");
            return false;
        }
        return panel_load_path (panel, path);
    }

    if (!selected_full_path (panel, path, sizeof (path)))
    {
        set_status ("Path too long");
        return false;
    }

    return spawn_kilo (path, readonly);
}

static void
refresh_panels (void)
{
    panel_reload (&g_app.panels[0]);
    panel_reload (&g_app.panels[1]);
}

static void
copy_selection_to_other_panel (bool move_mode)
{
    Panel *src_panel = &g_app.panels[g_app.active_panel];
    Panel *dst_panel = &g_app.panels[1 - g_app.active_panel];
    size_t *indexes = NULL;
    size_t count;

    count = panel_collect_target_indexes (src_panel, &indexes);
    if (count == 0)
    {
        set_status ("Nothing to %s", move_mode ? "move" : "copy");
        return;
    }

    if (count == 1)
    {
        Entry *entry = &src_panel->entries[indexes[0]];
        char src_path[PATH_MAX];
        char default_dst[PATH_MAX];
        char dst_path[PATH_MAX];
        char entry_name[PATH_MAX];
        int rc;

        if (!entry_full_path (src_panel, entry, src_path, sizeof (src_path))
            || !join_path (default_dst, sizeof (default_dst), dst_panel->cwd, entry->name)
            || !copy_string (entry_name, sizeof (entry_name), entry->name))
        {
            free (indexes);
            set_status ("Path too long");
            return;
        }

        if (!prompt_input (move_mode ? "Move to: " : "Copy to: ", default_dst, dst_path,
                           sizeof (dst_path)))
        {
            free (indexes);
            set_status ("%s canceled", move_mode ? "Move" : "Copy");
            return;
        }

        rc = move_mode ? move_path_portable (src_path, dst_path)
                       : copy_path_recursive (src_path, dst_path);
        if (rc == -1)
        {
            free (indexes);
            set_status ("Cannot %s %s: %s", move_mode ? "move" : "copy", entry->name,
                        strerror (errno));
            return;
        }

        refresh_panels ();
        panel_clear_marks (&g_app.panels[g_app.active_panel]);
        set_status ("%s complete: %s", move_mode ? "Move" : "Copy", entry_name);
        free (indexes);
        return;
    }

    {
        char dst_dir[PATH_MAX];
        struct stat st;

        if (!prompt_input (move_mode ? "Move into dir: " : "Copy into dir: ", dst_panel->cwd,
                           dst_dir, sizeof (dst_dir)))
        {
            free (indexes);
            set_status ("%s canceled", move_mode ? "Move" : "Copy");
            return;
        }

        if (stat (dst_dir, &st) == -1 || !S_ISDIR (st.st_mode))
        {
            free (indexes);
            set_status ("Destination is not a directory: %s", dst_dir);
            return;
        }

        for (size_t i = 0; i < count; i++)
        {
            Entry *entry = &src_panel->entries[indexes[i]];
            char src_path[PATH_MAX];
            char dst_path[PATH_MAX];
            int rc;

            if (!entry_full_path (src_panel, entry, src_path, sizeof (src_path))
                || !join_path (dst_path, sizeof (dst_path), dst_dir, entry->name))
            {
                free (indexes);
                set_status ("Path too long");
                return;
            }

            rc = move_mode ? move_path_portable (src_path, dst_path)
                           : copy_path_recursive (src_path, dst_path);
            if (rc == -1)
            {
                free (indexes);
                set_status ("Cannot %s %s: %s", move_mode ? "move" : "copy", entry->name,
                            strerror (errno));
                return;
            }
        }

        refresh_panels ();
        panel_clear_marks (&g_app.panels[g_app.active_panel]);
        set_status ("%s complete: %zu items", move_mode ? "Move" : "Copy", count);
    }

    free (indexes);
}

static void
make_directory_in_active_panel (void)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    char name[PROMPT_LEN];
    char path[PATH_MAX];

    if (!prompt_input ("New directory: ", "", name, sizeof (name)))
    {
        set_status ("mkdir canceled");
        return;
    }

    if (name[0] == '\0')
    {
        set_status ("Directory name is empty");
        return;
    }

    if (!join_path (path, sizeof (path), panel->cwd, name))
    {
        set_status ("Path too long");
        return;
    }

    if (mkdir (path, 0755) == -1)
    {
        set_status ("Cannot create %s: %s", name, strerror (errno));
        return;
    }

    panel_reload (panel);
    set_status ("Created directory: %s", name);
}

static void
create_file_in_active_panel (void)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    char name[PROMPT_LEN];
    char path[PATH_MAX];
    int fd;

    if (!prompt_input ("New file: ", "", name, sizeof (name)))
    {
        set_status ("new file canceled");
        return;
    }

    if (name[0] == '\0')
    {
        set_status ("File name is empty");
        return;
    }

    if (!join_path (path, sizeof (path), panel->cwd, name))
    {
        set_status ("Path too long");
        return;
    }

    fd = open (path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
    {
        set_status ("Cannot create %s: %s", name, strerror (errno));
        return;
    }

    close (fd);
    panel_reload (panel);
    set_status ("Created file: %s", name);
}

static void
delete_selection (void)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    size_t *indexes = NULL;
    size_t count;

    count = panel_collect_target_indexes (panel, &indexes);
    if (count == 0)
    {
        set_status ("Nothing to delete");
        return;
    }

    if (count == 1)
    {
        Entry *entry = &panel->entries[indexes[0]];
        char path[PATH_MAX];
        char question[STATUS_LEN];
        char entry_name[PATH_MAX];

        if (!copy_string (entry_name, sizeof (entry_name), entry->name))
        {
            free (indexes);
            set_status ("Path too long");
            return;
        }

        snprintf (question, sizeof (question), "Delete %.*s?",
                  (int) (sizeof (question) - sizeof ("Delete ?")), entry_name);
        if (!prompt_confirm (question))
        {
            free (indexes);
            set_status ("Delete canceled");
            return;
        }

        if (!entry_full_path (panel, entry, path, sizeof (path)))
        {
            free (indexes);
            set_status ("Path too long");
            return;
        }

        if (delete_path_recursive (path) == -1)
        {
            free (indexes);
            set_status ("Cannot delete %s: %s", entry->name, strerror (errno));
            return;
        }

        panel_reload (panel);
        panel_clear_marks (panel);
        set_status ("Deleted: %s", entry_name);
        free (indexes);
        return;
    }

    {
        char question[STATUS_LEN];

        snprintf (question, sizeof (question), "Delete %zu selected entries?", count);
        if (!prompt_confirm (question))
        {
            free (indexes);
            set_status ("Delete canceled");
            return;
        }

        for (size_t i = 0; i < count; i++)
        {
            Entry *entry = &panel->entries[indexes[i]];
            char path[PATH_MAX];

            if (!entry_full_path (panel, entry, path, sizeof (path)))
            {
                free (indexes);
                set_status ("Path too long");
                return;
            }

            if (delete_path_recursive (path) == -1)
            {
                free (indexes);
                set_status ("Cannot delete %s: %s", entry->name, strerror (errno));
                return;
            }
        }

        panel_reload (panel);
        panel_clear_marks (panel);
        set_status ("Deleted: %zu items", count);
    }

    free (indexes);
}

static void
rename_selection_in_place (void)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    Entry *entry;
    size_t *indexes = NULL;
    size_t count;
    char new_name[PROMPT_LEN];
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
    char old_name[PATH_MAX];

    count = panel_collect_target_indexes (panel, &indexes);
    if (count == 0)
    {
        set_status ("Nothing to rename");
        return;
    }
    if (count != 1)
    {
        free (indexes);
        set_status ("Rename needs a single selected item");
        return;
    }

    entry = &panel->entries[indexes[0]];
    if (!copy_string (old_name, sizeof (old_name), entry->name))
    {
        free (indexes);
        set_status ("Path too long");
        return;
    }

    if (!prompt_input ("Rename to: ", old_name, new_name, sizeof (new_name)))
    {
        free (indexes);
        set_status ("Rename canceled");
        return;
    }

    if (new_name[0] == '\0')
    {
        free (indexes);
        set_status ("New name is empty");
        return;
    }

    if (!entry_full_path (panel, entry, src_path, sizeof (src_path))
        || !join_path (dst_path, sizeof (dst_path), panel->cwd, new_name))
    {
        free (indexes);
        set_status ("Path too long");
        return;
    }

    if (rename (src_path, dst_path) == -1)
    {
        free (indexes);
        set_status ("Cannot rename %s: %s", entry->name, strerror (errno));
        return;
    }

    panel_reload (panel);
    panel_clear_marks (panel);
    set_status ("Renamed %s -> %s", old_name, new_name);
    free (indexes);
}

static void
change_to_parent (void)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    char parent[PATH_MAX];

    if (!parent_path (parent, sizeof (parent), panel->cwd))
    {
        set_status ("Path too long");
        return;
    }

    panel_load_path (panel, parent);
}

static void
switch_panel (void)
{
    g_app.active_panel = 1 - g_app.active_panel;
}

static void
handle_key (int key)
{
    Panel *panel = &g_app.panels[g_app.active_panel];
    int body_rows = g_app.screen_rows - 4;

    if (body_rows < 1)
        body_rows = 1;

    switch (key)
    {
    case KEY_F1:
        show_help_screen ();
        break;
    case KEY_ARROW_UP:
        panel_move_selection (panel, -1);
        break;
    case KEY_ARROW_DOWN:
        panel_move_selection (panel, 1);
        break;
    case KEY_PAGE_UP:
        panel_move_selection (panel, -body_rows);
        break;
    case KEY_PAGE_DOWN:
        panel_move_selection (panel, body_rows);
        break;
    case KEY_HOME:
        panel->selected = 0;
        panel_adjust_scroll (panel);
        break;
    case KEY_END:
        if (panel->len > 0)
            panel->selected = panel->len - 1;
        panel_adjust_scroll (panel);
        break;
    case KEY_ARROW_LEFT:
    case KEY_ARROW_RIGHT:
    case '\t':
        switch_panel ();
        break;
    case CTRL_KEY ('n'):
    case 'a':
    case 'A':
        create_file_in_active_panel ();
        break;
    case KEY_F2:
        set_status ("F2 user menu is not implemented");
        break;
    case ' ':
        panel_toggle_selected_mark (panel);
        break;
    case '*':
        panel_toggle_all_marks (panel);
        break;
    case KEY_ENTER:
    case 'e':
    case 'E':
        open_selection (false);
        break;
    case 'v':
    case 'V':
    case KEY_F3:
        open_selection (true);
        break;
    case KEY_F4:
        open_selection (false);
        break;
    case KEY_F5:
    case 'c':
    case 'C':
        copy_selection_to_other_panel (false);
        break;
    case KEY_F6:
    case 'm':
    case 'M':
        copy_selection_to_other_panel (true);
        break;
    case 'n':
    case 'N':
        rename_selection_in_place ();
        break;
    case KEY_F7:
        make_directory_in_active_panel ();
        break;
    case KEY_F8:
    case 'd':
    case 'D':
        delete_selection ();
        break;
    case KEY_BACKSPACE:
    case '-':
        change_to_parent ();
        break;
    case 'q':
    case 'Q':
    case CTRL_KEY ('q'):
    case KEY_F10:
        g_app.running = false;
        break;
    case KEY_F9:
        set_status ("F9 menu bar is not implemented");
        break;
    case 'r':
    case 'R':
    case KEY_CTRL_L:
        refresh_panels ();
        set_status ("Refreshed");
        break;
    default:
        break;
    }
}

static void
print_help (const char *argv0)
{
    printf ("Usage: %s [path] [--left PATH --right PATH]\n", argv0);
    printf ("Minimal two-panel file manager using external kilo.\n");
}

static bool
parse_args (int argc, char **argv, char *left_path, size_t left_size, char *right_path,
            size_t right_size)
{
    int i;

    if (!copy_string (left_path, left_size, ".") || !copy_string (right_path, right_size, "."))
        return false;

    for (i = 1; i < argc; i++)
    {
        if (strcmp (argv[i], "--help") == 0)
        {
            print_help (argv[0]);
            exit (EXIT_SUCCESS);
        }
        if (strcmp (argv[i], "--version") == 0)
        {
            printf ("zc %s\n", ZC_VERSION);
            exit (EXIT_SUCCESS);
        }
        if (strcmp (argv[i], "--left") == 0 && i + 1 < argc)
        {
            if (!copy_string (left_path, left_size, argv[++i]))
                return false;
            continue;
        }
        if (strcmp (argv[i], "--right") == 0 && i + 1 < argc)
        {
            if (!copy_string (right_path, right_size, argv[++i]))
                return false;
            continue;
        }
        if (argv[i][0] == '-')
        {
            fprintf (stderr, "Unknown option: %s\n", argv[i]);
            return false;
        }

        if (!copy_string (left_path, left_size, argv[i]) || !copy_string (right_path, right_size, argv[i]))
            return false;
    }

    return true;
}

static void
init_app (const char *argv0)
{
    const char *kilo_env = getenv ("ZC_KILO");

    memset (&g_app, 0, sizeof (g_app));
    g_app.running = true;

    if (!ZC_FORCE_BUNDLED_EDITOR && kilo_env != NULL && kilo_env[0] != '\0')
        copy_string (g_app.kilo_cmd, sizeof (g_app.kilo_cmd), kilo_env);
    else
        discover_default_kilo (argv0, g_app.kilo_cmd, sizeof (g_app.kilo_cmd));
    if (ZC_FORCE_BUNDLED_EDITOR)
        set_status ("Ready. bundled editor: %s", g_app.kilo_cmd);
    else
        set_status ("Ready. kilo command: %s", g_app.kilo_cmd);
}

int
main (int argc, char **argv)
{
    char left_path[PATH_MAX];
    char right_path[PATH_MAX];

    init_app (argv[0]);
    if (!parse_args (argc, argv, left_path, sizeof (left_path), right_path, sizeof (right_path)))
    {
        print_help (argv[0]);
        return EXIT_FAILURE;
    }

    if (!panel_load_path (&g_app.panels[0], left_path) || !panel_load_path (&g_app.panels[1], right_path))
        return EXIT_FAILURE;

    enable_raw_mode ();
    atexit (disable_raw_mode);

    while (g_app.running)
    {
        int key;

        draw_screen (NULL, NULL);
        key = read_key ();
        handle_key (key);
    }

    disable_raw_mode ();
    clear_screen ();
    return EXIT_SUCCESS;
}
