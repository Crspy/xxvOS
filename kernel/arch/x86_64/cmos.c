#include <kernel/arch/x86_64/cmos.h>
#include <klog.h>
#include <x86intrin.h>
static int64_t arch_boot_time;      /* Time (in seconds) according to the CMOS right before we examine the TSC */
static uint64_t tsc_basis_time = 0; /* Accumulated time (in microseconds) on the TSC, when we timed it; eg. how long did boot take */
static uint64_t tsc_mhz = 2000;     /* MHz rating we determined for the TSC. Usually also the core speed */

/**
 * @brief read timestamp counter
 */
static inline uint64_t read_tsc(void)
{
    uint32_t lo, hi;
    asm volatile("rdtsc"
                 : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32UL) | (uint64_t)lo;
}

/**
 * @brief Initializes boot time, system time, TSC rate, etc.
 *
 * We determine TSC rate with a one-shot PIT, which seems
 * to work fine... the PIT is the only thing with both reasonable
 * precision and actual known wall-clock configuration.
 *
 * In Bochs, this has a tendency to be 1) completely wrong (usually
 * about half the time that actual execution will run at, in my
 * experiences) and 2) loud, as despite the attempt to turn off
 * the speaker it still seems to beep it (the second channel of the
 * PIT controls the beeper).
 *
 * In QEMU, VirtualBox, VMware, and on all real hardware I've tested,
 * including everything from a ThinkPad T410 to a Surface Pro 6, this
 * has been surprisingly accurate and good enough to use the TSC as
 * our only wall clock source.
 */
