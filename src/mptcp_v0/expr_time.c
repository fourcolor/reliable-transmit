#include <stddef.h>
#include "expr_time.h"

#ifdef HAVE_CLOCK_GETTIME

#include <time.h>

int
expr_time_now(struct expr_time *time1)
{
    struct timespec ts;
    int result;
    result = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (result == 0) {
        time1->secs = (uint32_t) ts.tv_sec;
        time1->usecs = (uint32_t) ts.tv_nsec / 1000;
    }
    return result;
}

#else

#include <sys/time.h>

int
expr_time_now(struct expr_time *time1)
{
    struct timeval tv;
    int result;
    result = gettimeofday(&tv, NULL);
    time1->secs = tv.tv_sec;
    time1->usecs = tv.tv_usec;
    return result;
}

#endif

/* expr_time_add_usecs
 *
 * Add a number of microseconds to a expr_time.
 */
void
expr_time_add_usecs(struct expr_time *time1, uint64_t usecs)
{
    time1->secs += usecs / 1000000L;
    time1->usecs += usecs % 1000000L;
    if ( time1->usecs >= 1000000L ) {
        time1->secs += time1->usecs / 1000000L;
        time1->usecs %= 1000000L;
    }
}

uint64_t
expr_time_in_usecs(struct expr_time *time)
{
    return time->secs * 1000000LL + time->usecs;
}

double
expr_time_in_secs(struct expr_time *time)
{
    return time->secs + time->usecs / 1000000.0;
}

/* expr_time_compare
 *
 * Compare two timestamps
 *
 * Returns -1 if time1 is earlier, 1 if time1 is later,
 * or 0 if the timestamps are equal.
 */
int
expr_time_compare(struct expr_time *time1, struct expr_time *time2)
{
    if (time1->secs < time2->secs)
        return -1;
    if (time1->secs > time2->secs)
        return 1;
    if (time1->usecs < time2->usecs)
        return -1;
    if (time1->usecs > time2->usecs)
        return 1;
    return 0;
}

/* expr_time_diff
 *
 * Calculates the time from time2 to time1, assuming time1 is later than time2.
 * The diff will always be positive, so the return value should be checked
 * to determine if time1 was earlier than time2.
 *
 * Returns 1 if the time1 is less than or equal to time2, otherwise 0.
 */
int
expr_time_diff(struct expr_time *time1, struct expr_time *time2, struct expr_time *diff)
{
    int past = 0;
    int cmp = 0;

    cmp = expr_time_compare(time1, time2);
    if (cmp == 0) {
        diff->secs = 0;
        diff->usecs = 0;
        past = 1;
    }
    else if (cmp == 1) {
        diff->secs = time1->secs - time2->secs;
        diff->usecs = time1->usecs;
        if (diff->usecs < time2->usecs) {
            diff->secs -= 1;
            diff->usecs += 1000000;
        }
        diff->usecs = diff->usecs - time2->usecs;
    } else {
        diff->secs = time2->secs - time1->secs;
        diff->usecs = time2->usecs;
        if (diff->usecs < time1->usecs) {
            diff->secs -= 1;
            diff->usecs += 1000000;
        }
        diff->usecs = diff->usecs - time1->usecs;
        past = 1;
    }

    return past;
}
