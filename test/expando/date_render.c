/**
 * @file
 * Test code for Expando Date Rendering
 *
 * @authors
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
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

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mutt/lib.h"
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep
#include "test_common.h"

struct SimpleDateData
{
  time_t t;
};

static void simple_date(const struct ExpandoNode *node, void *data,
                        MuttFormatFlags flags, int max_cols, struct Buffer *buf)
{
  ASSERT(node->type == ENT_EXPANDO);

  const struct SimpleDateData *dd = data;
  struct tm tm = mutt_date_localtime(dd->t);

  char tmp[128] = { 0 };
  char tmp2[128] = { 0 };

  const int len = node->end - node->start;
  memcpy(tmp2, node->start, len);

  strftime(tmp, sizeof(tmp), tmp2, &tm);
  buf_strcpy(buf, tmp);
}

void test_expando_date_render(void)
{
  struct ExpandoParseError error = { 0 };

  struct SimpleDateData data = {
    .t = 1457341200,
  };

  {
    const char *input = "%[%Y-%m-%d] date";

    struct ExpandoNode *root = NULL;

    const struct ExpandoDefinition defs[] = {
      { "[", NULL, 1, 0, 0, parse_date },
      { NULL, NULL, 0, 0, 0, NULL },
    };

    node_tree_parse(&root, input, defs, &error);

    TEST_CHECK(error.position == NULL);

    check_node_expando(get_nth_node(root, 0), "%Y-%m-%d", NULL);
    check_node_test(get_nth_node(root, 1), " date");

    const char *expected = "2016-03-07 date";

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { 1, 0, simple_date },
      { -1, -1, NULL },
    };

    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, MUTT_FORMAT_NO_FLAGS, buf->dsize, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);

    node_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "%-12[%Y-%m-%d]";

    struct ExpandoNode *root = NULL;

    const struct ExpandoDefinition defs[] = {
      { "[", NULL, 1, 0, 0, parse_date },
      { NULL, NULL, 0, 0, 0, NULL },
    };

    node_tree_parse(&root, input, defs, &error);

    TEST_CHECK(error.position == NULL);

    struct ExpandoFormat fmt = { 0 };
    fmt.min_cols = 12;
    fmt.max_cols = INT_MAX;
    fmt.justification = JUSTIFY_LEFT;
    fmt.leader = ' ';

    check_node_expando(get_nth_node(root, 0), "%Y-%m-%d", &fmt);

    const char *expected = "2016-03-07  ";

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { 1, 0, simple_date },
      { -1, -1, NULL },
    };

    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, MUTT_FORMAT_NO_FLAGS, buf->dsize, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);

    node_tree_free(&root);
    buf_pool_release(&buf);
  }
}