void arch_clock_initialize(void)
{
    /*
    PORT 0x43	r/w	PIT  mode port, control word register for counters 0-2
         bit 7-6 = 00  counter 0 select
             = 01  counter 1 select	  (not PS/2)
             = 10  counter 2 select
         bit 5-4 = 00  counter latch command
             = 01  read/write counter bits 0-7 only
             = 10  read/write counter bits 8-15 only
             = 11  read/write counter bits 0-7 first, then 8-15
         bit 3-1 = 000 mode 0 select
             = 001 mode 1 select - programmable one shot
             = x10 mode 2 select - rate generator
             = x11 mode 3 select - square wave generator
             = 100 mode 4 select - software triggered strobe
             = 101 mode 5 select - hardware triggered strobe
         bit 0	 = 0   binary counter 16 bits
             = 1   BCD counter
    */
    enum PIT_CounterMode
    {
        PIT_MODE_INTERRUPT_ON_COUNT = 0,
        PIT_MODE_ONE_SHOT = 1,
        PIT_MODE_RATE_GENERATOR = 2,
        // PIT_MODE_RATE_GENERATOR = 6,
        PIT_MODE_SQUARE_WAVE_GENERATOR = 3,
        // PIT_MODE_SQUARE_WAVE_GENERATOR = 7,
        PIT_MODE_SW_TRIGGERED_STROBE = 4,
        PIT_MODE_HW_TRIGGERED_STROBE = 5
    };
    enum PIT_CounterAccess
    {
        PIT_COUNTER_ACCESS_LATCH_COMMAND = 0,
        PIT_COUNTER_ACCESS_LOWER_HALF = 1,
        PIT_COUNTER_ACCESS_HIGHER_HALF = 2,
        PIT_COUNTER_ACCESS_ALL = 3,
    };
    typedef union
    {
        struct
        {
            uint8_t bcd_counter : 1;    // 0: binary counter , 1 : BCD counter
            uint8_t mode : 3;           // check enum PIT_CounterMode
            uint8_t counter_access : 2; // cehck enum PIT_CounterAccess
            uint8_t counter_select : 2; // select counter [0:2]
        };
        uint8_t raw;
    } PIT_Mode;

    enum
    {
        PIT_COUNTER2_PORT = 0x42, // PIT  counter 2, cassette & speaker    (XT, AT, PS/2)
        PIT_MODE_PORT = 0x43,     // control word register for counters 0-2
        KB_CONTROLLER_DATA_PORT = 0x60,
        KB_CONTROLLER_PORT_B = 0x61,
    };
    enum KB_CONTROLLER_PORT_B_CR_BITS
    {
        KB_TIMER2_GATE_TO_SPEAKER_ENABLE = (1 << 0),
        KB_SPEAKER_DATA_ENABLE = (1 << 1),
        KB_PARITY_CHECK_ENABLE = (1 << 2),
        KB_CHANNEL_CHECK_ENABLE = (1 << 3),
        // (bits 4-6) reserved
        KB_CLEAR_KEYBOARD = (1 << 7)
    };
    enum KB_CONTROLLER_PORT_B_STATUS_BITS
    {
        KB_TIMER2_GATE_TO_SPEAKER_STATUS = (1 << 0),
        KB_SPEAKER_DATA_STATUS = (1 << 1),
        KB_PARITY_CHECK_STATUS = (1 << 2),
        KB_CHANNEL_CHECK_STATUS = (1 << 3),
        // bit 4	 toggles with each refresh request
        KB_TIMER2_OUTPUT_STATUS = (1 << 5),
        KB_CHANNEL_CHECK_OCURRED = (1 << 6),
        KB_PARITY_CHECK_OCURRED = (1 << 7)
    };
    //date_t date = read_rtc();
    arch_boot_time = read_epoch_time();
    /* Disables and sets gating for channel 2 */

    // make sure speaker is disabled and enable timer2
    outportb(KB_CONTROLLER_PORT_B, (inportb(KB_CONTROLLER_PORT_B) & ~(KB_SPEAKER_DATA_ENABLE)) | KB_TIMER2_GATE_TO_SPEAKER_ENABLE);

    /* Configure channel 2 to one-shot */
    PIT_Mode counter2_mode = {.bcd_counter = 0,
                              .mode = PIT_MODE_ONE_SHOT,
                              .counter_access = PIT_COUNTER_ACCESS_ALL,
                              .counter_select = 2};

    // set reload register = 0x2E9B = 11931 (which is approximately 10 ms)
    outportb(PIT_MODE_PORT, counter2_mode.raw); 
    outportb(PIT_COUNTER2_PORT, 0x9B); 
    inportb(KB_CONTROLLER_DATA_PORT);
    outportb(PIT_COUNTER2_PORT, 0x2E);

    /* disable and enable timer 2 */
    uint8_t kb_port_cr = inportb(KB_CONTROLLER_PORT_B);
    kb_port_cr &= ~(KB_TIMER2_GATE_TO_SPEAKER_ENABLE);
    outportb(KB_CONTROLLER_PORT_B, kb_port_cr);
    outportb(KB_CONTROLLER_PORT_B, kb_port_cr | KB_TIMER2_GATE_TO_SPEAKER_ENABLE);

    const uint64_t time_start = __rdtsc();

    if (inportb(KB_CONTROLLER_PORT_B) & KB_TIMER2_OUTPUT_STATUS)
    {
        /* Loop until output goes low */
        while (inportb(KB_CONTROLLER_PORT_B) & KB_TIMER2_OUTPUT_STATUS)
            ;
    }
    else
    {
        /* Loop until output goes high */
        while ((inportb(KB_CONTROLLER_PORT_B) & KB_TIMER2_OUTPUT_STATUS) == 0)
            ;
    }
    const uint64_t time_end = __rdtsc();

    const uint64_t current_tsc_mhz = (time_end - time_start) / 10000; // divide by 10ms  to obtain speed in Mhz
    if (current_tsc_mhz != 0)
        tsc_mhz = current_tsc_mhz;
    tsc_basis_time = time_start / tsc_mhz;

    KLOGD("TSC", "TSC timed at %lu MHz\n", tsc_mhz);
    KLOGD("TSC", "Boot time is %lus\n", arch_boot_time);
    KLOGD("TSC", "Initial TSC timestamp was %luus\n", tsc_basis_time);
}

unsigned short century_register = 0x00; // Set by ACPI table parsing code if possible
static date_t current_date;

enum
{
    CMOS_ADDRESS_PORT = 0x70,
    CMOS_DATA_PORT = 0x71
};

