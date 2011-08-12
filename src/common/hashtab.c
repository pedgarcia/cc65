/*****************************************************************************/
/*                                                                           */
/*                                 hashtab.c                                 */
/*                                                                           */
/*                             Generic hash table                            */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2003-2011, Ullrich von Bassewitz                                      */
/*                Roemerstrasse 52                                           */
/*                D-70794 Filderstadt                                        */
/* EMail:         uz@cc65.org                                                */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



/* common */
#include "check.h"
#include "hashtab.h"
#include "xmalloc.h"



/*****************************************************************************/
/*                             struct HashTable                              */
/*****************************************************************************/



HashTable* InitHashTable (HashTable* T, unsigned Slots, const HashFunctions* Func)
/* Initialize a hash table and return it */
{
    /* Initialize the fields */
    T->Slots    = Slots;
    T->Count    = 0;
    T->Table    = 0;
    T->Func     = Func;

    /* Return the initialized table */
    return T;
}



void DoneHashTable (HashTable* T)
/* Destroy the contents of a hash table. Note: This will not free the entries
 * in the table!
 */
{
    /* Just free the array with the table pointers */
    xfree (T->Table);
}



void FreeHashTable (HashTable* T)
/* Free a hash table. Note: This will not free the entries in the table! */
{
    if (T) {
        /* Free the contents */
        DoneHashTable (T);
        /* Free the table structure itself */
        xfree (T);
    }
}



static void HT_Alloc (HashTable* T)
/* Allocate table memory */
{
    unsigned I;

    /* Allocate memory */
    T->Table = xmalloc (T->Slots * sizeof (T->Table[0]));

    /* Initialize the table */
    for (I = 0; I < T->Slots; ++I) {
        T->Table[I] = 0;
    }
}



HashNode* HT_Find (const HashTable* T, const void* Key)
/* Find the node with the given index */
{
    /* If we don't have a table, there's nothing to find */
    if (T->Table == 0) {
        return 0;
    }

    /* Search for the entry */
    return HT_FindHash (T, Key, T->Func->GenHash (Key));
}



HashNode* HT_FindHash (const HashTable* T, const void* Key, unsigned Hash)
/* Find the node with the given key. Differs from HT_Find in that the hash
 * for the key is precalculated and passed to the function.
 */
{
    HashNode* N;

    /* If we don't have a table, there's nothing to find */
    if (T->Table == 0) {
        return 0;
    }

    /* Search for the entry in the given chain */
    N = T->Table[Hash % T->Slots];
    while (N) {

        /* First compare the full hash, to avoid calling the compare function
         * if it is not really necessary.
         */
        if (N->Hash == Hash &&
            T->Func->Compare (Key, T->Func->GetKey (N)) == 0) {
            /* Found */
            break;
        }

        /* Not found, next entry */
        N = N->Next;
    }

    /* Return what we found */
    return N;
}



void* HT_FindEntry (const HashTable* T, const void* Key)
/* Find the node with the given index and return the corresponding entry */
{
    /* Since the HashEntry must be first member, we can use HT_Find here */
    return HT_Find (T, Key);
}



void HT_Insert (HashTable* T, HashNode* N)
/* Insert a node into the given hash table */
{
    unsigned RHash;

    /* If we don't have a table, we need to allocate it now */
    if (T->Table == 0) {
        HT_Alloc (T);
    }

    /* Generate the hash over the node key. */
    N->Hash = T->Func->GenHash (T->Func->GetKey (N));

    /* Calculate the reduced hash */
    RHash = N->Hash % T->Slots;

    /* Insert the entry into the correct chain */
    N->Next = T->Table[RHash];
    T->Table[RHash] = N;

    /* One more entry */
    ++T->Count;
}



void HT_Remove (HashTable* T, HashNode* N)
/* Remove a node from a hash table. */
{
    /* Calculate the reduced hash, which is also the slot number */
    unsigned Slot = N->Hash % T->Slots;

    /* Remove the entry from the single linked list */
    HashNode** Q = &T->Table[Slot];
    while (1) {
        /* If the pointer is NULL, the node is not in the table which we will
         * consider a serious error.
         */
        CHECK (*Q != 0);
        if (*Q == N) {
            /* Found - remove it */
            *Q = N->Next;
            break;
        }
        /* Next node */
        Q = &(*Q)->Next;
    }
}



void HT_InsertEntry (HashTable* T, void* Entry)
/* Insert an entry into the given hash table */
{
    /* Since the hash node must be first member, Entry is also the pointer to
     * the hash node.
     */
    HT_Insert (T, Entry);
}



void HT_RemoveEntry (HashTable* T, void* Entry)
/* Remove an entry from the given hash table */
{
    /* The entry is the first member, so we can just convert the pointer */
    HT_Remove (T, Entry);
}



void HT_Walk (HashTable* T, void (*F) (void* Entry, void* Data), void* Data)
/* Walk over all nodes of a hash table. For each node, the user supplied
 * function F is called, passing a pointer to the entry, and the data pointer
 * passed to HT_Walk by the caller.
 */
{
    unsigned I;

    /* If we don't have a table there are no entries to walk over */
    if (T->Table == 0) {
        return;
    }

    /* Walk over all chains */
    for (I = 0; I < T->Slots; ++I) {

        /* Get the pointer to the first entry of the hash chain */
        HashNode* N = T->Table[I];

        /* Walk over all entries in this chain */
        while (N) {
            /* Call the user function. N is also the pointer to the entry */
            F (N, Data);
            /* Next node in chain */
            N = N->Next;
        }

    }
}



