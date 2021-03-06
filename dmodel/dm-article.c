/* Copyright 2016 Endless Mobile, Inc. */

#include "dm-article.h"
#include "dm-macros.h"
#include "dm-utils-private.h"
#include "dm-content-private.h"

#include <archive.h>
#include <archive_entry.h>
#include <endless/endless.h>

typedef struct {
  GInputStream *stream;
  void *buffer;
} DmArchiveClientData;

#define DM_ARCHIVE_CLIENT_DATA_BUFFER_SIZE 4096

/**
 * SECTION:article
 * @title: Article
 * @short_description: Access article object metadata
 *
 * The model class for article objects.
 */
typedef struct {
  gchar *source;
  gchar *source_name;
  gchar *published;
  guint word_count;
  gboolean is_server_templated;
  char **authors;
  char **temporal_coverage;
  char **outgoing_links;
  GVariant *table_of_contents;
} DmArticlePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DmArticle, dm_article, DM_TYPE_CONTENT)

enum {
  PROP_0,
  PROP_SOURCE,
  PROP_SOURCE_NAME,
  PROP_PUBLISHED,
  PROP_WORD_COUNT,
  PROP_IS_SERVER_TEMPLATED,
  PROP_AUTHORS,
  PROP_TEMPORAL_COVERAGE,
  PROP_OUTGOING_LINKS,
  PROP_TABLE_OF_CONTENTS,
  NPROPS
};

static GParamSpec *dm_article_props[NPROPS] = { NULL, };

