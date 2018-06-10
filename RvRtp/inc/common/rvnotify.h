/***********************************************************************
Filename   : rvnotify.h
Description: notifications handling
************************************************************************
Copyright (c) 2011 RADVISION Inc. and RADVISION Ltd.
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

#ifndef RV_NOTIFY_H
#define RV_NOTIFY_H

#include "rvtypes.h" 
#include "rverror.h" 
#include "rvlog.h" 

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
    extern "C" {
#endif 

/* the notifier source ID
   set by notifier module when the source is registered */
typedef RvInt32 RvNotifierSrcId;

/*  notifier source may set its  own number for the source
    this number must not be unique across all notifier sources */
typedef RvInt32 RvNotifierSource;

/* the notifier level values */
typedef enum {
    RvNtfLvlInfo        = 10,
    RvNtfLvlWarning     = 20,
    RvNtfLvlError       = 30,
    RvNtfLvlException   = 40
} RvNotificationLvl;

/* the notification id,
   the values should be unique in a scope of the notifier source */
typedef RvInt32 RvNotificationId;


/* this structure is used when the RvNotifyEx is called the same structure instance will
   be passed to the registered listeners.
 */
typedef struct {
    IN  RvNotifierSrcId         ntSrcId;
        /* The notifier source ID as was set when the notifier source was registered with 
           RvNotifierRegisterSource call or RV_NOTIFIER_SRC_ID_UNDEFINED value.
           If ntSrcId is RV_NOTIFIER_SRC_ID_UNDEFINED the ntSrcName must be set.
         */
    IN  const RvChar*           ntSrcName;
        /* The name of the registered notifier source. */

    IN  RvNotificationLvl       ntLvl;
        /* The level of this notification */

    IN  RvNotificationId        ntId;
        /* The notification ID. Should be unique in the scope of the 
           specific notifier source.
        */
    IN  const RvChar*          strReason;
        /* Optional notification text. */

    IN  void*                  ntCtx;
        /* Optional notification context. */

    OUT RvNotifierSource       ntSourceNum;
        /* The notifier source number.  */

} RvNotificationData;


/********************************************************************************************
* RvNotifierEnd
* The listener notification callback.
* INPUT   : 
*   pCtx    - Listener context registered when the RvNotifierRegisterListener was called. 
*   strListenerName - the lsitener name.
*   pNtData - The notification data.
* OUTPUT  : None
* RETURN  : None
*/
typedef void (*RvNotifierFunc)(
    IN  void*                   pCtx,
    IN  const RvChar*           strListenerName,
    IN  RvNotificationData*     pNtData
    );
   

