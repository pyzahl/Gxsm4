/*
 * gxsm-menu-extension.c
 * This file is part of gxsm -- code borrowed from Gedit.
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

#include "gxsm_menu-extension.h"

#include <string.h>

static guint last_merge_id = 0;


typedef struct _GxsmMenuExtensionPrivate
{
	GMenu *menu;
	guint merge_id;
	gboolean dispose_has_run;
} GxsmMenuExtensionPrivate;

enum
{
	PROP_0,
	PROP_MENU
};

G_DEFINE_TYPE_WITH_PRIVATE (GxsmMenuExtension, gxsm_menu_extension, G_TYPE_OBJECT)

static void
gxsm_menu_extension_dispose (GObject *object)
{
	GxsmMenuExtension *menu = GXSM_MENU_EXTENSION (object);
	GxsmMenuExtensionPrivate *priv = gxsm_menu_extension_get_instance_private (menu);

	if (!priv->dispose_has_run)
	{
		gxsm_menu_extension_remove_items (menu);
		priv->dispose_has_run = TRUE;
	}

	g_clear_object (&priv->menu);

	G_OBJECT_CLASS (gxsm_menu_extension_parent_class)->dispose (object);
}

static void
gxsm_menu_extension_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
	GxsmMenuExtension *menu = GXSM_MENU_EXTENSION (object);
	GxsmMenuExtensionPrivate *priv = gxsm_menu_extension_get_instance_private (menu);

	switch (prop_id)
	{
		case PROP_MENU:
			g_value_set_object (value, priv->menu);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gxsm_menu_extension_set_property (GObject     *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
	GxsmMenuExtension *menu = GXSM_MENU_EXTENSION (object);
	GxsmMenuExtensionPrivate *priv = gxsm_menu_extension_get_instance_private (menu);

	switch (prop_id)
	{
		case PROP_MENU:
			priv->menu = g_value_dup_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gxsm_menu_extension_class_init (GxsmMenuExtensionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gxsm_menu_extension_dispose;
	object_class->get_property = gxsm_menu_extension_get_property;
	object_class->set_property = gxsm_menu_extension_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_MENU,
	                                 g_param_spec_object ("menu",
	                                                      "Menu",
	                                                      "The main menu",
	                                                      G_TYPE_MENU,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_STATIC_STRINGS));
}

static void
gxsm_menu_extension_init (GxsmMenuExtension *menu)
{
	GxsmMenuExtensionPrivate *priv;

	priv = gxsm_menu_extension_get_instance_private (menu);

	priv->merge_id = ++last_merge_id;
}

GxsmMenuExtension *
gxsm_menu_extension_new (GMenu *menu)
{
	return g_object_new (GXSM_TYPE_MENU_EXTENSION, "menu", menu, NULL);
}

void
gxsm_menu_extension_append_menu_item (GxsmMenuExtension *menu,
                                       GMenuItem       *item)
{
	GxsmMenuExtensionPrivate *priv;

	g_return_if_fail (GXSM_IS_MENU_EXTENSION (menu));
	g_return_if_fail (G_IS_MENU_ITEM (item));

	priv = gxsm_menu_extension_get_instance_private (menu);

	if (priv->menu != NULL)
	{
		g_menu_item_set_attribute (item, "gxsm-merge-id", "u", priv->merge_id);
		g_menu_append_item (priv->menu, item);
	}
}

void
gxsm_menu_extension_prepend_menu_item (GxsmMenuExtension *menu,
                                        GMenuItem       *item)
{
	GxsmMenuExtensionPrivate *priv;

	g_return_if_fail (GXSM_IS_MENU_EXTENSION (menu));
	g_return_if_fail (G_IS_MENU_ITEM (item));

	priv = gxsm_menu_extension_get_instance_private (menu);

	if (priv->menu != NULL)
	{
		g_menu_item_set_attribute (item, "gxsm-merge-id", "u", priv->merge_id);
		g_menu_prepend_item (priv->menu, item);
	}
}

void
gxsm_menu_extension_remove_items (GxsmMenuExtension *menu)
{
	GxsmMenuExtensionPrivate *priv;
	gint i, n_items;

	g_return_if_fail (GXSM_IS_MENU_EXTENSION (menu));

	priv = gxsm_menu_extension_get_instance_private (menu);

	n_items = g_menu_model_get_n_items (G_MENU_MODEL (priv->menu));
	i = 0;
	while (i < n_items)
	{
		guint id = 0;

		if (g_menu_model_get_item_attribute (G_MENU_MODEL (priv->menu),
		                                     i, "gxsm-merge-id", "u", &id) &&
		    id == priv->merge_id)
		{
			g_menu_remove (priv->menu, i);
			n_items--;
		}
		else
		{
			i++;
		}
	}
}

/* ex:set ts=8 noet: */
