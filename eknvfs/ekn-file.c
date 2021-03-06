/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * ekn-file.c
 *
 * Copyright (C) 2016 Endless Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Juan Pablo Ugarte <ugarte@endlessm.com>
 *
 */

#include <dm-utils.h>
#include <dm-shard.h>
#include "ekn-file.h"
#include "ekn-file-input-stream-wrapper.h"

#define EKN_SCHEME_LEN 6

struct _EknFile
{
  GObject parent;
};

typedef struct
{
  gchar *uri;
  DmShardRecord *record;
  DmContent *content;
} EknFilePrivate;

enum
{
  PROP_0,
  PROP_URI,
  PROP_RECORD,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

static void ekn_file_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (EknFile,
			ekn_file,
			G_TYPE_OBJECT, 0,
			G_ADD_PRIVATE (EknFile)
			G_IMPLEMENT_INTERFACE (G_TYPE_FILE,
                                               ekn_file_iface_init))

#define EKN_FILE_PRIVATE(d) ((EknFilePrivate *) ekn_file_get_instance_private((EknFile*)d))

static void
ekn_file_init (G_GNUC_UNUSED EknFile *self)
{
}

static void
ekn_file_finalize (GObject *self)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);

  g_clear_pointer (&priv->uri, g_free);
  g_clear_pointer (&priv->record, dm_shard_record_unref);

  G_OBJECT_CLASS (ekn_file_parent_class)->finalize (self);
}

static inline void
ekn_file_set_uri (EknFile *self, const gchar *uri)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);

  g_return_if_fail (dm_utils_is_valid_uri (uri));

  if (g_strcmp0 (priv->uri, uri) != 0)
    {
      g_free (priv->uri);
      priv->uri = g_strdup (uri);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI]);
    }
}

static inline void
ekn_file_set_record (EknFile *self, DmShardRecord *record)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);

  g_clear_pointer (&priv->record, dm_shard_record_unref);
  g_clear_pointer (&priv->content, g_object_unref);

  if (record)
    priv->record = dm_shard_record_ref (record);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RECORD]);
}

static void
ekn_file_set_property (GObject      *self,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  g_return_if_fail (EKN_IS_FILE (self));

  switch (prop_id)
    {
    case PROP_URI:
      ekn_file_set_uri (EKN_FILE (self), g_value_get_string (value));
      break;
    case PROP_RECORD:
      ekn_file_set_record (EKN_FILE (self), g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

static void
ekn_file_get_property (GObject    *self,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  EknFilePrivate *priv;

  g_return_if_fail (EKN_IS_FILE (self));
  priv = EKN_FILE_PRIVATE (self);

  switch (prop_id)
    {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
    case PROP_RECORD:
      g_value_set_boxed (value, priv->record);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

/* GFile iface implementation */

static GFile *
ekn_file_dup (GFile *self)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);
  return _ekn_file_new (priv->uri, priv->record);
}

static guint
ekn_file_hash (GFile *self)
{
  return g_str_hash (EKN_FILE_PRIVATE (self)->uri);
}

static gboolean
ekn_file_equal (GFile *file1, GFile *file2)
{
  return g_str_equal (EKN_FILE_PRIVATE (file1)->uri, EKN_FILE_PRIVATE (file2)->uri);
}

static gboolean
ekn_file_is_native (G_GNUC_UNUSED GFile *self)
{
  return TRUE;
}

static gboolean
ekn_file_has_uri_scheme (G_GNUC_UNUSED GFile *self,
                         const char *uri_scheme)
{
  return g_str_equal ("ekn", uri_scheme);
}

static char *
ekn_file_get_uri_scheme (G_GNUC_UNUSED GFile *self)
{
  return g_strdup ("ekn");
}

static char *
ekn_file_get_basename (GFile *self)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);
  return priv->uri ? g_path_get_basename (priv->uri + EKN_SCHEME_LEN) : NULL;
}

static char *
ekn_file_get_path (GFile *self)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);
  return priv->uri ? g_strdup (priv->uri + EKN_SCHEME_LEN) : NULL;
}

static char *
ekn_file_get_uri (GFile *self)
{
  return g_strdup (EKN_FILE_PRIVATE (self)->uri);
}

static char *
ekn_file_get_parse_name (GFile *self)
{
  return g_strdup (EKN_FILE_PRIVATE (self)->uri);
}

static GFile *
ekn_file_get_parent (G_GNUC_UNUSED GFile *self)
{
  return NULL;
}

static GFileInfo *
ekn_file_query_info (GFile                *self,
                     const char           *attributes,
                     G_GNUC_UNUSED GFileQueryInfoFlags flags,
                     GCancellable         *cancellable,
                     GError              **error)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);
  GFileAttributeMatcher *matcher;
  GFileInfo *info;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  info    = g_file_info_new ();
  matcher = g_file_attribute_matcher_new (attributes);

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_SIZE))
    {
      gsize size = dm_shard_get_data_size (dm_shard_record_get_shard (priv->record),
                                           priv->record);
      g_file_info_set_size (info, size);
    }

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
    {
      if (!priv->content)
        priv->content = dm_shard_get_model (dm_shard_record_get_shard (priv->record),
                                            priv->record, cancellable, error);

      if (g_cancellable_set_error_if_cancelled (cancellable, error))
        return NULL;

      g_autofree gchar *content_type;
      g_object_get (priv->content, "content-type", &content_type, NULL);
      g_file_info_set_content_type (info, content_type);
    }

  g_file_attribute_matcher_unref (matcher);

  return info;
}

static GFileInputStream *
ekn_file_read_fn (GFile *self, GCancellable *cancellable, GError **error)
{
  EknFilePrivate *priv = EKN_FILE_PRIVATE (self);

  GInputStream *stream = dm_shard_stream_data (dm_shard_record_get_shard (priv->record),
                                               priv->record, cancellable, error);
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  return _ekn_file_input_stream_wrapper_new (self, stream);
}

static void
ekn_file_iface_init (gpointer g_iface,
                     G_GNUC_UNUSED gpointer iface_data)
{
  GFileIface *iface = g_iface;

  iface->dup            = ekn_file_dup;
  iface->hash           = ekn_file_hash;
  iface->equal          = ekn_file_equal;
  iface->is_native      = ekn_file_is_native;
  iface->has_uri_scheme = ekn_file_has_uri_scheme;
  iface->get_uri_scheme = ekn_file_get_uri_scheme;
  iface->get_basename   = ekn_file_get_basename;
  iface->get_path       = ekn_file_get_path;
  iface->get_uri        = ekn_file_get_uri;
  iface->get_parse_name = ekn_file_get_parse_name;
  iface->get_parent     = ekn_file_get_parent;
  iface->query_info     = ekn_file_query_info;
  iface->read_fn        = ekn_file_read_fn;
}

static void
ekn_file_class_init (EknFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ekn_file_finalize;
  object_class->get_property = ekn_file_get_property;
  object_class->set_property = ekn_file_set_property;

  /* Properties */
  properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "URI",
                         "EOS Knowledge URI",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  properties[PROP_RECORD] =
    g_param_spec_boxed ("record",
                        "Record",
                        "DmShardRecord for URI",
                        DM_TYPE_SHARD_RECORD,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

GFile *
_ekn_file_new (const gchar *uri, DmShardRecord *record)
{
  return g_object_new (EKN_TYPE_FILE, "uri", uri, "record", record, NULL);
}
