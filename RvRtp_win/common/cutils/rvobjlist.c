/***********************************************************************
Filename   : rvobjlist.c
Description: utility for building lists of objects (structures)
************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/
#include "rvobjlist.h"



/********************************************************************************************
 * RvObjListConstruct
 * Constructs an object list.
 * INPUT   : itemtmp - Pointer to an object of the type to be stored in the list.
 *           elementptr - Pointer to the element within itemtmp to use for this list.
 * OUTPUT  : objlist - Pointer to object list object to construct.
 * RETURN  : A pointer to the object list object or, if there is an error, NULL.
 * NOTE    : The itemtmp and elementptr pointers are simply used to find the offset
 *           within the structure where the RvObjListElement element is located.
 *           Anything the itemtmp pointer may point is is never touched.}
 *           For a more complicated structure, the RvObjListElement element can
 *           be placed outside the actual structure that will be in the list.
 *           It can even be before the structure itself, resulting in a negative
 *           offset. As long as the offset between the pointer passed in for an
 *           item and the location of the RvObjListElement data is constant, it
 *           will work.
 *           The template object is never touched and is simply used to find the
 *           location of the RvObjListElement structure (elementptr) within it.
 *           It is possible to put the Element structure outside of the the object
 *           and have a negative offset between the two.
 */
RVCOREAPI
RvObjList * RVCALLCONV RvObjListConstruct(
    IN void *itemtemp,
    IN RvObjListElement *elementptr,
    OUT RvObjList *objlist)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((objlist == NULL) || (itemtemp == NULL) || (elementptr == NULL))
        return NULL;
#endif
    objlist->offset = (RvInt8 *)elementptr - (RvInt8 *)itemtemp;

    return RvObjListReset(objlist);
}





/********************************************************************************************
 * RvObjListSize
 * Get the current size of an object list.
 * INPUT   : objlist - Pointer to object list to get size of.
 * OUTPUT  : None
 * RETURN  : The size of the object list.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjListSize(
    IN RvObjList *objlist)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objlist == NULL)
        return 0;
#endif
    return(objlist->count);
}


/********************************************************************************************
 * RvObjListInsertAfter
 * Inserts a new object into the list after a specified item.
 * INPUT   : objlist - Pointer to object list to insert new item into.
 *           curitem - Pointer to item to insert new item after (NULL puts new item at beginning of list.
 * OUTPUT  : newitem - Pointer to new item to be inserted into the list.
 * RETURN  : A pointer to newitem or NULL if the item could not be inserted.
 * NOTE    : The curitem item must be in the list because that condition will not
 *           be detected and will cause major problems.
 *           If curitem is NULL than item is put at start of list.
 */
RVCOREAPI
void * RVCALLCONV RvObjListInsertAfter(
    IN RvObjList *objlist,
    IN void *curitem,
    OUT void *newitem)
{
    RvObjListElement *curelem, *newelem;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((objlist == NULL) || (newitem == NULL))
        return NULL;
#endif

    objlist->count += 1;
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(objlist->count == 0) {
        /* We wrapped, which means we can't add anything */
        objlist->count -= 1;
        return NULL;
    }
#endif

    if(curitem != NULL) {
        curelem = (RvObjListElement *)((RvInt8 *)curitem + objlist->offset);
    } else curelem = &objlist->anchor; /* put at start of list */
    newelem = (RvObjListElement *)((RvInt8 *)newitem + objlist->offset);
    newelem->obj = newitem;
    newelem->prev = curelem;
    newelem->next = curelem->next;
    curelem->next = newelem;
    newelem->next->prev = newelem;

    return newitem;
}


/********************************************************************************************
 * RvObjListInsertBefore
 * Inserts a new object into the list before a specified item.
 * INPUT   : objlist - Pointer to object list to insert new item into.
 *           curitem - Pointer to item to insert new item after (NULL puts new item at the end of list.
 * OUTPUT  : newitem - Pointer to new item to be inserted into the list.
 * RETURN  : A pointer to newitem or NULL if the item could not be inserted.
 * NOTE    : The curitem item must be in the list because that condition will not
 *           be detected and will cause major problems.
 *           If curitem is NULL than item is put at end of list.
 */
RVCOREAPI
void * RVCALLCONV RvObjListInsertBefore(
    IN RvObjList *objlist,
    IN void *curitem,
    OUT void *newitem)
{
    RvObjListElement *curelem, *newelem;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((objlist == NULL) || (newitem == NULL))
        return NULL;
#endif

    objlist->count += RvUintConst(1);
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(objlist->count == 0) {
        /* We wrapped, which means we can't add anything */
        objlist->count -= 1;
        return NULL;
    }
#endif

    if(curitem != NULL) {
        curelem = (RvObjListElement *)((RvInt8 *)curitem + objlist->offset);
    } else curelem = &objlist->anchor; /* put at end of list */
    newelem = (RvObjListElement *)((RvInt8 *)newitem + objlist->offset);
    newelem->obj = newitem;
    newelem->next = curelem;
    newelem->prev = curelem->prev;
    curelem->prev = newelem;
    newelem->prev->next = newelem;

    return newitem;
}


