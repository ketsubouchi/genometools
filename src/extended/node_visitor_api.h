/*
  Copyright (c) 2006-2011 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2008 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef NODE_VISITOR_API_H
#define NODE_VISITOR_API_H

/* The <GtNodeVisitor> interface, a visitor for <GtGenomeNode>s. */
typedef struct GtNodeVisitor GtNodeVisitor;

#include "extended/comment_node_api.h"
#include "extended/eof_node_api.h"
#include "extended/feature_node.h"
#include "extended/region_node.h"
#include "extended/sequence_node_api.h"

/* Visit <comment_node> with <node_visitor>. */
int   gt_node_visitor_visit_comment_node(GtNodeVisitor *node_visitor,
                                         GtCommentNode *comment_node,
                                         GtError *err);
/* Visit <feature_node> with <node_visitor>. */
int   gt_node_visitor_visit_feature_node(GtNodeVisitor *node_visitor,
                                         GtFeatureNode *feature_node,
                                         GtError *err);
/* Visit <region_node> with <node_visitor>. */
int   gt_node_visitor_visit_region_node(GtNodeVisitor *node_visitor,
                                        GtRegionNode *region_node,
                                        GtError *err);
/* Visit <sequence_node> with <node_visitor>. */
int   gt_node_visitor_visit_sequence_node(GtNodeVisitor *node_visitor,
                                          GtSequenceNode *sequence_node,
                                          GtError *err);
/* Delete <node_visitor>. */
void  gt_node_visitor_delete(GtNodeVisitor *node_visitor);

#endif
