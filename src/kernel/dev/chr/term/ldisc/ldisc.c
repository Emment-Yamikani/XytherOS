#include <bits/errno.h>
#include <core/defs.h>
#include <dev/ldisc.h>

static ldisc_t *ldisciplines[8] = {
    NULL,
};

int ldisc_init() {
    return register_ldisc(x_TTY, &ldisc_X_TTY);
}

int register_ldisc(int ldisc_id, ldisc_t *ldisc) {
    if (ldisc == NULL || ldisc_id >= (int)NELEM(ldisciplines)) {
        return -EINVAL;
    }

    ldisciplines[ldisc_id] = ldisc;
    return 0;
}