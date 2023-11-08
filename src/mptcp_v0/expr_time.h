#ifndef EXPR_TIME_H
#define EXPR_TIME_H

#include <stdint.h>

struct expr_time {
    uint32_t secs;
    uint32_t usecs;
};

int expr_time_now(struct expr_time *time1);

void expr_time_add_usecs(struct expr_time *time1, uint64_t usecs);

int expr_time_compare(struct expr_time *time1, struct expr_time *time2);

int expr_time_diff(struct expr_time *time1, struct expr_time *time2, struct expr_time *diff);

uint64_t expr_time_in_usecs(struct expr_time *time);

double expr_time_in_secs(struct expr_time *time);

#endif