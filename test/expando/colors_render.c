/**
 * @file
 * Test code for Expando Colour Rendering
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
#include <stdio.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "color/lib.h"
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep
#include "mutt_thread.h"
#include "test_common.h"

struct SimpleExpandoData
{
  const char *s;
  int C;
};

static void simple_s(const struct ExpandoNode *node, void *data,
                     MuttFormatFlags flags, int max_cols, struct Buffer *buf)
{
  ASSERT(node->type == ENT_EXPANDO);

  const struct SimpleExpandoData *sd = data;
  struct NodeExpandoPrivate *priv = node->ndata;

  priv->color = MT_COLOR_INDEX_SUBJECT;

  const char *s = NONULL(sd->s);
  buf_strcpy(buf, s);
}

static void simple_C(const struct ExpandoNode *node, void *data,
                     MuttFormatFlags flags, int max_cols, struct Buffer *buf)
{
  ASSERT(node->type == ENT_EXPANDO);

  const struct SimpleExpandoData *sd = data;
  struct NodeExpandoPrivate *priv = node->ndata;

  priv->color = MT_COLOR_INDEX_NUMBER;

  const int num = sd->C;
  buf_printf(buf, "%d", num);
}

void test_expando_colors_render(void)
{
  {
    const char *input = "%C - %s";

    struct ExpandoNode *root = NULL;
    struct ExpandoParseError error = { 0 };

    const struct ExpandoDefinition defs[] = {
      // clang-format off
      { "*", "padding-soft", ED_GLOBAL, ED_GLO_PADDING_SOFT, E_TYPE_STRING, node_padding_parse },
      { ">", "padding-hard", ED_GLOBAL, ED_GLO_PADDING_HARD, E_TYPE_STRING, node_padding_parse },
      { "|", "padding-eol",  ED_GLOBAL, ED_GLO_PADDING_EOL,  E_TYPE_STRING, node_padding_parse },
      { "s", NULL, 1, 0, 0, NULL },
      { "C", NULL, 1, 1, 0, NULL },
      { NULL, NULL, 0, 0, 0, NULL },
      // clang-format on
    };

    node_tree_parse(&root, input, defs, &error);

    TEST_CHECK(error.position == NULL);
    check_node_expando(get_nth_node(root, 0), "C", NULL);
    check_node_test(get_nth_node(root, 1), " - ");
    check_node_expando(get_nth_node(root, 2), "s", NULL);

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { 1, 0, simple_s },
      { 1, 1, simple_C },
      { -1, -1, NULL },
    };

    struct SimpleExpandoData data = {
      .s = "Test",
      .C = 1,
    };

    char expected[] = "XX1XX - XXTestXX";
    expected[0] = MUTT_SPECIAL_INDEX;
    expected[1] = MT_COLOR_INDEX_NUMBER;
    expected[3] = MUTT_SPECIAL_INDEX;
    expected[4] = MT_COLOR_INDEX;
    expected[8] = MUTT_SPECIAL_INDEX;
    expected[9] = MT_COLOR_INDEX_SUBJECT;
    expected[14] = MUTT_SPECIAL_INDEX;
    expected[15] = MT_COLOR_INDEX;

    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, MUTT_FORMAT_INDEX, buf->dsize, buf);

    const int expected_width = mutt_str_len(expected) - 8;
    TEST_CHECK(mutt_strwidth(expected) == expected_width);
    TEST_CHECK_STR_EQ(buf_string(buf), expected);

    node_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "%C %* %s";

    struct ExpandoNode *root = NULL;
    struct ExpandoParseError error = { 0 };

    const struct ExpandoDefinition defs[] = {
      // clang-format off
      { "*", "padding-soft", ED_GLOBAL, ED_GLO_PADDING_SOFT, E_TYPE_STRING, node_padding_parse },
      { ">", "padding-hard", ED_GLOBAL, ED_GLO_PADDING_HARD, E_TYPE_STRING, node_padding_parse },
      { "|", "padding-eol",  ED_GLOBAL, ED_GLO_PADDING_EOL,  E_TYPE_STRING, node_padding_parse },
      { "s", NULL, 1, 0, 0, NULL },
      { "C", NULL, 1, 1, 0, NULL },
      { NULL, NULL, 0, 0, 0, NULL },
      // clang-format on
    };

    node_tree_parse(&root, input, defs, &error);

    TEST_CHECK(error.position == NULL);
    check_node_padding(root, " ", EPT_SOFT_FILL);

    struct ExpandoNode *left = node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_node_expando(get_nth_node(left, 0), "C", NULL);
    check_node_test(get_nth_node(left, 1), " ");
    check_node_expando(get_nth_node(right, 0), "s", NULL);

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { 1, 0, simple_s },
      { 1, 1, simple_C },
      { -1, -1, NULL },
    };

    struct SimpleExpandoData data = {
      .s = "Test",
      .C = 1,
    };

    char expected[] = "XX1XX   XXTestXX";
    expected[0] = MUTT_SPECIAL_INDEX;
    expected[1] = MT_COLOR_INDEX_NUMBER;
    expected[3] = MUTT_SPECIAL_INDEX;
    expected[4] = MT_COLOR_INDEX;
    expected[8] = MUTT_SPECIAL_INDEX;
    expected[9] = MT_COLOR_INDEX_SUBJECT;
    expected[14] = MUTT_SPECIAL_INDEX;
    expected[15] = MT_COLOR_INDEX;

    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, MUTT_FORMAT_INDEX, 8, buf);

    const int expected_width = mutt_str_len(expected) - 8;
    TEST_CHECK(mutt_strwidth(expected) == expected_width);
    TEST_CHECK_STR_EQ(buf_string(buf), expected);

    char expected2[] = "XX1XX XXTestXX";
    expected2[0] = MUTT_SPECIAL_INDEX;
    expected2[1] = MT_COLOR_INDEX_NUMBER;
    expected2[3] = MUTT_SPECIAL_INDEX;
    expected2[4] = MT_COLOR_INDEX;
    expected2[6] = MUTT_SPECIAL_INDEX;
    expected2[7] = MT_COLOR_INDEX_SUBJECT;
    expected2[12] = MUTT_SPECIAL_INDEX;
    expected2[13] = MT_COLOR_INDEX;

    buf_reset(buf);
    expando_render(&expando, render, &data, MUTT_FORMAT_INDEX, 6, buf);

    const int expected_width2 = mutt_str_len(expected2) - 8;
    TEST_CHECK(mutt_strwidth(expected2) == expected_width2);
    TEST_CHECK_STR_EQ(buf_string(buf), expected2);

    node_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "%s %* %s";

    struct ExpandoNode *root = NULL;
    struct ExpandoParseError error = { 0 };

    const struct ExpandoDefinition defs[] = {
      // clang-format off
      { "*", "padding-soft", ED_GLOBAL, ED_GLO_PADDING_SOFT, E_TYPE_STRING, node_padding_parse },
      { ">", "padding-hard", ED_GLOBAL, ED_GLO_PADDING_HARD, E_TYPE_STRING, node_padding_parse },
      { "|", "padding-eol",  ED_GLOBAL, ED_GLO_PADDING_EOL,  E_TYPE_STRING, node_padding_parse },
      { "s", NULL, 1, 0, 0, NULL },
      { "C", NULL, 1, 1, 0, NULL },
      { NULL, NULL, 0, 0, 0, NULL },
      // clang-format on
    };

    node_tree_parse(&root, input, defs, &error);

    TEST_CHECK(error.position == NULL);
    check_node_padding(root, " ", EPT_SOFT_FILL);

    struct ExpandoNode *left = node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_node_expando(get_nth_node(left, 0), "s", NULL);
    check_node_test(get_nth_node(left, 1), " ");
    check_node_expando(get_nth_node(right, 0), "s", NULL);

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { 1, 0, simple_s },
      { 1, 1, simple_C },
      { -1, -1, NULL },
    };

    struct SimpleExpandoData data = {
      .s = "Test",
      .C = 1,
    };

    char expected[] = "XXTeXXXXTestXX";
    expected[0] = MUTT_SPECIAL_INDEX;
    expected[1] = MT_COLOR_INDEX_SUBJECT;
    expected[4] = MUTT_SPECIAL_INDEX;
    expected[5] = MT_COLOR_INDEX;
    expected[6] = MUTT_SPECIAL_INDEX;
    expected[7] = MT_COLOR_INDEX_SUBJECT;
    expected[12] = MUTT_SPECIAL_INDEX;
    expected[13] = MT_COLOR_INDEX;

    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, MUTT_FORMAT_INDEX, 6, buf);

    const int expected_width = mutt_str_len(expected) - 8;
    TEST_CHECK(mutt_strwidth(expected) == expected_width);
    // TEST_CHECK_STR_EQ(buf_string(buf), expected);
    // TEST_MSG("Expected: %s", expected);
    // TEST_MSG("Actual:   %s", buf_string(buf));

    node_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "%s %* %s";

    struct ExpandoNode *root = NULL;
    struct ExpandoParseError error = { 0 };

    const struct ExpandoDefinition defs[] = {
      // clang-format off
      { "*", "padding-soft", ED_GLOBAL, ED_GLO_PADDING_SOFT, E_TYPE_STRING, node_padding_parse },
      { ">", "padding-hard", ED_GLOBAL, ED_GLO_PADDING_HARD, E_TYPE_STRING, node_padding_parse },
      { "|", "padding-eol",  ED_GLOBAL, ED_GLO_PADDING_EOL,  E_TYPE_STRING, node_padding_parse },
      { "s", NULL, 1, 0, 0, NULL },
      { "C", NULL, 1, 1, 0, NULL },
      { NULL, NULL, 0, 0, 0, NULL },
      // clang-format on
    };

    node_tree_parse(&root, input, defs, &error);

    TEST_CHECK(error.position == NULL);
    check_node_padding(root, " ", EPT_SOFT_FILL);

    struct ExpandoNode *left = node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_node_expando(get_nth_node(left, 0), "s", NULL);
    check_node_test(get_nth_node(left, 1), " ");
    check_node_expando(get_nth_node(right, 0), "s", NULL);

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { 1, 0, simple_s },
      { 1, 1, simple_C },
      { -1, -1, NULL },
    };

    struct SimpleExpandoData data = {
      .s = "Táéí",
      .C = 1,
    };

    char expected[] = "\x0e\x63Tá\x0e\x5b\x0e\x63Táéí\x0e\x5b";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, MUTT_FORMAT_INDEX, 6, buf);

    TEST_CHECK(mutt_strwidth(expected) == 6);
    // TEST_CHECK_STR_EQ(buf_string(buf), expected);
    // TEST_MSG("Expected: %s", expected);
    // TEST_MSG("Actual:   %s", buf_string(buf));

    node_tree_free(&root);
    buf_pool_release(&buf);
  }
}
