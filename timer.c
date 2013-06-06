#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/init.h>

atomic_t tact = ATOMIC_INIT(5000);

static struct timer_list sos_timer;

static struct kobject * kobj;

static ssize_t file_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t file_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobj_attribute sos_attr =__ATTR(sos, 0666,    file_show, file_store);

static struct attribute *attrs[] = {
    &sos_attr.attr, NULL
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

void sos_timer_callback(unsigned long data)
{
    int c = atomic_read(&tact);
    if (c != -1) {
        printk(KERN_INFO "Timer tick. Frequency is %d\n", c);
        mod_timer(&sos_timer, jiffies + msecs_to_jiffies(c));
    }
}

static ssize_t file_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "Timer frequency is %d msecs now.\n", atomic_read(&tact));
}

static ssize_t file_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int res, cache = 0;
    res = sscanf(buf, "%du", &cache);
    if (res <= 0) {
        printk(KERN_ERR "Value is not appliable: %s!\n", buf);
        return count;
    }
    if (cache > 500) {
        atomic_set(&tact, cache);
    } else {
        printk(KERN_ERR "%d value is invalid.\n", cache);
    }
    return count;
}

static int __init timer_init(void)
{
    int ret, start = atomic_read(&tact);
    printk(KERN_INFO "Timer module is installing.\n");
    setup_timer(&sos_timer, sos_timer_callback, 0);
    printk(KERN_INFO "Starting timer to fire in default %dms (jiffies %ld)\n", start, jiffies);
    ret = mod_timer(&sos_timer, jiffies + msecs_to_jiffies(start));
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

static void __exit timer_exit(void)
{
    int ret;
    atomic_set(&tact, -1);
    ret = del_timer(&sos_timer);
    if (ret) {
        printk(KERN_ERR "Failed to turn off timer.\n");
    }
    kobject_put(kobj);
    printk(KERN_INFO "Timer module uninstalled.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eugene Gritskevich");

module_init(timer_init);
module_exit(timer_exit);

