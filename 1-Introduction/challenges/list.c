/**
 * Implement a doubly linked-list
 * Functions: insert / find / delete node, print list
 * Allow user to initialize the list through command-line arguments
 * */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct node
{
    int value;
    struct node *prev;
    struct node *next;
} node;

void prependNode(node **list, int value);
void printList(node *list);
void freeList(node *list);
bool findNode(node *list, int value);
bool deleteNode(node **list, int value);

int main(int argc, char *argv[])
{
    node *list = NULL;

    // Initilize the list with command-line arguments
    for (int i = 1; i < argc; i++)
    {
        int value = atoi(argv[i]);
        prependNode(&list, value);
    }

    printList(list);

    int valueToFind = 4;
    bool found = findNode(list, valueToFind);
    if (found)
    {
        printf("%i found!\n", valueToFind);
    }
    else
    {
        printf("%i not found!\n", valueToFind);
    }

    int valueToDelete = 2;
    bool deleted = deleteNode(&list, valueToDelete);
    if (deleted)
    {
        printf("%i deleted!\n", valueToDelete);
    }
    else
    {
        printf("%i cannot be deleted!\n", valueToDelete);
    }

    printList(list);
    freeList(list);
}

void prependNode(node **list, int value)
{
    // Create new node
    node *n = malloc(sizeof(node));
    if (n == NULL)
    {
        // Free the list if memory allocation failed
        if (*list != NULL)
        {
            freeList(*list);
        }
        exit(1);
    }

    n->value = value;
    n->prev = NULL;
    n->next = NULL;

    // Prepend node to list
    if (*list != NULL)
    {
        n->next = *list;
        (*list)->prev = n;
    }
    *list = n;
}

void printList(node *list)
{
    node *current = list;
    printf("List: ");
    while (current != NULL)
    {
        printf("%i->", current->value);
        current = current->next;
    }
    printf("\n");
}

void freeList(node *list)
{
    node *current = list;
    while (current != NULL)
    {
        node *next = current->next;
        free(current);
        current = next;
    }
}

bool findNode(node *list, int value)
{
    node *current = list;
    while (current != NULL)
    {
        if (current->value == value)
        {
            return true;
        }
        current = current->next;
    }
    return false;
}

bool deleteNode(node **list, int value)
{
    node *current = *list;
    node *prev = NULL;

    while (current != NULL)
    {
        if (current->value == value)
        {
            if (prev == NULL)
            {
                // Handle the head of the list
                *list = current->next;
                if (current->next != NULL)
                {
                    current->next->prev = NULL;
                }
            }
            else
            {
                prev->next = current->next;
                if (current->next != NULL)
                {
                    current->next->prev = prev;
                }
            }

            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}