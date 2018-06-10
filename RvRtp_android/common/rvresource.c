#include "rvresource.h"
#include "rvansi.h"
#include "rvlock.h"

#if RV_USE_RESOURCE_COUNTING

static RvResourceCounter *gsHead = 0;
static RvInt64            gsStartTime = 0;
static RvBool             doCount = 0;

#define rvGetTime() ((RvInt32)((RvTimestampGet(0) - gsStartTime) >> 20))

void RvResourceStartCount(void) {
    RvResourceCounter *cur;


    for(cur = gsHead; cur; cur = cur->next) {
        RvLockConstruct(0, &cur->lock);
    }

    doCount = 1;
}

void RvResourceEndCount(void) {
    RvResourceCounter *cur;

    doCount = 0;

    for(cur = gsHead; cur; cur = cur->next) {
        RvLockDestruct(&cur->lock, 0);
    }

    gsHead = 0;
}

static 
void RvResourceComputeAverage(RvResourceCounter *self) {
	RvInt32 curTime = rvGetTime();

	self->nAverage += self->nCurrent * (curTime - self->lastTime);
	self->lastTime = curTime;
}

void RvResourceCounterConstruct(RvResourceCounter *self, const RvChar *name) {
	if(gsStartTime == 0) {
		gsStartTime = RvTimestampGet(0);
	}
	self->nConstructed = 0;
	self->nCurrent = 0;
	self->lastTime = self->startTime = rvGetTime();
	self->nMax = 0;
	self->nAverage = 0;
	self->name = name;
	self->next = gsHead;
	gsHead = self;
    if(doCount) {
	    RvLockConstruct(0, &self->lock);
    }
}

#if 0

static 
void RvResourceCounterReset(RvResourceCounter *self) {
   	self->nConstructed = 0;
	self->nCurrent = 0;
	self->lastTime = self->startTime = rvGetTime();
	self->nMax = 0;
	self->nAverage = 0;

    RvLockDestruct(&self->lock, 0);
}
#endif

void RvOnConstructResource(RvResourceCounter *self) {
	if(!doCount) {
		return;
	}

	RvLockGet(&self->lock, 0);
	self->nConstructed++;
	self->nCurrent++;
	if(self->nCurrent > self->nMax) {
		self->nMax = self->nCurrent;
	}
	RvResourceComputeAverage(self);
	RvLockRelease(&self->lock, 0);
}

void RvOnDestructResource(RvResourceCounter *self) {
	if(!doCount) {
		return;
	}
	RvLockGet(&self->lock, 0);
	self->nCurrent--;
	RvResourceComputeAverage(self);
	RvLockRelease(&self->lock, 0);
}

RVCOREAPI void RvResourceCounterReportToString(RvChar *buf, RvSize_t bufLen) {
	RvResourceCounter *cur = gsHead;

	while(cur != 0) {
		RvInt32 avg;
		RvInt64 elapsedTime;
		RvSize_t len;
        RvResourceCounter *prev;

		RvResourceComputeAverage(cur);
		elapsedTime = cur->lastTime - cur->startTime;
		if(elapsedTime <= 0) {
			avg = -1;
		} else {
			avg = (RvInt32)(cur->nAverage / elapsedTime);
		}

        RvSnprintf(buf, bufLen, "Resource: %s\n\tConstructed: %d\n\tMax:         %d\n\tAvg:         %d\n", cur->name, cur->nConstructed, cur->nMax, avg);
		len = strlen(buf);
		if(len >= bufLen - 1) {
			return;
		}

		buf += len;
		bufLen -= len;
        prev = cur;
		cur = cur->next;
	}

}

#else
int prevent_warning_of_ranlib_has_no_symbols_rvresource=0;
#endif /* #if RV_USE_RESOURCE_COUNTING*/

