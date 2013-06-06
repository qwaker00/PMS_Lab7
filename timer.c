#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/init.h>

#define DRIVER_AUTHOR "Eugene Gritskevich <qwaker.00@gmail.com>"
#define DRIVER_DESC   "Timer module"

atomic_t interval = ATOMIC_INIT(1000);

static ssize_t file_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "Timer frequency is %d msecs.\n", atomic_read(&interval));
}

static ssize_t file_write(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int res, cache = 0;
    res = sscanf(buf, "%u", &cache);
    if (res <= 0) {
        printk(KERN_ERR "Bad value: %s!\n", buf);
        return count;
    }
    if (cache > 500) {
        atomic_set(&interval, cache);
    } else {
        printk(KERN_ERR "Bad value %d.\n", cache);
    }
    return count;
}
 
static struct timer_list tick_timer;
static struct kobject * kobj;
static struct kobj_attribute tick_attr =__ATTR(tick, 0666, file_read, file_write);
static struct attribute *attrs[] = {&tick_attr.attr, NULL};
static struct attribute_group attr_group = { .attrs = attrs };

void tick_timer_callback(unsigned long data)
{
    int c = atomic_read(&interval);
    if (c != -1) {
        printk(KERN_INFO "Timer tick. Frequency is %d\n", c);
        mod_timer(&tick_timer, jiffies + msecs_to_jiffies(c));
    }
}

static int __init init(void)
{
    int ret, start = atomic_read(&interval);
    printk(KERN_INFO "Timer module init.\n");
    setup_timer(&tick_timer, tick_timer_callback, 0);
    printk(KERN_INFO "Starting timer. Frequency %dms\n", start);
    ret = mod_timer(&tick_timer, jiffies + msecs_to_jiffies(start));
    if (ret) {
        printk(KERN_INFO "Error when setting up timer.\n");
    }
    kobj = kobject_create_and_add("timerk", kernel_kobj);
    if (!kobj) {
        return -ENOMEM;
    }
    ret = sysfs_create_group(kobj, &attr_group);
    if (ret) {
        kobject_put(kobj);
    }
    return ret;
}

static void __exit cleanup(void)
{
    int ret;
    atomic_set(&interval, -1);
    ret = del_timer(&tick_timer);
    if (ret) {
        printk(KERN_ERR "Failed to shutdown timer.\n");
    }
    kobject_put(kobj);
    printk(KERN_INFO "Timer module exit.\n");
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("idevice");

