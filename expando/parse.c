/**
 * @file
 * Expando Parsing
 *
 * @authors
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
 * Copyright (C) 2023-2024 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page expando_parse Expando Parsing
 *
 * Turn a format string into a tree of Expando Nodes.
 */

#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "parse.h"
#include "definition.h"
#include "helpers.h"
#include "node.h"
#include "node_condbool.h"
#include "node_condition.h"
#include "node_expando.h"
#include "node_padding.h"
#include "node_text.h"

/**
 * skip_until_if_true_end - Search for the end of an 'if true' condition
 * @param start          Start of string
 * @param end_terminator Terminator character
 * @retval ptr Position of terminator character, or end-of-string
 */
static const char *skip_until_if_true_end(const char *start, char end_terminator)
{
  int ctr = 0;
  while (*start)
  {
    if ((ctr == 0) && ((*start == end_terminator) || (*start == '&')))
    {
      break;
    }

    // handle nested if-else-s
    if (*start == '<')
    {
      ctr++;
    }

    if (*start == '>')
    {
      ctr--;
    }

    start++;
  }

  return start;
}

/**
 * skip_until_if_false_end - Search for the end of an 'if false' condition
 * @param start          Start of string
 * @param end_terminator Terminator character
 * @retval ptr Position of terminator character, or end-of-string
 */
static const char *skip_until_if_false_end(const char *start, char end_terminator)
{
  int ctr = 0;
  while (*start)
  {
    if ((ctr == 0) && (*start == end_terminator))
    {
      break;
    }

    // handle nested if-esle-s
    if (*start == '<')
    {
      ctr++;
    }

    if (*start == '>')
    {
      ctr--;
    }

    start++;
  }

  return start;
}

/**
 * node_parse - Parse a format string into ExpandoNodes
 * @param[in]  str             Start of string to parse
 * @param[in]  end             End of string to parse
 * @param[in]  condition_start Flag for conditional expandos
 * @param[out] parsed_until    First character after parsed string
 * @param[in]  defs            Expando definitions
 * @param[out] error           Buffer for errors
 * @retval ptr Tree of ExpandoNodes representing the format string
 */
static struct ExpandoNode *node_parse(const char *str, const char *end,
                                      enum ExpandoConditionStart condition_start,
                                      const char **parsed_until,
                                      const struct ExpandoDefinition *defs,
                                      struct ExpandoParseError *error)
{
  while (*str && (end ? (str <= end) : 1))
  {
    // %X -> expando
    // if there is condition like <X..., the `%` is implicit
    if ((*str == '%') ||
        ((condition_start == CON_START) && ((*str == '?') || (*str == '<'))))
    {
      str++;

      // %% -> "%"s
      if (*str == '%')
      {
        *parsed_until = str + 1;
        return node_text_new(str, str + 1);
      }
      // conditional
      else if ((*str == '?') || (*str == '<'))
      {
        bool old_style = (*str == '?');
        char end_terminator = old_style ? '?' : '>';

        const char *cond_end = skip_until_ch(str, '?');
        const char *next = NULL;
        struct ExpandoNode *condition = node_parse(str, cond_end, CON_START,
                                                   &next, defs, error);
        if (!condition)
          return NULL;

        if (*next != '?')
        {
          error->position = next;
          snprintf(error->message, sizeof(error->message),
                   // L10N: Expando is missing a terminator character
                   //       e.g. "%[..." is missing the final ']'
                   _("Conditional expando is missing '%c'"), '?');
          node_free(&condition);
          return NULL;
        }

        str = next + 1;

        const char *if_true_start = str;
        // nested if-else only allowed in the new style
        const char *if_true_end = skip_until_if_true_end(str, end_terminator);
        bool only_true = (*if_true_end == end_terminator);
        bool invalid = ((*if_true_end != '&') && !only_true);

        if (invalid)
        {
          error->position = if_true_end;
          snprintf(error->message, sizeof(error->message),
                   // L10N: Expando is missing a terminator character
                   //       e.g. "%[..." is missing the final ']'
                   _("Conditional expando is missing '&' or '%c'"), end_terminator);
          node_free(&condition);
          return NULL;
        }

        const char *if_true_parsed = NULL;
        struct ExpandoNode *if_true_tree = NULL;

        while (if_true_start < if_true_end)
        {
          struct ExpandoNode *node = node_parse(if_true_start, if_true_end, CON_NO_CONDITION,
                                                &if_true_parsed, defs, error);
          if (!node)
          {
            node_free(&condition);
            return NULL;
          }

          node_append(&if_true_tree, node);

          if_true_start = if_true_parsed;
        }

        if ((if_true_start == if_true_end) && !if_true_tree)
        {
          if_true_tree = node_new();
        }

        if (only_true)
        {
          *parsed_until = if_true_end + 1;
          return node_condition_new(condition, if_true_tree, NULL);
        }
        else
        {
          const char *if_false_start = if_true_end + 1;
          // nested if-else only allowed in the new style
          const char *if_false_end = skip_until_if_false_end(if_false_start, end_terminator);

          if (*if_false_end != end_terminator)
          {
            error->position = if_false_start;
            snprintf(error->message, sizeof(error->message),
                     // L10N: Expando is missing a terminator character
                     //       e.g. "%[..." is missing the final ']'
                     _("Conditional expando is missing '%c'"), end_terminator);
            node_free(&if_true_tree);
            node_free(&condition);
            return NULL;
          }

          const char *if_false_parsed = NULL;
          struct ExpandoNode *if_false_tree = NULL;

          while (if_false_start < if_false_end)
          {
            struct ExpandoNode *node = node_parse(if_false_start, if_false_end, CON_NO_CONDITION,
                                                  &if_false_parsed, defs, error);
            if (!node)
            {
              node_free(&if_true_tree);
              node_free(&condition);
              return NULL;
            }

            node_append(&if_false_tree, node);

            if_false_start = if_false_parsed;
          }

          if ((if_false_start == if_false_end) && !if_false_tree)
          {
            if_false_tree = node_new();
          }

          *parsed_until = if_false_end + 1;
          return node_condition_new(condition, if_true_tree, if_false_tree);
        }
      }
      // expando
      else
      {
        ExpandoParserFlags flags = (condition_start == CON_START) ? EP_CONDITIONAL : EP_NO_FLAGS;
        struct ExpandoNode *node = NULL;
        if (flags & EP_CONDITIONAL)
        {
          node = node_condbool_parse(str, parsed_until, defs, flags, error);
        }
        else
        {
          node = node_expando_parse(str, parsed_until, defs, flags, error);
        }

        if (!node || error->position)
        {
          node_free(&node);
          return NULL;
        }

        return node;
      }
    }
    // text
    else
    {
      return node_text_parse(str, end, parsed_until);
    }
  }

  ASSERT(false && "Internal parsing error"); // LCOV_EXCL_LINE
  return NULL;
}

/**
 * node_tree_parse - Parse a format string into ExpandoNodes
 * @param[in,out] root   Parent ExpandoNode
 * @param[in]     string String to parse
 * @param[in]     defs   Expando definitions
 * @param[out]    error  Buffer for errors
 */
void node_tree_parse(struct ExpandoNode **root, const char *string,
                     const struct ExpandoDefinition *defs, struct ExpandoParseError *error)
{
  if (!string || !*string)
  {
    node_append(root, node_new());
    return;
  }

  const char *end = NULL;
  const char *start = string;

  while (*start)
  {
    struct ExpandoNode *node = node_parse(start, NULL, CON_NO_CONDITION, &end, defs, error);
    if (!node)
      break;

    node_append(root, node);
    start = end;
  }

  node_padding_repad(root);
}
