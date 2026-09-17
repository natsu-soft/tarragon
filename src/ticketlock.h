#ifndef TICKETLOCK_H
#define TICKETLOCK_H

#include "ticketlock_types.h"

void ticket_lock(struct ticketlock* ticketlock);

void ticket_unlock(struct ticketlock* ticketlock);


#endif /* End TICKETLOCK_H */
