/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  (C) 2002  David Woodhouse <dwmw2@infradead.org>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  linux/lib/rbtree.c
*/

#include <scaraOS/kernel.h>

static void __rb_rotate(struct rb_node *node, struct rb_root *root,
				unsigned int side)
{
	struct rb_node *other = node->rb_child[RB_RIGHT ^ side];
	struct rb_node *parent = rb_parent(node);

	if ((node->rb_child[RB_RIGHT ^ side] = 
			other->rb_child[RB_LEFT ^ side]))
		rb_set_parent(other->rb_child[RB_LEFT ^ side], node);
	other->rb_child[RB_LEFT ^ side] = node;

	rb_set_parent(other, parent);

	if (parent)
	{
		if (node == parent->rb_child[RB_LEFT ^ side])
			parent->rb_child[RB_LEFT ^ side] = other;
		else
			parent->rb_child[RB_RIGHT ^ side] = other;
	}
	else
		root->rb_node = other;
	rb_set_parent(node, other);
}

static void __rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
	__rb_rotate(node, root, RB_LEFT);
}
static void __rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
	__rb_rotate(node, root, RB_RIGHT);
}

void rb_insert_color(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *parent, *gparent;

	while ((parent = rb_parent(node)) && rb_is_red(parent))
	{
		gparent = rb_parent(parent);

		if (parent == gparent->rb_child[RB_LEFT])
		{
			{
				register struct rb_node *uncle = gparent->rb_child[RB_RIGHT];
				if (uncle && rb_is_red(uncle))
				{
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rb_child[RB_RIGHT] == node)
			{
				register struct rb_node *tmp;
				__rb_rotate_left(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_right(gparent, root);
		} else {
			{
				register struct rb_node *uncle = gparent->rb_child[RB_LEFT];
				if (uncle && rb_is_red(uncle))
				{
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rb_child[RB_LEFT] == node)
			{
				register struct rb_node *tmp;
				__rb_rotate_right(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_left(gparent, root);
		}
	}

	rb_set_black(root->rb_node);
}

static void __rb_erase_color(struct rb_node *node, struct rb_node *parent,
			     struct rb_root *root)
{
	struct rb_node *other;

	while ((!node || rb_is_black(node)) && node != root->rb_node)
	{
		if (parent->rb_child[RB_LEFT] == node)
		{
			other = parent->rb_child[RB_RIGHT];
			if (rb_is_red(other))
			{
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_left(parent, root);
				other = parent->rb_child[RB_RIGHT];
			}
			if ((!other->rb_child[RB_LEFT] || rb_is_black(other->rb_child[RB_LEFT])) &&
			    (!other->rb_child[RB_RIGHT] || rb_is_black(other->rb_child[RB_RIGHT])))
			{
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			}
			else
			{
				if (!other->rb_child[RB_RIGHT] || rb_is_black(other->rb_child[RB_RIGHT]))
				{
					rb_set_black(other->rb_child[RB_LEFT]);
					rb_set_red(other);
					__rb_rotate_right(other, root);
					other = parent->rb_child[RB_RIGHT];
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rb_child[RB_RIGHT]);
				__rb_rotate_left(parent, root);
				node = root->rb_node;
				break;
			}
		}
		else
		{
			other = parent->rb_child[RB_LEFT];
			if (rb_is_red(other))
			{
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_right(parent, root);
				other = parent->rb_child[RB_LEFT];
			}
			if ((!other->rb_child[RB_LEFT] || rb_is_black(other->rb_child[RB_LEFT])) &&
			    (!other->rb_child[RB_RIGHT] || rb_is_black(other->rb_child[RB_RIGHT])))
			{
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			}
			else
			{
				if (!other->rb_child[RB_LEFT] || rb_is_black(other->rb_child[RB_LEFT]))
				{
					rb_set_black(other->rb_child[RB_RIGHT]);
					rb_set_red(other);
					__rb_rotate_left(other, root);
					other = parent->rb_child[RB_LEFT];
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rb_child[RB_LEFT]);
				__rb_rotate_right(parent, root);
				node = root->rb_node;
				break;
			}
		}
	}
	if (node)
		rb_set_black(node);
}

void rb_erase(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *child, *parent;
	int color;

	if (!node->rb_child[RB_LEFT])
		child = node->rb_child[RB_RIGHT];
	else if (!node->rb_child[RB_RIGHT])
		child = node->rb_child[RB_LEFT];
	else
	{
		struct rb_node *old = node, *left;

		node = node->rb_child[RB_RIGHT];
		while ((left = node->rb_child[RB_LEFT]) != NULL)
			node = left;

		if (rb_parent(old)) {
			if (rb_parent(old)->rb_child[RB_LEFT] == old)
				rb_parent(old)->rb_child[RB_LEFT] = node;
			else
				rb_parent(old)->rb_child[RB_RIGHT] = node;
		} else
			root->rb_node = node;

		child = node->rb_child[RB_RIGHT];
		parent = rb_parent(node);
		color = rb_color(node);

		if (parent == old) {
			parent = node;
		} else {
			if (child)
				rb_set_parent(child, parent);
			parent->rb_child[RB_LEFT] = child;

			node->rb_child[RB_RIGHT] = old->rb_child[RB_RIGHT];
			rb_set_parent(old->rb_child[RB_RIGHT], node);
		}

		node->rb_parent_color = old->rb_parent_color;
		node->rb_child[RB_LEFT] = old->rb_child[RB_LEFT];
		rb_set_parent(old->rb_child[RB_LEFT], node);

		goto color;
	}

	parent = rb_parent(node);
	color = rb_color(node);

	if (child)
		rb_set_parent(child, parent);
	if (parent)
	{
		if (parent->rb_child[RB_LEFT] == node)
			parent->rb_child[RB_LEFT] = child;
		else
			parent->rb_child[RB_RIGHT] = child;
	}
	else
		root->rb_node = child;

 color:
	if (color == RB_BLACK)
		__rb_erase_color(child, parent, root);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
struct rb_node *rb_first(const struct rb_root *root)
{
	struct rb_node	*n;

	n = root->rb_node;
	if (!n)
		return NULL;
	while (n->rb_child[RB_LEFT])
		n = n->rb_child[RB_LEFT];
	return n;
}

struct rb_node *rb_last(const struct rb_root *root)
{
	struct rb_node	*n;

	n = root->rb_node;
	if (!n)
		return NULL;
	while (n->rb_child[RB_RIGHT])
		n = n->rb_child[RB_RIGHT];
	return n;
}

struct rb_node *rb_next(const struct rb_node *node)
{
	struct rb_node *parent;

	if (rb_parent(node) == node)
		return NULL;

	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rb_child[RB_RIGHT]) {
		node = node->rb_child[RB_RIGHT]; 
		while (node->rb_child[RB_LEFT])
			node=node->rb_child[RB_LEFT];
		return (struct rb_node *)node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while ((parent = rb_parent(node)) && node == parent->rb_child[RB_RIGHT])
		node = parent;

	return parent;
}

struct rb_node *rb_prev(const struct rb_node *node)
{
	struct rb_node *parent;

	if (rb_parent(node) == node)
		return NULL;

	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rb_child[RB_LEFT]) {
		node = node->rb_child[RB_LEFT]; 
		while (node->rb_child[RB_RIGHT])
			node=node->rb_child[RB_RIGHT];
		return (struct rb_node *)node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while ((parent = rb_parent(node)) && node == parent->rb_child[RB_LEFT])
		node = parent;

	return parent;
}

void rb_replace_node(struct rb_node *victim, struct rb_node *new,
		     struct rb_root *root)
{
	struct rb_node *parent = rb_parent(victim);

	/* Set the surrounding nodes to point to the replacement */
	if (parent) {
		if (victim == parent->rb_child[RB_LEFT])
			parent->rb_child[RB_LEFT] = new;
		else
			parent->rb_child[RB_RIGHT] = new;
	} else {
		root->rb_node = new;
	}
	if (victim->rb_child[RB_LEFT])
		rb_set_parent(victim->rb_child[RB_LEFT], new);
	if (victim->rb_child[RB_RIGHT])
		rb_set_parent(victim->rb_child[RB_RIGHT], new);

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;
}