enum CMOS_MAP
{
    CMOS_RTC_SECONDS = 0x0, // Contains the seconds value of current time
    CMOS_RTC_SECONDS_ALARM, // Contains the seconds value for the RTC alarm
    CMOS_RTC_MINUTES,       // Contains the minutes value of the current time
    CMOS_RTC_MINUTES_ALARM, // Contains the minutes value for the RTC alarm
    CMOS_RTC_HOURS,         // Contains the hours value of the current time
    CMOS_RTC_HOURS_ALARM,   // Contains the hours value for the RTC alarm
    CMOS_RTC_WEEKS,         // Contains the current day of the week
    CMOS_RTC_DAYS,          // Contains day value of current date
    CMOS_RTC_MONTHS,        // Contains the month value of current date
    CMOS_RTC_YEARS,         // Contains the year value of current date

    /*
     * Bit 7 = Update in progress (0 = Date and time can be read, 1 = Time update in progress)
     * Bits 6-4 = Time frequency divider (010 = 32.768KHz)
     * Bits 3-0 = Rate selection frequency (0110 = 1.024KHz square wave frequency)
     */
    CMOS_STATUS_REG_A,

    /*
     * Bit 7 = Clock update cycle (0 = Update normally, 1 = Abort update in progress)
     * Bit 6 = Periodic interrupt (0 = Disable interrupt (default), 1 = Enable interrupt)
     * Bit 5 = Alarm interrupt (0 = Disable interrupt (default), 1 = Enable interrupt)
     * Bit 4 = Update ended interrupt (0 = Disable interrupt (default), 1 = Enable interrupt)
     * Bit 3 = Status register A square wave frequency (0 = Disable square wave (default), 1 = Enable square wave)
     * Bit 2 = Binary mode (0 = BCD mode (default), 1 = time/date in binary)
     * Bit 1 = 24 hour clock (0 = 24 hour mode (default), 1 = 12 hour mode)
     * Bit 0 = Daylight savings time (0 = Disable daylight savings (default), 1 = Enable daylight savings)
     */
    CMOS_STATUS_REG_B,
};

int get_update_in_progress_flag()
{
    outportb(CMOS_ADDRESS_PORT, CMOS_STATUS_REG_A);
    enum CMOS_STATUS_REG_A
    {
        CMOS_STATUS_A_UPDATE_IN_PROGRESS = 1 << 7, // Bit 7 = Update in progress (0 = Date and time can be read, 1 = Time update in progress)
    };
    return (inportb(CMOS_DATA_PORT) & CMOS_STATUS_A_UPDATE_IN_PROGRESS);
}

unsigned char get_RTC_register(int reg)
{
    outportb(CMOS_ADDRESS_PORT, reg | 0x80); // disable NMI by ORing with 0x80
    return inportb(CMOS_DATA_PORT);
}

// #define IS_LEAP_YEAR(year) (((year) % 4 == 0) && (((year) % 100 != 0) || ((year) % 400 == 0)))
#define IS_LEAP_YEAR(year) (((year)&3) == 0 && (((year) % 25) != 0 || ((year)&15) == 0)) /* optimized version */

/**
 * @brief calculate seconds passed since Unix epoch until the given year.
 *
 * @param year Gregorian Calendar Year
 * @returns Seconds passed since the Unix epoch
 */
static uint64_t year_to_seconds(unsigned int year)
{
    // count days passed since 1970
    uint64_t days = 0;
    while (year > 1969)
    {
        days += 365;
        if (IS_LEAP_YEAR(year))
        {
            days++;
        }
        year--;
    }
    return days * 86400;
}

/**
 * @brief calculate seconds passed since the start of a year until a certain month
 *
 * Handles leap year properly
 *
 * @param month Gregorian Calendar month
 * @param year   Gregorian Calendar Year
 * @return Number of seconds in that month.
 */
static uint64_t months_to_seconds(unsigned int month, unsigned int year)
{

    // calculate days passed since since the start of a year until given "month"
    uint64_t days = 0;
    switch (month)
    {
    case 11:
        days += 30; /* fallthrough */
    case 10:
        days += 31; /* fallthrough */
    case 9:
        days += 30; /* fallthrough */
    case 8:
        days += 31; /* fallthrough */
    case 7:
        days += 31; /* fallthrough */
    case 6:
        days += 30; /* fallthrough */
    case 5:
        days += 31; /* fallthrough */
    case 4:
        days += 30; /* fallthrough */
    case 3:
        days += 31; /* fallthrough */
    case 2:
        days += 28;
        if (IS_LEAP_YEAR(year))
        {
            days++;
        } /* fallthrough */
    case 1:
        days += 31; /* fallthrough */
    default:
        break;
    }
    return days * 86400;
}

