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

#ifndef __WS_TMP_DIR_H__
#define __WS_TMP_DIR_H__

#include "ws_symbol_export.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

WS_DLL_PUBLIC const char *get_tmp_dir(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __WS_TMP_DIR_H__