/********************************************************************************************
 * RvObjListGetNext
 * Gets the next item after a particular item in a list.
 * INPUT   : objlist - Pointer to object list to insert new item into.
 *           curitem - Pointer to item to insert new item after (NULL puts new item at beginning of list.
 *           removeitem - Set to RV_OBJLIST_REMOVE to remove the item from the list or set it
 *                                  to RV_OBJLIST_LEAVE to leave the item in the list.
 * OUTPUT  : None.
 * RETURN  : A pointer to the next item or NULL if curitem was the last item in the list.
 * NOTE    : The curitem item must be in the list because that
 *           condition will not be detected and will cause major problems.
 */
RVCOREAPI
void * RVCALLCONV RvObjListGetNext(
    IN RvObjList *objlist,
    IN void *curitem,
    IN RvBool removeitem)
{
    RvObjListElement *elem;
    void *item;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objlist == NULL)
        return NULL;
#endif

    if(curitem != NULL) {
        elem = (RvObjListElement *)((RvInt8 *)curitem + objlist->offset);
        elem = elem->next;
    } else elem = objlist->anchor.next; /* Get first item */
    item = elem->obj;
    if((removeitem == RV_OBJLIST_REMOVE) && (item != NULL)) {
        /* Only remove the item if requested and its not the anchor */
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
        objlist->count -= 1;
    }

    return item;
}


/********************************************************************************************
 * RvObjListGetPrevious
 * Gets the previous item before a particular item in a list.
 * INPUT   : objlist - Pointer to object list to insert new item into.
 *           curitem - Pointer to item to insert new item after (NULL puts new item at beginning of list.
 *           removeitem - Set to RV_OBJLIST_REMOVE to remove the item from the list or set it
 *                                  to RV_OBJLIST_LEAVE to leave the item in the list.
 * OUTPUT  : None.
 * RETURN  : A pointer to the previous item or NULL if curitem was the first item in the list.
 * NOTE    : The curitem item must be in the list because that
 *           condition will not be detected and will cause major problems.
 */
RVCOREAPI
void * RVCALLCONV RvObjListGetPrevious(
    IN RvObjList *objlist,
    IN void *curitem,
    IN RvBool removeitem)
{
    RvObjListElement *elem;
    void *item;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objlist == NULL)
        return NULL;
#endif

    if(curitem != NULL) {
        elem = (RvObjListElement *)((RvInt8 *)curitem + objlist->offset);
        elem = elem->prev;
    } else elem = objlist->anchor.prev; /* Get last item */
    item = elem->obj;
    if((removeitem == RV_OBJLIST_REMOVE) && (item != NULL)) {
        /* Only remove the item if requested and its not the anchor */
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
        objlist->count -= 1;
    }

    return item;
}


/********************************************************************************************
 * RvObjListRemoveItem
 * Removes an items from a list.
 * INPUT   : objlist - Pointer to object list to remove item from.
 *           item - Pointer to item to remove from the list.
 * OUTPUT  : None.
 * RETURN  : A pointer to the previous item that was removed or NULL if it could not be removed.
 * NOTE    : Do not remove an item from a list that it is not in because that
 *           condition will not be detected and will cause major problems.
 */
RVCOREAPI
void * RVCALLCONV RvObjListRemoveItem(
    IN RvObjList *objlist,
    IN void *item)
{
    RvObjListElement *elem;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((objlist == NULL) || (item == NULL))
        return NULL;
#endif

    elem = (RvObjListElement *)((RvInt8 *)item + objlist->offset);
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    objlist->count -= 1;

    return item;
}


/********************************************************************************************
 * RvObjListReset
 *
 * Resets (clears) the objlist. After resetting state of the objlist is identical to the
 *  state just after construction.
 *
 * INPUT   : objlist - Pointer to object list to remove item from.
 * OUTPUT  : None.
 * RETURN  : A pointer to the objlist itself
 */
RVCOREAPI
RvObjList* RVCALLCONV RvObjListReset(RvObjList *objlist)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objlist == NULL)
        return NULL;
#endif

    /* List is circular with anchor having NULL obj */
    objlist->anchor.prev = &objlist->anchor;
    objlist->anchor.next = &objlist->anchor;
    objlist->anchor.obj = NULL;
    objlist->count = 0;

    return objlist;
}