date_t read_rtc()
{
    unsigned char century = 0;
    unsigned char last_century = 0;
    unsigned char registerB;
    date_t last_date;

    // Note: This uses the "read registers until you get the same values twice in a row" technique
    //       to avoid getting dodgy/inconsistent values due to RTC updates

    while (get_update_in_progress_flag())
        ; // Make sure an update isn't in progress
    current_date.second = get_RTC_register(CMOS_RTC_SECONDS);
    current_date.minute = get_RTC_register(CMOS_RTC_MINUTES);
    current_date.hour = get_RTC_register(CMOS_RTC_HOURS);
    current_date.day = get_RTC_register(CMOS_RTC_DAYS);
    current_date.month = get_RTC_register(CMOS_RTC_MONTHS);
    current_date.year = get_RTC_register(CMOS_RTC_YEARS);
    if (century_register != 0)
    {
        century = get_RTC_register(century_register);
    }

    do
    {
        last_century = century;
        last_date = current_date;

        while (get_update_in_progress_flag())
            ; // Make sure an update isn't in progress
        current_date.second = get_RTC_register(CMOS_RTC_SECONDS);
        current_date.minute = get_RTC_register(CMOS_RTC_MINUTES);
        current_date.hour = get_RTC_register(CMOS_RTC_HOURS);
        current_date.day = get_RTC_register(CMOS_RTC_DAYS);
        current_date.month = get_RTC_register(CMOS_RTC_MONTHS);
        current_date.year = get_RTC_register(CMOS_RTC_YEARS);
        if (century_register != 0)
        {
            century = get_RTC_register(century_register);
        }
    } while ((last_date.second != current_date.second) || (last_date.minute != current_date.minute) || (last_date.hour != current_date.hour) ||
             (last_date.day != current_date.day) || (last_date.month != current_date.month) || (last_date.year != current_date.year) ||
             (last_century != century));

    registerB = get_RTC_register(CMOS_STATUS_REG_B);
    enum CMOS_STATUS_REG_B
    {
        CMOS_STATUS_B_HOUR_FORMAT = (1 << 1), // Bit 1 = 24 hour clock (0 = 24 hour mode (default), 1 = 12 hour mode)
        CMOS_STATUS_B_BINARY_MODE = (1 << 2), // Bit 2 = Binary mode (0 = BCD mode (default), 1 = time/date in binary)
    };

    // Convert BCD to binary values if necessary
    if (!(registerB & CMOS_STATUS_B_BINARY_MODE))
    {
#define BCD_TO_BIN(val) ((((val) / 16) * 10) + ((val)&0xf))
        current_date.second = BCD_TO_BIN(current_date.second);
        current_date.minute = BCD_TO_BIN(current_date.minute);
        current_date.day = BCD_TO_BIN(current_date.day);
        current_date.month = BCD_TO_BIN(current_date.month);
        current_date.year = BCD_TO_BIN(current_date.year);
        if (century_register != 0)
        {
            century = BCD_TO_BIN(century);
        }

        // hours are handled a little differently since they come in two different formats (12 hour/ 24 hour)
        current_date.hour = ((current_date.hour & 0x0F) + (((current_date.hour & 0x70) / 16) * 10)) | (current_date.hour & 0x80);
    }

    // Convert 12 hour clock to 24 hour clock if necessary
    if (!(registerB & CMOS_STATUS_B_HOUR_FORMAT) && (current_date.hour & 0x80))
    {
        current_date.hour = ((current_date.hour & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year
    if (century_register != 0)
    {
        current_date.year += century * 100;
    }
    else
    {
        current_date.year += 2000; // rtc year register contains years passed since 2000
    }

    return current_date;
}

uint64_t read_epoch_time()
{
    // calculate std::time from <time.h>
    uint64_t time =
        year_to_seconds(current_date.year - 1) +
        months_to_seconds(current_date.month - 1, current_date.year) +
        (current_date.day - 1) * 86400 +
        (current_date.hour) * 3600 +
        (current_date.minute) * 60 +
        current_date.second + 0;
    return time;
}