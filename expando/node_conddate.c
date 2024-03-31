/**
 * @file
 * Expando Node for a Conditional Date
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
 * @page expando_node_conddate Conditional Date Node
 *
 * Expando Node for a Conditional Date
 */

#include "config.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mutt/lib.h"
#include "node_conddate.h"
#include "helpers.h"
#include "node.h"
#include "parse.h"
#include "render.h"

/**
 * node_conddate_private_new - Create new CondDate private data
 * @param count      Number of 'units' to count          
 * @param period     Units, e.g. 'd' Day or 'm' Month    
 * @retval ptr New CondDate private data
 */
struct NodeCondDatePrivate *node_conddate_private_new(int count, char period)
{
  struct NodeCondDatePrivate *priv = mutt_mem_calloc(1, sizeof(struct NodeCondDatePrivate));

  priv->count = count;
  priv->period = period;

  return priv;
}

/**
 * node_conddate_private_free - Free CondDate private data
 * @param ptr Data to free
 */
void node_conddate_private_free(void **ptr)
{
  if (!ptr || !*ptr)
    return;

  FREE(ptr);
}

/**
 * node_conddate_render - Render a CondDate Node - Implements ExpandoNode::render - @ingroup expando_render
 */
int node_conddate_render(const struct ExpandoNode *node,
                         const struct ExpandoRenderData *rdata, struct Buffer *buf,
                         int max_cols, void *data, MuttFormatFlags flags)
{
  ASSERT(node->type == ENT_CONDDATE);

  const struct ExpandoRenderData *rd_match = find_get_number(rdata, node->did, node->uid);
  ASSERT(rd_match && "Unknown UID");

  const long t_test = rd_match->get_number(node, data, flags);

  const struct NodeCondDatePrivate *priv = node->ndata;

  time_t t = mutt_date_now();
  struct tm tm = { 0 };
  gmtime_r(&t, &tm);

  switch (priv->period)
  {
    case 'y':
      tm.tm_year -= priv->count;
      break;

    case 'm':
      tm.tm_mon -= priv->count;
      break;

    case 'w':
      tm.tm_mday -= (7 * priv->count);
      break;

    case 'd':
      tm.tm_mday -= priv->count;
      break;

    case 'H':
      tm.tm_hour -= priv->count;
      break;

    case 'M':
      tm.tm_min -= priv->count;
      break;
  }

  const time_t t_cutoff = mktime(&tm);

  return (t_test > t_cutoff); // bool-ify
}

/**
 * node_conddate_new - Create a new CondDate ExpandoNode
 * @param count      Number of 'units' to count          
 * @param period     Units, e.g. 'd' Day or 'm' Month    
 * @param did        Domain ID
 * @param uid        Unique ID
 * @retval ptr New CondDate ExpandoNode
 */
struct ExpandoNode *node_conddate_new(int count, char period, int did, int uid)
{
  struct ExpandoNode *node = node_new();
  node->type = ENT_CONDDATE;
  node->did = did;
  node->uid = uid;
  node->render = node_conddate_render;

  node->ndata = node_conddate_private_new(count, period);
  node->ndata_free = node_conddate_private_free;

  return node;
}

/**
 * node_conddate_parse - Parse a CondDate format string - Implements ExpandoDefinition::parse - @ingroup expando_parse_api
 */
struct ExpandoNode *node_conddate_parse(const char *str, const char **parsed_until,
                                        int did, int uid, struct ExpandoParseError *error)
{
  int count = 1;
  char period = '\0';

  if (isdigit(*str))
  {
    unsigned short number = 0;
    const char *end_ptr = mutt_str_atous(str, &number);

    // NOTE(g0mb4): str is NOT null-terminated
    if (!end_ptr || (number == USHRT_MAX))
    {
      error->position = str;
      snprintf(error->message, sizeof(error->message), _("Invalid number: %s"), str);
      return NULL;
    }

    count = number;
    str = end_ptr;
  };

  // Allowed periods: year, month, week, day, hour, minute
  if (!strchr("ymwdHM", *str))
  {
    error->position = str;
    snprintf(error->message, sizeof(error->message),
             // L10N: The 'ymwdHM' should not be translated
             _("Invalid time period: '%c', must be one of 'ymwdHM'"), *str);
    return NULL;
  }

  period = *str;
  *parsed_until = str + 1;

  return node_conddate_new(count, period, did, uid);
}
