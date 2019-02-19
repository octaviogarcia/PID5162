/* This library provides generic watchdog timer management functionality.
 * The functions operate on a timer queue provided by the caller. Note that
 * the timers must use absolute time to allow sorting. The library provides:
 *
 *    tmrs_settimer:     (re)set a new watchdog timer in the timers queue 
 *    tmrs_clrtimer:     remove a timer from both the timers queue 
 *    tmrs_exptimers:    check for expired timers and run watchdog functions
 *
 * Author:
 *    Jorrit N. Herder <jnherder@cs.vu.nl>
 *    Adapted from dvk_tmr_settimer and dvk_tmr_clrtimer in src/kernel/clock.c. 
 *    Last modified: September 30, 2004.
 */

#ifndef _DVSCOM_TIMERS_H
#define _DVSCOM_TIMERS_H

typedef unsigned long dvkclock_t;	

struct dvktimer;
typedef void (*dvk_tmr_func_t)(struct dvktimer *tp);
typedef union { int ta_int; long ta_long; void *ta_ptr; } dvk_tmr_arg_t;



/* A timer_t variable must be declare for each distinct timer to be used.
 * The timers watchdog function and expiration time are automatically set
 * by the library function tmrs_settimer, but its argument is not.
 */
typedef struct dvktimer
{
  struct dvktimer	*dvk_tmr_next;	/* next in a timer chain */
  dvkclock_t 	dvk_tmr_exp_time;	/* expiration time */
  dvk_tmr_func_t	dvk_tmr_func;	/* function to call when expired */
  dvk_tmr_arg_t		dvk_tmr_arg;	/* random argument */
} dvktimer_t;


dvkclock_t tmrs_clrtimer(dvktimer_t **tmrs, dvktimer_t *tp, dvkclock_t *next_time);
void tmrs_exptimers(dvktimer_t **tmrs, dvkclock_t now, dvkclock_t *new_head);
dvkclock_t tmrs_settimer(dvktimer_t **tmrs, dvktimer_t *tp, dvkclock_t exp_time, dvk_tmr_func_t watchdog, dvkclock_t *new_head);


/* Used when the timer is not active. */
#define TMR_NEVER    ((dvkclock_t) -1 < 0) ? ((dvkclock_t) MOL_MOL_LONG_MAX) : ((dvkclock_t) -1)
#undef TMR_NEVER
#define TMR_NEVER	((dvkclock_t) MOL_LONG_MAX)

/* These definitions can be used to set or get data from a timer variable. */ 
#define dvk_tmr_arg(tp) (&(tp)->dvk_tmr_arg)
#define dvk_tmr_exp_time(tp) (&(tp)->dvk_tmr_exp_time)

/* Timers should be initialized once before they are being used. Be careful
 * not to reinitialize a timer that is in a list of timers, or the chain
 * will be broken.
 */
#define dvk_tmr_inittimer(tp) (void)((tp)->dvk_tmr_exp_time = TMR_NEVER, \
	(tp)->dvk_tmr_next = NULL)

#endif /* _DVSCOM_TIMERS_H */

