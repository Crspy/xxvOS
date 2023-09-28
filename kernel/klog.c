
#include <klog.h>
#include <misc/kprintf.h>
#include <kernel/arch/x86_64/cmos.h>

vprintf_like_t s_log_print_func = vprintf; // imported from <kprintf.h>

void klog_write(klog_level_t level,
                   const char *tag,
                   const char *format, ...)
{
    va_list list;
    va_start(list, format);
    klog_writev(level, tag, format, list);
    va_end(list);
}

void klog_writev(klog_level_t level,
                   const char *tag,
                   const char *format,
                   va_list args)
{
    (void)level;
    (void)tag;
    // spinlock.lock

    // klog_level_t level_for_tag = s_log_level_get_and_unlock(tag);
    // if (!should_output(level, level_for_tag)) {
    //     return;
    // }

    (*s_log_print_func)(format, args);
    // spinlock.unlock

}

vprintf_like_t klog_set_vprintf(vprintf_like_t func)
{
    // spinlock.lock
    vprintf_like_t orig_func = s_log_print_func;
    s_log_print_func = func;
    // spinlock.unlock
    return orig_func;
}

char *klog_system_timestamp(void)
{
    static char buffer[32] = {0};
    date_t date = read_rtc();
    snprintf(buffer,sizeof(buffer),"%02d:%02d:%02d",date.hour,date.minute,date.second);
    return buffer; // not thread safe yet
}