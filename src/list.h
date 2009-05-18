/*
 * Copyright (C) 2009 Daiki Ueno <ueno@unixuser.org>
 * This file is part of libusg.
 *
 * libusg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libusg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIST_H
#define __LIST_H

#include <stddef.h>		/* offsetof */

struct usg_list_head {
  struct usg_list_head *next, *prev;
};

/* copied and renamed from linux/include/linux/kernel.h */
#define usg_container_of(ptr, type, member) ({			\
      const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
      (type *)( (char *)__mptr - offsetof(type,member) );})

#define usg_list_entry(ptr, type, member)	\
  usg_container_of(ptr, type, member)

#define usg_list_for_each_entry(pos, head, member)			\
  for (pos = usg_list_entry((head)->next, typeof(*pos), member);	\
       pos->member.next, &pos->member != (head);			\
       pos = usg_list_entry(pos->member.next, typeof(*pos), member))

static inline void usg_init_list_head (struct usg_list_head *list)
{
  list->next = list;
  list->prev = list;
}

static inline void __usg_list_add (struct usg_list_head *new,
				   struct usg_list_head *prev,
				   struct usg_list_head *next)
{
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

static inline void usg_list_add (struct usg_list_head *new,
				 struct usg_list_head *head)
{
  __usg_list_add (new, head, head->next);
}

static inline void usg_list_add_tail (struct usg_list_head *new,
				      struct usg_list_head *head)
{
  __usg_list_add (new, head->prev, head);
}

static inline void usg_list_del (struct usg_list_head *head)
{
  head->next->prev = head->prev;
  head->prev->next = head->next;
}

static inline int usg_list_empty (struct usg_list_head *head)
{
  return head->next == head;
}

#endif
