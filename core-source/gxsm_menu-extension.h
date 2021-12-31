/*
 * gxsm-menu-extension.h
 * This file is part of gxsm
 *
 * Copyright (C) 2014 - Ignacio Casal Quinteiro
 *
 * gxsm is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gxsm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gxsm. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GXSM_MENU_EXTENSION_H__
#define __GXSM_MENU_EXTENSION_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GXSM_TYPE_MENU_EXTENSION		(gxsm_menu_extension_get_type ())
#define GXSM_MENU_EXTENSION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GXSM_TYPE_MENU_EXTENSION, GxsmMenuExtension))
#define GXSM_MENU_EXTENSION_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GXSM_TYPE_MENU_EXTENSION, GxsmMenuExtension const))
#define GXSM_MENU_EXTENSION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GXSM_TYPE_MENU_EXTENSION, GxsmMenuExtensionClass))
#define GXSM_IS_MENU_EXTENSION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GXSM_TYPE_MENU_EXTENSION))
#define GXSM_IS_MENU_EXTENSION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GXSM_TYPE_MENU_EXTENSION))
#define GXSM_MENU_EXTENSION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GXSM_TYPE_MENU_EXTENSION, GxsmMenuExtensionClass))

typedef struct _GxsmMenuExtension	 GxsmMenuExtension;
typedef struct _GxsmMenuExtensionClass	 GxsmMenuExtensionClass;

struct _GxsmMenuExtension
{
	GObject parent;
};

struct _GxsmMenuExtensionClass
{
	GObjectClass parent_class;
};

GType                     gxsm_menu_extension_get_type            (void) G_GNUC_CONST;

GxsmMenuExtension       *gxsm_menu_extension_new                 (GMenu                *menu);

void                      gxsm_menu_extension_append_menu_item    (GxsmMenuExtension   *menu,
                                                                    GMenuItem            *item);

void                      gxsm_menu_extension_prepend_menu_item   (GxsmMenuExtension   *menu,
                                                                    GMenuItem            *item);

void                      gxsm_menu_extension_remove_items        (GxsmMenuExtension   *menu);

G_END_DECLS

#endif /* __GXSM_MENU_EXTENSION_H__ */

/* ex:set ts=8 noet: */
