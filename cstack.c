#include "cstack.h"

#include <stdlib.h>
#include <string.h>

struct node
{
    struct node* prev;
    unsigned int size;
    char data[];
};

typedef struct node* stack_t;

struct stack_entry
{
    int reserved;
    stack_t stack;
};

typedef struct stack_entry stack_entry_t;

struct stack_entries_table
{
    unsigned int size;
    stack_entry_t* entries;
};

static struct stack_entries_table g_table = {0u, NULL};

static int upgrade_table(void)
{
    unsigned int new_size;
    if (g_table.size == 0u)
    {
        new_size = 2u;
    }
    else
    {
        new_size = g_table.size * 2u;
    }
    struct stack_entry* res =
        realloc(g_table.entries, new_size * sizeof(struct stack_entry));
    if (res == NULL)
    {
        return 0;
    }
    for (unsigned int i = g_table.size; i < new_size; ++i)
    {
        res[i].reserved = 0;
        res[i].stack = NULL;
    }
    g_table.entries = res;
    g_table.size = new_size;

    return 1;
}

static void free_nodes(stack_entry_t* entry)
{
    stack_t current_node = entry->stack;
    while (current_node != NULL)
    {
        stack_t prev_node = current_node->prev;
        free(current_node);
        current_node = prev_node;
    }
}

hstack_t stack_new(void)
{
    for (unsigned int i = 0; i < g_table.size; ++i)
    {
        if (g_table.entries[i].reserved == 0)
        {
            g_table.entries[i].reserved = 1;
            return i;
        }
    }
    unsigned int old_size = g_table.size;
    int res = upgrade_table();
    if (!res)
    {
        return -1;
    }
    g_table.entries[old_size].reserved = 1;

    return old_size;
}

void stack_free(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return;
    }
    free_nodes(&g_table.entries[hstack]);

    g_table.entries[hstack].reserved = 0;
    g_table.entries[hstack].stack = NULL;
}

void stack_free_all(void)
{
    if (g_table.entries == NULL)
    {
        return;
    }
    for (unsigned int i = 0; i < g_table.size; ++i)
    {
        if (g_table.entries[i].reserved == 1)
        {
            free_nodes(&g_table.entries[i]);
        }
    }
    free(g_table.entries);
    g_table.size = 0;
    g_table.entries = NULL;
}

int stack_valid_handler(const hstack_t hstack)
{
    if (hstack < 0)
    {
        return 0;
    }
    if ((unsigned int)hstack >= g_table.size)
    {
        return 0;
    }
    if (g_table.entries[hstack].reserved != 1)
    {
        return 0;
    }
    return 1;
}

unsigned int stack_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return 0;
    }
    stack_t current_node = g_table.entries[hstack].stack;
    unsigned int count = 0;
    while (current_node != NULL)
    {
        current_node = current_node->prev;
        ++count;
    }
    return count;
}

void
stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    if (!stack_valid_handler(hstack) || data_in == NULL || size == 0)
    {
        return;
    }
    stack_t node = malloc(sizeof(struct node) + size);
    if (!node)
    {
        return;
    }
    node->size = size;
    memcpy(node->data, data_in, size);
    node->prev = g_table.entries[hstack].stack;
    g_table.entries[hstack].stack = node;
}

unsigned int
stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    const unsigned int top_size = stack_top_size(hstack);
    if (!top_size || data_out == NULL || size == 0)
    {
        return 0;
    }
    if (size < top_size)
    {
        return 0;
    }
    memcpy(data_out, g_table.entries[hstack].stack->data, top_size);
    stack_t current_node = g_table.entries[hstack].stack;
    stack_t prev_node = current_node->prev;
    g_table.entries[hstack].stack = prev_node;
    free(current_node);

    return top_size;
}

unsigned stack_top_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return 0;
    }
    if (g_table.entries[hstack].stack == NULL)
    {
        return 0;
    }
    return g_table.entries[hstack].stack->size;
}

hstack_t stack_get_all(void)
{
    hstack_t hstack = stack_new();
    if (hstack == -1)
    {
        return -1;
    }
    for (unsigned int i = 0; i < g_table.size; ++i)
    {
        if (g_table.entries[i].reserved == 1 && i != (unsigned int)hstack)
        {
            hstack_t data = i;
            unsigned int size = stack_size(hstack);
            stack_push(hstack, &data, sizeof(hstack_t));
            if (size == stack_size(hstack))
            {
                stack_free(hstack);
                return -1;
            }
        }
    }
    return hstack;
}