static void
dm_article_get_property (GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
  DmArticle *self = DM_ARTICLE (object);
  DmArticlePrivate *priv = dm_article_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_SOURCE:
      g_value_set_string (value, priv->source);
      break;

    case PROP_SOURCE_NAME:
      g_value_set_string (value, priv->source_name);
      break;

    case PROP_PUBLISHED:
      g_value_set_string (value, priv->published);
      break;

    case PROP_WORD_COUNT:
      g_value_set_uint (value, priv->word_count);
      break;

    case PROP_IS_SERVER_TEMPLATED:
      g_value_set_boolean (value, priv->is_server_templated);
      break;

    case PROP_AUTHORS:
      g_value_set_boxed (value, priv->authors);
      break;

    case PROP_TEMPORAL_COVERAGE:
      g_value_set_boxed (value, priv->temporal_coverage);
      break;

    case PROP_OUTGOING_LINKS:
      g_value_set_boxed (value, priv->outgoing_links);
      break;

    case PROP_TABLE_OF_CONTENTS:
      g_value_set_variant (value, priv->table_of_contents);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dm_article_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  DmArticle *self = DM_ARTICLE (object);
  DmArticlePrivate *priv = dm_article_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_SOURCE:
      g_clear_pointer (&priv->source, g_free);
      priv->source = g_value_dup_string (value);
      break;

    case PROP_SOURCE_NAME:
      g_clear_pointer (&priv->source_name, g_free);
      priv->source_name = g_value_dup_string (value);
      break;

    case PROP_PUBLISHED:
      g_clear_pointer (&priv->published, g_free);
      priv->published = g_value_dup_string (value);
      break;

    case PROP_WORD_COUNT:
      priv->word_count = g_value_get_uint (value);
      break;

    case PROP_IS_SERVER_TEMPLATED:
      priv->is_server_templated = g_value_get_boolean (value);
      break;

    case PROP_AUTHORS:
      g_clear_pointer (&priv->authors, g_strfreev);
      priv->authors = g_value_dup_boxed (value);
      break;

    case PROP_TEMPORAL_COVERAGE:
      g_clear_pointer (&priv->temporal_coverage, g_strfreev);
      priv->temporal_coverage = g_value_dup_boxed (value);
      break;

    case PROP_OUTGOING_LINKS:
      g_clear_pointer (&priv->outgoing_links, g_strfreev);
      priv->outgoing_links = g_value_dup_boxed (value);
      break;

    case PROP_TABLE_OF_CONTENTS:
      g_clear_pointer (&priv->table_of_contents, g_variant_unref);
      priv->table_of_contents = g_value_dup_variant (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dm_article_finalize (GObject *object)
{
  DmArticle *self = DM_ARTICLE (object);
  DmArticlePrivate *priv = dm_article_get_instance_private (self);

  g_clear_pointer (&priv->source, g_free);
  g_clear_pointer (&priv->source_name, g_free);
  g_clear_pointer (&priv->published, g_free);
  g_clear_pointer (&priv->authors, g_strfreev);
  g_clear_pointer (&priv->temporal_coverage, g_strfreev);
  g_clear_pointer (&priv->outgoing_links, g_strfreev);
  g_clear_pointer (&priv->table_of_contents, g_variant_unref);

  G_OBJECT_CLASS (dm_article_parent_class)->finalize (object);
}

static void
dm_article_class_init (DmArticleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dm_article_get_property;
  object_class->set_property = dm_article_set_property;
  object_class->finalize = dm_article_finalize;

  /**
   * DmArticle:source:
   *
   * Source of the HTML. Right now can be wikipedia, wikihow, wikisource
   * or wikibooks.
   */
  dm_article_props[PROP_SOURCE] =
    g_param_spec_string ("source", "Source of the HTML",
      "Where the article html was retrieved from.",
      "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:source-name:
   *
   * Human-readable name of the source of this article
   *
   * A string containing the name of this article's source.
   * For example, "Wikipedia" or "Huffington Post" or "Cosimo's Blog".
   */
  dm_article_props[PROP_SOURCE_NAME] =
    g_param_spec_string ("source-name", "Source name",
      "Human-readable name of the source of this article",
      "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:published:
   *
   * The date this article was published. It treats dates
   * according to the ISO8601 standard.
   */
  dm_article_props[PROP_PUBLISHED] =
    g_param_spec_string ("published", "Publication Date",
      "Publication Date of the article",
      "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:word-count:
   *
   * Integer indicating how many words are in the article
   */
  dm_article_props[PROP_WORD_COUNT] =
    g_param_spec_uint ("word-count", "Word Count",
      "Number of words contained in the article body",
      0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:is-server-templated:
   *
   * Whether this content should be given priority in the UI
   */
  dm_article_props[PROP_IS_SERVER_TEMPLATED] =
    g_param_spec_boolean ("is-server-templated", "Is Server Templated",
      "Is Server Templated",
      FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:authors:
   *
   * A list of authors of the article being read
   */
  dm_article_props[PROP_AUTHORS] =
    g_param_spec_boxed ("authors", "Authors",
      "A list of authors of the article being read",
      G_TYPE_STRV,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:temporal-coverage:
   *
   * A list of dates that the article being read refers to. The
   * dates are all in ISO8601.
   */
  dm_article_props[PROP_TEMPORAL_COVERAGE] =
    g_param_spec_boxed ("temporal-coverage", "Temporal Coverage",
      "A list of dates that the article being read refers to",
      G_TYPE_STRV,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:outgoing-links:
   *
   * A list of the outbound links present in this article.
   */
  dm_article_props[PROP_OUTGOING_LINKS] =
    g_param_spec_boxed ("outgoing-links", "Outgoing Links",
      "A list of the outbound links present in this article",
      G_TYPE_STRV,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmArticle:table-of-contents:
   *
   * A json array representing the article's hierarchical table of
   * contents
   */
  dm_article_props[PROP_TABLE_OF_CONTENTS] =
    g_param_spec_variant ("table-of-contents", "Table of Contents",
      "A json array representing the article's hierarchical table of contents",
      G_VARIANT_TYPE ("aa{sv}"), NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NPROPS, dm_article_props);
}

static void
dm_article_init (G_GNUC_UNUSED DmArticle *self)
{
}

static void
dm_article_add_json_to_params (JsonNode *node,
                               GArray *params)
{
  if (!JSON_NODE_HOLDS_OBJECT (node))
    {
      g_critical ("Trying to instantiate a DmArticle from a non json object.");
      return;
    }

  dm_content_add_json_to_params (node, params);

  JsonObject *object = json_node_get_object (node);
  GObjectClass *klass = g_type_class_ref (DM_TYPE_ARTICLE);

  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "source"),
                                         g_object_class_find_property (klass, "source"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "sourceName"),
                                         g_object_class_find_property (klass, "source-name"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "published"),
                                         g_object_class_find_property (klass, "published"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "wordCount"),
                                         g_object_class_find_property (klass, "word-count"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "isServerTemplated"),
                                         g_object_class_find_property (klass, "is-server-templated"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "authors"),
                                         g_object_class_find_property (klass, "authors"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "temporalCoverage"),
                                         g_object_class_find_property (klass, "temporal-coverage"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "outgoingLinks"),
                                         g_object_class_find_property (klass, "outgoing-links"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "tableOfContents"),
                                         g_object_class_find_property (klass, "table-of-contents"),
                                         params);
  g_type_class_unref (klass);
}

/**
 * dm_article_get_authors:
 * @self: the model
 *
 * Get the models authors.
 *
 * Returns: (transfer none) (array zero-terminated=1): an array of strings
 */
char * const *
dm_article_get_authors (DmArticle *self)
{
  g_return_val_if_fail (DM_IS_ARTICLE (self), NULL);

  DmArticlePrivate *priv = dm_article_get_instance_private (self);
  return priv->authors;
}

/**
 * dm_article_get_temporal_coverage:
 * @self: the model
 *
 * Get the temporal coverage over the article.
 *
 * Since: 2
 * Returns: (transfer none) (array zero-terminated=1): a list of strings
 */
char * const *
dm_article_get_temporal_coverage (DmArticle *self)
{
  g_return_val_if_fail (DM_IS_ARTICLE (self), NULL);

  DmArticlePrivate *priv = dm_article_get_instance_private (self);
  return priv->temporal_coverage;
}

/**
 * dm_article_get_outgoing_links:
 * @self: the model
 *
 * Get the models outgoing_links.
 *
 * Returns: (transfer none) (array zero-terminated=1): a list of strings
 */
char * const *
dm_article_get_outgoing_links (DmArticle *self)
{
  g_return_val_if_fail (DM_IS_ARTICLE (self), NULL);

  DmArticlePrivate *priv = dm_article_get_instance_private (self);
  return priv->outgoing_links;
}

/**
 * dm_article_get_table_of_contents:
 * @self: the model
 *
 * Get the models table of contents.
 *
 * Returns: (transfer none): the resources GVariant
 */
GVariant *
dm_article_get_table_of_contents (DmArticle *self)
{
  g_return_val_if_fail (DM_IS_ARTICLE (self), NULL);

  DmArticlePrivate *priv = dm_article_get_instance_private (self);
  return priv->table_of_contents;
}

static la_ssize_t
_archive_read_callback(G_GNUC_UNUSED struct archive *a, void *client_data, const void **buffer)
{
  DmArchiveClientData *cdata = (DmArchiveClientData *) client_data;
  *buffer = cdata->buffer;
  return g_input_stream_read (G_INPUT_STREAM (cdata->stream),
                              cdata->buffer,
                              DM_ARCHIVE_CLIENT_DATA_BUFFER_SIZE,
                              NULL,
                              NULL);
}

static la_int64_t
_archive_skip_callback(G_GNUC_UNUSED struct archive *a, void *client_data, off_t request)
{
  DmArchiveClientData *cdata = (DmArchiveClientData *) client_data;
  return g_input_stream_skip (G_INPUT_STREAM (cdata->stream), request, NULL, NULL);
}

static int
_archive_open_callback(G_GNUC_UNUSED struct archive *a, G_GNUC_UNUSED void *client_data)
{
  return ARCHIVE_OK;
}

static int
_archive_close_callback(G_GNUC_UNUSED struct archive *a, G_GNUC_UNUSED void *client_data)
{
  return ARCHIVE_OK;
}

/**
 * dm_article_get_archive_member_content_stream:
 * @self: the model
 * @member_name: the archive member name
 * @error: error object
 *
 * For the cases of models that are archives (ZIP files), get a stream for
 * the specified member inside the archive.
 *
 * Returns: (transfer full): a GMemoryInputStream of the member content
 */
GInputStream *
dm_article_get_archive_member_content_stream (DmArticle *self,
                                              const char *member_name,
                                              GError **error)
{
  struct archive *arch;
  struct archive_entry *arch_entry = NULL;
  GInputStream *member_stream = NULL;
  DmArchiveClientData client_data;
  g_autoptr(GFileInputStream) client_data_stream = NULL;
  g_autofree void *client_data_buffer = NULL;
  int status;

  arch = archive_read_new ();
  dm_libarchive_set_error_and_return_if_fail (arch != NULL, arch, error, NULL);

  status = archive_read_support_format_all (arch);
  dm_libarchive_set_error_and_return_if_fail (status == ARCHIVE_OK, arch, error, NULL);

  client_data_stream = dm_content_get_content_stream (DM_CONTENT (self), error);
  g_return_val_if_fail (client_data_stream != NULL, NULL);

  client_data_buffer = g_malloc (DM_ARCHIVE_CLIENT_DATA_BUFFER_SIZE);
  client_data.stream = G_INPUT_STREAM (client_data_stream);
  client_data.buffer = client_data_buffer;

  archive_read_open2(arch, &client_data,
                     _archive_open_callback,
                     _archive_read_callback,
                     _archive_skip_callback,
                     _archive_close_callback);
  for (;;)
    {
      status = archive_read_next_header(arch, &arch_entry);

      if (status == ARCHIVE_EOF)
        break;
      if (status == ARCHIVE_RETRY)
        continue;
      if (status == ARCHIVE_WARN)
        g_printerr("%s\n", archive_error_string (arch));
      if (status < ARCHIVE_WARN)
        g_set_error_literal (error,
                             dm_content_error_quark (),
                             archive_errno (arch),
                             archive_error_string (arch));

      if (!g_strcmp0 (archive_entry_pathname (arch_entry), member_name))
        {
          member_stream = g_memory_input_stream_new ();
          void *read_buffer = g_malloc (DM_ARCHIVE_CLIENT_DATA_BUFFER_SIZE);
          gsize read_size;
          gsize total_read_size = 0;
          gsize size_to_read = archive_entry_size (arch_entry);

          while (total_read_size < size_to_read)
            {
              read_size = archive_read_data (arch, read_buffer,
                                             DM_ARCHIVE_CLIENT_DATA_BUFFER_SIZE);

              g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (member_stream),
                                              g_memdup (read_buffer, read_size),
                                              read_size, g_free);
              total_read_size += read_size;
            }

          g_free (read_buffer);

          break;
        }
    }

  archive_read_free (arch);

  return member_stream;
}

/**
 * dm_article_new_from_json_node:
 * @node: a json node with the model metadata
 *
 * Instantiates a #DmArticle from a JsonNode of object metadata.
 * Outside of testing this metadata is usually retrieved from a shard.
 *
 * Returns: The newly created #DmArticle.
 */
DmContent *
dm_article_new_from_json_node (JsonNode *node)
{
  g_autoptr(EosProfileProbe) probe = EOS_PROFILE_PROBE ("/dmodel/object/article");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GArray *params = g_array_new (FALSE, TRUE, sizeof (GParameter));
  dm_article_add_json_to_params (node, params);
  DmArticle *model = g_object_newv (DM_TYPE_ARTICLE, params->len,
                                    (GParameter *)params->data);
  dm_utils_free_gparam_array (params);
G_GNUC_END_IGNORE_DEPRECATIONS

  return DM_CONTENT (model);
}