/********************************************************************************************
* RvNotifierRegisterSource
* Registers notifier source. When the source was successfully registered it can later be addressed
* using the notifier source ID produced as the result of function call.
* INPUT   : 
*   srcName  - optional srcName or NULL. 
*   srcNum   - the number given will be available to the listeners when the notifications will be called
*              (as the  RvNotificationData.ntSourceNum value).
*
* OUTPUT  : 
*   pSrcId   - the notifier source ID.
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierRegisterSource(
    IN  const RvChar*           srcName,
    IN  RvNotifierSource        srcNum,
    OUT RvNotifierSrcId*        pSrcId);

/********************************************************************************************
* RvNotifierUnregisterSource
* Unregisters notifier source. 
* INPUT   : 
*   srcId   - the notifier source ID returned when the notifier source was registered with 
*             RvNotifierRegisterSource call.
* OUTPUT  : None
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierUnregisterSource(
    IN  RvNotifierSrcId       srcId);

/********************************************************************************************
* RvNotifierGetSourceByName
* Retrieves the notifier source ID and number by source name.
* INPUT   : 
*   srcName   - the notifier source name.
*
* OUTPUT  : 
*   pSrcId   - the notifier source ID returned when the notifier source was registered with 
*             RvNotifierRegisterSource call.
*   pSrcPubNum   - the notifier source number set when the notifier source was registered with 
*             RvNotifierRegisterSource call.
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierGetSourceByName(
    IN  const RvChar*      srcName,
    OUT RvNotifierSrcId*   pSrcId,
    OUT RvNotifierSource*  pSrcPubNum);


/********************************************************************************************
* RvNotifierRegisterListener
* Registers notifier listener. 
* INPUT   : 
*   ntLstnrName - optional listener name or NULL.
*   cbFunc   - the listener notifications callback function.
*   cbCtx    - the listener context.
*   lvl      - the minimal notifications level this listener wishes
*              to receive.
*   onlyRegisteredSrc - if set to RV_TRUE the listener will receive notifications
*              only from the sources registered to this listener with the 
*              RvNotifierListenerAddSrc API call.
*              Otherwise the listener will receive notifications from all sources.
*
* OUTPUT  :   None
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierRegisterListener(
    IN  const RvChar*       ntLstnrName,
    IN  RvNotifierFunc      cbFunc,
    IN  void*               cbCtx,
    IN  RvNotificationLvl   lvl,
    IN  RvBool              onlyRegisteredSrc);

/********************************************************************************************
* RvNotifierUnregisterListener
* Unregisters notifier listener. 
* At least one of three function parameters must not be NULL.
* INPUT   : 
*   ntLstnrName - optional listener name or NULL.
*   cbFunc   - optional listener notifications callback function or NULL.
*   cbCtx    - optional listener context or NULL.
* OUTPUT  :   None
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierUnregisterListener(
    IN  const RvChar*       ntLstnrName,
    IN  RvNotifierFunc      cbFunc,
    IN  void*               cbCtx);

/********************************************************************************************
* RvNotifierListenerAddSrc
* Add the the notifier source to the notifier listener. 
* At least one of the ntLstnrName, cbFunc and cbCtx must not be NULL.
* Also one of srcId or srcName must be provided.
* Note: when the notifier source is unregistered it will also remove itself
* from all listeners where it was added.
* INPUT   : 
*   ntLstnrName - optional listener name or NULL.
*   cbFunc   - optional listener notifications callback function or NULL.
*   cbCtx    - optional listener context or NULL.
*   srcId    - optional notifier source ID.
*   srcName  - optional notifier source name.
*   lvl      - the minimal notification level.
*              The listener will receive notifications with level higher then 
*              both level: one set for this source and general listener level set when
*              listener was registered.
* OUTPUT  :   None
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierListenerAddSrc(
    IN  const RvChar*           ntLstnrName,
    IN  RvNotifierFunc          cbFunc,
    IN  void*                   cbCtx,
    IN  RvNotifierSrcId         srcId,
    IN  RvChar*                 srcName,
    IN  RvNotificationLvl       lvl);


/********************************************************************************************
* RvNotify
* Create and propagate the notification.
* INPUT   : 
*   ntSrcId  - notifier source ID responsible for this notification.
*   ntLvl    - the notification level.
*   ntId     - the notification ID.
*   ntCtx    - the notification proprietary data.
* OUTPUT  :   None
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotify(
    IN RvNotifierSrcId    ntSrcId,
    IN RvNotificationLvl  ntLvl,
    IN RvNotificationId   ntId,
    IN void*              ntCtx);


/********************************************************************************************
* RvNotifyEx
* Create and propagate the notification.
* INPUT   : 
*   pNtData  - full notification data.
* OUTPUT  :   None
* RETURN  : RV_OK on success or error otherwise.
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifyEx(
    IN RvNotificationData*  pNtData);

#define   RV_NOTIFIER_SRC_ID_UNDEFINED         0

/********************************************************************************************
* RvNotifierInit
* Initializes the Notifier Module.
* INPUT   : None
* RETURN  : RV_OK on success, other on failure
*/
RvStatus RvNotifierInit(void);

/********************************************************************************************
* RvNotifierEnd
* De-initializes the Notifier Module.
* INPUT   : None
* RETURN  : RV_OK on success, other on failure
*/
RvStatus RvNotifierEnd(void);


/********************************************************************************************
* RvNotifierSetLogMgr
* Sets/replaces the log manager of the notifier module.
* INPUT   : 
*     pLogMgr - the log manager.
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI
RvStatus RVCALLCONV RvNotifierSetLogMgr(
    RvLogMgr* pLogMgr);

/********************************************************************************************
* RvNotifierLvl2Text
* Converts the notification level to text.
* INPUT   : 
*   lvl  - the level
* RETURN  : The textual value of the level.
*/
RVCOREAPI
const RvChar* RVCALLCONV RvNotifierLvl2Text(
    IN  RvNotificationLvl       lvl);



#if defined(__cplusplus)
    }
#endif

#endif /*RV_NOTIFY_H */


