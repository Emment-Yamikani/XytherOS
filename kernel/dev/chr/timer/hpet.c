#include <arch/chipset.h>
#include <arch/firmware/acpi.h>
#include <arch/traps.h>
#include <bits/errno.h>
#include <dev/timer.h>
#include <lib/printk.h>
#include <string.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>

typedef struct {
    acpiSDT_t       SdtHdr;     // ACPI SDT header.
    uint32_t        TmrBlkID;   // Timer block ID.
    struct {
        uint8_t     addrID;     // address ID.
        uint8_t     width;      // register width.
        uint8_t     offset;     // register offset.
        uint8_t     resvd0;
        uint64_t    blk_addr;   // timer block base address.
    } __packed;
    uint8_t         hpet_num;   // HPET number.
    uint16_t        clk_tick;   // minimum clok tick period. 
    struct {
        uint8_t     no_guarantee : 1;
        uint8_t     _4kib   : 1;
        uint8_t     _64kib  : 1;
        uint8_t     resvd1  : 4;
    } __packed;
} __packed acpi_hpet_t;

static volatile atomic_u64 *HPET = NULL;

/*General Capabilities and Identification register. Read-only*/
#define HPET_CAPID              HPET[(0x000) / 8]

/*General Configurations register.*/
#define HPET_CNF                HPET[(0x010) / 8]

/*General Interrupt Status register. */
#define HPET_INT_STATUS         HPET[(0x020) / 8]

/*Main Counter Value register.*/
#define HPET_MAIN_COUNTER_VAL   HPET[(0x0F0) / 8]

/*----------------------------------------------------*/
/*------------------Timer locations-------------------*/
/*----------------------------------------------------*/

#define HPET_TMR_CNF(n)         HPET[(0x100 + ((n) * 0x20)) / 8]
#define HPET_TMR_CMP(n)         HPET[(0x108 + ((n) * 0x20)) / 8]
#define HPET_TMR_FSB_RT(n)      HPET[(0x110 + ((n) * 0x20)) / 8]

#define HPET_TMR0_CNF           HPET_TMR_CNF(0)
#define HPET_TMR0_CMP           HPET_TMR_CMP(0)
#define HPET_TMR0_FSB_RT        HPET_TMR_FSB_RT(0)

#define HPET_TMR1_CNF           HPET_TMR_CNF(1)
#define HPET_TMR1_CMP           HPET_TMR_CMP(1)
#define HPET_TMR1_FSB_RT        HPET_TMR_FSB_RT(1)

#define HPET_TMR2_CNF           HPET_TMR_CNF(2)
#define HPET_TMR2_CMP           HPET_TMR_CMP(2)
#define HPET_TMR2_FSB_RT        HPET_TMR_FSB_RT(2)

/*Clock period of the main counter in femptoseconds(10^-15).*/
#define HPET_CLK_PERIOD         ((HPET_CAPID >> 32) & 0xffffffff)

/*Vendor ID, gives a value that PCI Vendor ID reports.*/
#define HPET_VENDOR             ((HPET_CAPID >> 16) & 0xffff)

/*Legacy routing capabilities bit, 1 if Legacy interrupt routing is supported.*/
#define HPET_LEG_CAP            (HPET_CAPID & (1 << 15))

/**
 * Size of the main counter register, 1 if 64bits or 0 is 32bits.
 * @NOTE: 32bit counter can not be set to run in 64bit mode.*/
#define HPET_COUNTER_SIZE       (HPET_CAPID & (1 << 13))

/**
 * Number of timers implemented in this HPET block.
 * @NOTE: this field == n, where n is the last timer, i.e. 'total # timers - 1'
 * e.g. this field = 2 for 3 timers.*/
#define HPET_nTMR_CAP           ((HPET_CAPID >> 8)  & 0x1f)

/*Indicates which version of the function is implemented.*/
#define HPET_REVISION           (HPET_CAPID & 0xff)

#define HPET_LEG_RT_CNF         (1 << 1)
#define HPET_ENABLE_CNF         (1 << 0)

#define HPET_TMR_INT_CAP(n)         ((HPET_TMR_CNF(n) >> 32) & 0xffffffff)
#define HPET_TMR_FSB_DEL_CAP        (1 << 15)
#define HPET_TMR_FSB_EN_CNF         (1 << 14)
#define HPET_TMR_INT_RT_CNF(n)      ((HPET_TMR_CNF(n) >> 9) & 0x1f)
#define HPET_TMR_SET_INT_RT_CNF(n, rt) (HPET_TMR_CNF(n) |= (((rt) & 0x1f) << 9))
#define HPET_TMR_32MODE_CNF         (1 << 8)
#define HPET_TMR_VAL_SET_CNF        (1 << 6)
#define HPET_TMR_64BIT_CAP          (1 << 5)
#define HPET_TMR_PER_INT_CAP        (1 << 4)
#define HPET_TMR_PERIODIC_EN        (1 << 3)
#define HPET_TMR_INT_EN_CNF         (1 << 2)
#define HPET_TMR_LEVEL_TRIG_CNF     (1 << 1)

typedef struct {

} hpet_tmr_t;

static hpet_tmr_t timer[3];

static void hpet_disable(void) {
    HPET_CNF &= ~HPET_ENABLE_CNF;
}

static void hpet_enable(void) {
    HPET_CNF |= HPET_ENABLE_CNF;
}

int hpet_tmr_init() {
    return 0;
}

int hpet_init(void) {
    int         err   = 0;
    usize       tmrcnf= 0;
    acpi_hpet_t *hpet = NULL;

    hpet = (acpi_hpet_t *)acpi_enumerate("HPET");
    if (hpet == NULL)
        return -ENOTSUP;

    memset(timer, 0, sizeof timer);

    HPET = (atomic_u64 *)V2HI(hpet->blk_addr);

    hpet_disable();
    if ((err = hpet_tmr_init(0)))
        return err;

    HPET_MAIN_COUNTER_VAL = 0;

    HPET_TMR_SET_INT_RT_CNF(0, IRQ_HPET);
    tmrcnf = HPET_TMR0_CNF;
    tmrcnf |= HPET_TMR_VAL_SET_CNF;
    tmrcnf |= HPET_TMR_PERIODIC_EN;
    tmrcnf |= HPET_TMR_LEVEL_TRIG_CNF;
    tmrcnf |= HPET_TMR_INT_EN_CNF;

    HPET_TMR0_CNF = tmrcnf;
    HPET_TMR0_CMP = (1000000000000000uL / SYS_Hz) / HPET_CLK_PERIOD;
    hpet_enable();

    interrupt_controller_enable(IRQ_HPET, getcpuid());
    return 0;
}

void hpet_wait(double s __unused) {
    return;
}

void hpet_intr(void) {
    u64 int_status = HPET_INT_STATUS;

    if (int_status & (1 << 0)) {
        HPET_INT_STATUS |= 1 << 0;
        jiffies_update();
    } else if (int_status & (1 << 1)) {
        HPET_INT_STATUS |= 1 << 1;
    } else if (int_status & (1 << 2)) {
        HPET_INT_STATUS |= 1 << 2;
    }
}