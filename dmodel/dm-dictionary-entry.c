/* Copyright 2017 Endless Mobile, Inc. */

#include "dm-dictionary-entry.h"

#include "dm-utils-private.h"
#include "dm-content-private.h"

#include <endless/endless.h>

/**
 * SECTION:dictionary-entry
 * @title: Dictionary entry model
 * @short_description: Access dictionary entry object metadata
 *
 * The model class for dictionary entry objects.
 */
typedef struct {
  gchar *word;
  gchar *definition;
  gchar *part_of_speech;
} DmDictionaryEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DmDictionaryEntry, dm_dictionary_entry,
                            DM_TYPE_CONTENT)

enum {
  PROP_0,
  PROP_WORD,
  PROP_DEFINITION,
  PROP_PART_OF_SPEECH,
  NPROPS
};

static GParamSpec *dm_dictionary_entry_props[NPROPS] = { NULL, };

static void
dm_dictionary_entry_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  DmDictionaryEntry *self = DM_DICTIONARY_ENTRY (object);
  DmDictionaryEntryPrivate *priv = dm_dictionary_entry_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_WORD:
      g_value_set_string (value, priv->word);
      break;

    case PROP_DEFINITION:
      g_value_set_string (value, priv->definition);
      break;

    case PROP_PART_OF_SPEECH:
      g_value_set_string (value, priv->part_of_speech);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dm_dictionary_entry_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  DmDictionaryEntry *self = DM_DICTIONARY_ENTRY (object);
  DmDictionaryEntryPrivate *priv = dm_dictionary_entry_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_WORD:
      priv->word = g_value_dup_string (value);
      break;

    case PROP_DEFINITION:
      priv->definition = g_value_dup_string (value);
      break;

    case PROP_PART_OF_SPEECH:
      priv->part_of_speech = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dm_dictionary_entry_finalize (GObject *object)
{
  DmDictionaryEntry *self = DM_DICTIONARY_ENTRY (object);
  DmDictionaryEntryPrivate *priv = dm_dictionary_entry_get_instance_private (self);

  g_clear_pointer (&priv->word, g_free);
  g_clear_pointer (&priv->definition, g_free);
  g_clear_pointer (&priv->part_of_speech, g_free);

  G_OBJECT_CLASS (dm_dictionary_entry_parent_class)->finalize (object);
}

static void
dm_dictionary_entry_class_init (DmDictionaryEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dm_dictionary_entry_get_property;
  object_class->set_property = dm_dictionary_entry_set_property;
  object_class->finalize = dm_dictionary_entry_finalize;

  /**
   * DmDictionaryEntry:word:
   *
   * The actual word
   */
  dm_dictionary_entry_props[PROP_WORD] =
    g_param_spec_string ("word", "Word",
      "The actual word",
      "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * DmDictionaryEntry:definition:
   *
   * The corresponding definition of the word
   */
  dm_dictionary_entry_props[PROP_DEFINITION] =
    g_param_spec_string ("definition", "Definition",
      "Definition of the word",
      "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * DmDictionaryEntry:part-of-speech:
   *
   * The part of speech the word does belong to e.g. noun, verb
   */
  dm_dictionary_entry_props[PROP_PART_OF_SPEECH] =
    g_param_spec_string ("part-of-speech", "Part of speech",
      "The part of speech the word does belong to",
      "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NPROPS,
                                     dm_dictionary_entry_props);
}

static void
dm_dictionary_entry_init (G_GNUC_UNUSED DmDictionaryEntry *self)
{
}

static void
dm_dictionary_entry_add_json_to_params (JsonNode *node,
                                        GArray *params)
{
  if (!JSON_NODE_HOLDS_OBJECT (node))
    {
      g_critical ("Trying to instantiate a DmDictionaryEntry from a non json object.");
      return;
    }

  dm_content_add_json_to_params (node, params);

  JsonObject *object = json_node_get_object (node);
  GObjectClass *klass = g_type_class_ref (DM_TYPE_DICTIONARY_ENTRY);

  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "word"),
                                         g_object_class_find_property (klass, "word"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "definition"),
                                         g_object_class_find_property (klass, "definition"),
                                         params);
  dm_utils_append_gparam_from_json_node (json_object_get_member (object, "partOfSpeech"),
                                         g_object_class_find_property (klass, "part-of-speech"),
                                         params);
  g_type_class_unref (klass);
}

/**
 * dm_dictionary_entry_new_from_json_node:
 * @node: a json node with the model metadata
 *
 * Instantiates a #DmDictionaryEntry from a JsonNode of object metadata.
 * Outside of testing this metadata is usually retrieved from a shard.
 *
 * Returns: The newly created #DmDictionaryEntry.
 */
DmContent *
dm_dictionary_entry_new_from_json_node (JsonNode *node)
{
  g_autoptr(EosProfileProbe) probe = EOS_PROFILE_PROBE ("/dmodel/object/dictionary");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GArray *params = g_array_new (FALSE, TRUE, sizeof (GParameter));
  dm_dictionary_entry_add_json_to_params (node, params);
  DmDictionaryEntry *model = g_object_newv (DM_TYPE_DICTIONARY_ENTRY,
                                            params->len,
                                            (GParameter *)params->data);
  dm_utils_free_gparam_array (params);
G_GNUC_END_IGNORE_DEPRECATIONS

  return DM_CONTENT (model);
}

