/* wstmpdir.c
 *
 * Copyright (C) 2013 Red Hat, Inc. All right reserved.
 *
 * Temporary directory routine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Peter Hatina <phatina@redhat.com>
 */

#include "config.h"

#include <glib.h>
#include "wstmpdir.h"

/**
 * Gets the directory to use for temporary files.
 *
 * Inspired by glib-2.0. If no TMP, TEMP or TMPDIR is set,
 * /var/tmp is returned (Fedora specific).
 *
 * Returns: the directory to use for temporary files.
 */
const char *get_tmp_dir(void)
{
    static gchar *tmp_dir;

    if (g_once_init_enter(&tmp_dir)) {
        gchar *tmp;

        tmp = g_strdup(g_getenv("TEMP"));
        if (tmp == NULL || *tmp == '\0') {
            g_free(tmp);
            tmp = g_strdup(g_getenv("TMPDIR"));
        }

#ifdef P_tmpdir
        if (tmp == NULL || *tmp == '\0') {
            gsize k;
            g_free(tmp);
            tmp = g_strdup(P_tmpdir);
            k = strlen(tmp);
            if (k > 1 && G_IS_DIR_SEPARATOR(tmp[k - 1]))
                tmp[k - 1] = '\0';
        }
#endif /* P_tmpdir */

        if (tmp == NULL || *tmp == '\0') {
            g_free(tmp);
            tmp = g_strdup("/var/tmp");
        }

        g_once_init_leave(&tmp_dir, tmp);
    }

    return tmp_dir;
}
