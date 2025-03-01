#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <linux/printk.h>
#include <linux/syscore_ops.h>
#include <linux/dma-mapping.h>
#include "interface.h"
#include "sampler.h"
#include "util.h"

#include "ondiemet.h"

#include "met_drv.h"
#include "met_tag.h"
#include "met_kernel_symbol.h"

#include "version.h"

static int run = -1;
static int sample_rate = 1000;	/* Default: 1000 Hz */
static int met_suspend_compensation_mode;
static int met_suspend_compensation_flag;
/* defalut method is perf driver */
unsigned int met_cpu_pmu_method = 1;

int met_hrtimer_expire;		/* in ns */
int met_timer_expire;		/* in jiffies */
unsigned int ctrl_flags;
int met_mode;
EXPORT_SYMBOL(met_mode);
int acao_test;

DEFINE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf_nmi);
EXPORT_SYMBOL(met_strbuf_nmi);

DEFINE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf_irq);
EXPORT_SYMBOL(met_strbuf_irq);

DEFINE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf_sirq);
EXPORT_SYMBOL(met_strbuf_sirq);

DEFINE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf);
EXPORT_SYMBOL(met_strbuf);

static void calc_timer_value(int rate)
{
	sample_rate = rate;

	if (rate == 0) {
		met_hrtimer_expire = 0;
		met_timer_expire = 0;
		return;
	}

	met_hrtimer_expire = 1000000000 / rate;

	/* Case 1: hrtimer < 1 OS tick, met_timer_expire = 1 OS tick */
	if (rate > HZ)
		met_timer_expire = 1;
	/* Case 2: hrtimer > 1 OS tick, met_timer_expire is hrtimer + 1 OS tick */
	else
		met_timer_expire = (HZ / rate) + 1;

	/* xprintk("JBK HZ=%d, met_hrtimer_expire=%d ns, met_timer_expire=%d ticks\n", */
	/* HZ, met_hrtimer_expire, met_timer_expire); */
}

int met_parse_num(const char *str, unsigned int *value, int len)
{
	int ret;

	if (len <= 0)
		return -1;

	if ((len > 2) &&
		((str[0] == '0') &&
		((str[1] == 'x') || (str[1] == 'X')))) {
		ret = kstrtoint(str, 16, value);
	} else {
		ret = kstrtoint(str, 10, value);
	}

	if (ret != 0)
		return -1;

	return 0;
}

void met_set_suspend_notify(int flag)
{
	if (met_suspend_compensation_mode == 1)
		met_suspend_compensation_flag = flag;
	else
		met_suspend_compensation_flag = 0;
}

LIST_HEAD(met_list);
static struct kobject *kobj_misc;
static struct kobject *kobj_pmu;
static struct kobject *kobj_bus;

static ssize_t ver_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(ver, 0444, ver_show, NULL);

static ssize_t plf_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(plf, 0444, plf_show, NULL);

static ssize_t core_topology_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(core_topology, 0444, core_topology_show, NULL);

static ssize_t devices_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(devices, 0444, devices_show, NULL);

static ssize_t ctrl_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count);
static DEVICE_ATTR(ctrl, 0664, ctrl_show, ctrl_store);

static ssize_t spr_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t spr_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count);
static DEVICE_ATTR(sample_rate, 0664, spr_show, spr_store);

static ssize_t acao_test_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t acao_test_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count);
static DEVICE_ATTR(acao_test, 0664, acao_test_show, acao_test_store);

static ssize_t run_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t run_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count);
static DEVICE_ATTR(run, 0664, run_show, run_store);

static ssize_t ksym_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ksym_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count);
static DEVICE_ATTR(ksym, 0664, ksym_show, ksym_store);




static ssize_t cpu_pmu_method_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t cpu_pmu_method_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count);
static DEVICE_ATTR(cpu_pmu_method, 0664, cpu_pmu_method_show, cpu_pmu_method_store);

#ifdef PR_CPU_NOTIFY
int met_cpu_notify;
static ssize_t cpu_notify_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t cpu_notify_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count);
static DEVICE_ATTR(cpu_notify, 0664, cpu_notify_show, cpu_notify_store);
#endif

#ifdef CONFIG_CPU_FREQ
static ssize_t dvfs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count);
static DEVICE_ATTR(dvfs, 0664, dvfs_show, dvfs_store);
#endif

static ssize_t suspend_compensation_enable_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t suspend_compensation_enable_store(struct device *dev, struct device_attribute *attr,
							const char *buf, size_t count);
static DEVICE_ATTR(suspend_compensation_enable, 0664, suspend_compensation_enable_show,
			suspend_compensation_enable_store);

static ssize_t suspend_compensation_flag_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(suspend_compensation_flag, 0444, suspend_compensation_flag_show, NULL);

static const struct file_operations met_file_ops = {
	.owner = THIS_MODULE
};

struct miscdevice met_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "met",
	.mode = 0664,
	.fops = &met_file_ops
};
EXPORT_SYMBOL(met_device);

static int met_run(void)
{
	sampler_start();
#ifdef MET_USER_EVENT_SUPPORT
	bltab.flag &= (~MET_CLASS_ALL);
#endif
	ondiemet_start();
	return 0;
}

static void met_stop(void)
{
#ifdef MET_USER_EVENT_SUPPORT
	bltab.flag |= MET_CLASS_ALL;
#endif
	sampler_stop();
#if 0
	/* the met.ko will be use by script "cat ...", release it */
	if ((ondiemet_module[ONDIEMET_SSPM] == 0) || (sspm_buffer_size == -1))
		ondiemet_log_manager_stop();
	ondiemet_stop();
	ondiemet_extract();
#endif
}

static ssize_t ver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%s\n", MET_BACKEND_VERSION);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t devices_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len, total_len = 0;
	struct metdevice *c = NULL;

	mutex_lock(&dev->mutex);
	list_for_each_entry(c, &met_list, list) {
		len = 0;
		if (c->type == MET_TYPE_PMU)
			len = snprintf(buf, PAGE_SIZE - total_len, "pmu/%s:0\n", c->name);
		else if (c->type == MET_TYPE_BUS)
			len = snprintf(buf, PAGE_SIZE - total_len, "bus/%s:0\n", c->name);
		else if (c->type == MET_TYPE_MISC)
			len = snprintf(buf, PAGE_SIZE - total_len, "misc/%s:0\n", c->name);

		if (c->ondiemet_mode == 0) {
			if (c->process_argument)
				buf[len - 2]++;
		} else if (c->ondiemet_mode == 1) {
			if (c->ondiemet_process_argument)
				buf[len - 2]++;
		} else if (c->ondiemet_mode == 2) {
			if (c->process_argument)
				buf[len - 2]++;
			if (c->ondiemet_process_argument)
				buf[len - 2]++;
		}

		buf += len;
		total_len += len;
	}

	mutex_unlock(&dev->mutex);
	return total_len;
}

static char met_platform[16] = "none";
static ssize_t plf_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%s\n", met_platform);
	mutex_unlock(&dev->mutex);
	return i;
}

static char met_topology[64] = "none";
static ssize_t core_topology_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%s\n", met_topology);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", ctrl_flags);
}

static ssize_t ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count)
{
	unsigned int value = 0;

	if (met_parse_num(buf, &value, count) < 0)
		return -EINVAL;

	ctrl_flags = value;
	return count;
}

static ssize_t cpu_pmu_method_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", met_cpu_pmu_method);
}

static ssize_t cpu_pmu_method_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count)
{
	unsigned int value;

	if (met_parse_num(buf, &value, count) < 0)
		return -EINVAL;

	met_cpu_pmu_method = value;
	return count;
}

#if	defined(MET_BOOT_MSG)
char met_boot_msg_tmp[256];
char met_boot_msg[PAGE_SIZE];
int met_boot_msg_idx;

int pr_bootmsg(int str_len, char *str)
{
	if (met_boot_msg_idx+str_len+1 > PAGE_SIZE)
		return -1;
	memcpy(met_boot_msg+met_boot_msg_idx, str, str_len);
	met_boot_msg_idx += str_len;
	return 0;
}

static ssize_t bootmsg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int	i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%s\n", met_boot_msg);
	mutex_unlock(&dev->mutex);
	return i;
}

static DEVICE_ATTR_RO(bootmsg);
#endif

static ssize_t spr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", sample_rate);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t spr_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count)
{
	int value;
	struct metdevice *c = NULL;

	mutex_lock(&dev->mutex);

	if ((run == 1) || (count == 0) || (buf == NULL)) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}
	if (kstrtoint(buf, 0, &value) != 0) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	if ((value < 0) || (value > 10000)) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	calc_timer_value(value);

	list_for_each_entry(c, &met_list, list) {
		if (c->polling_interval > 0)
			c->polling_count_reload = ((c->polling_interval * sample_rate) - 1) / 1000;
		else
			c->polling_count_reload = 0;
	}

	mutex_unlock(&dev->mutex);

	return count;
}

static ssize_t acao_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", acao_test);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t acao_test_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count)
{
	int value;

	mutex_lock(&dev->mutex);

	if ((count == 0) || (buf == NULL)) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	if (kstrtoint(buf, 0, &value) != 0) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	acao_test = value;

	mutex_unlock(&dev->mutex);

	return count;
}

static ssize_t run_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", run);
	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t run_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count)
{
	int value;

	mutex_lock(&dev->mutex);

	if ((count == 0) || (buf == NULL)) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}
	if (kstrtoint(buf, 0, &value) != 0) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	switch (value) {
	case 1:
		if (run != 1) {
			run = 1;
			met_run();
		}
		break;
	case 0:
		if (run != 0) {
			if (run == 1) {
				met_stop();
#ifdef MET_USER_EVENT_SUPPORT
#ifdef CONFIG_MET_MODULE
				met_save_dump_buffer_real("/data/trace.dump");
#else
				met_save_dump_buffer("/data/trace.dump");
#endif
#endif
				run = 0;
			} else
				/* run == -1 */
				run = 0;
		}
		break;
	case -1:
		if (run != -1) {
			if (run == 1)
				met_stop();

			run = -1;
		}
		break;
	default:
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	mutex_unlock(&dev->mutex);

	return count;
}





static unsigned int met_ksym_addr;
static char met_func_name[512];
static ssize_t ksym_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;
	int len = 0;
	int idx = 0;

	mutex_lock(&dev->mutex);
	if (met_ksym_addr != 0)
		len = sprint_symbol_no_offset(met_func_name, met_ksym_addr);
	if (len != 0) {
		for (idx = 0; idx < 512; idx++)
			if (met_func_name[idx] == ' ')
				met_func_name[idx] = '\0';
		i = snprintf(buf, PAGE_SIZE, "%s\n", met_func_name);
	} else
		i = snprintf(buf, PAGE_SIZE, "ksymlookup fail(%x)\n", met_ksym_addr);

	mutex_unlock(&dev->mutex);
	return i;
}

static ssize_t ksym_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count)
{
	mutex_lock(&dev->mutex);

	if ((count == 0) || (buf == NULL)) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}
	if (kstrtoint(buf, 16, &met_ksym_addr) != 0) {
		mutex_unlock(&dev->mutex);
		return -EINVAL;
	}

	mutex_unlock(&dev->mutex);

	return count;
}

#if	defined(PR_CPU_NOTIFY)
static ssize_t cpu_notify_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	i = snprintf(buf, PAGE_SIZE, "%d\n", met_cpu_notify);
	return i;
}

static ssize_t cpu_notify_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count)
{
	if ((count == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &met_cpu_notify) != 0)
		return -EINVAL;

	return count;
}
#endif

#ifdef CONFIG_CPU_FREQ
static ssize_t dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	i = snprintf(buf, PAGE_SIZE, "%d\n", 0);
	return i;
}

static ssize_t dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t count)
{
	return count;
}
#endif

static ssize_t suspend_compensation_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", met_suspend_compensation_mode);

	return ret;
}

static ssize_t suspend_compensation_enable_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int value;

	if ((count == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	met_suspend_compensation_mode = value;

	return count;
}

static ssize_t suspend_compensation_flag_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", met_suspend_compensation_flag);

	return ret;
}

static ssize_t hash_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t hash_store(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t count)
{
	return 0;
}

static DEVICE_ATTR(hash, 0664, hash_show, hash_store);

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	return snprintf(buf, PAGE_SIZE, "%d\n", c->mode);
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
			  size_t n)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (kstrtoint(buf, 0, &(c->mode)) != 0)
		return -EINVAL;

	return n;
}

static struct kobj_attribute mode_attr = __ATTR(mode, 0664, mode_show, mode_store);

static ssize_t ondiemet_mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	return snprintf(buf, PAGE_SIZE, "%d\n", c->ondiemet_mode);
}

static ssize_t ondiemet_mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
			  size_t n)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (kstrtoint(buf, 0, &(c->ondiemet_mode)) != 0)
		return -EINVAL;

	return n;
}

static struct kobj_attribute ondiemet_mode_attr = __ATTR(ondiemet_mode, 0664, ondiemet_mode_show, ondiemet_mode_store);

static ssize_t polling_interval_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int interval = 1;
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (c->polling_interval)
		interval = c->polling_interval;

	return snprintf(buf, PAGE_SIZE, "%d\n", interval);
}

static ssize_t polling_interval_store(struct kobject *kobj, struct kobj_attribute *attr,
				      const char *buf, size_t n)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (kstrtoint(buf, 0, &(c->polling_interval)) != 0)
		return -EINVAL;

	if (c->polling_interval > 0)
		c->polling_count_reload = ((c->polling_interval * sample_rate) - 1) / 1000;
	else
		c->polling_count_reload = 0;

	return n;
}

static struct kobj_attribute polling_interval_attr =
__ATTR(polling_ms, 0664, polling_interval_show, polling_interval_store);

static ssize_t header_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct metdevice *c = NULL;
	ssize_t count = 0;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (c->ondiemet_mode == 0) {
		if ((c->mode) && (c->print_header))
			return c->print_header(buf, PAGE_SIZE);
	} else if (c->ondiemet_mode == 1) {
		if ((c->mode) && (c->ondiemet_print_header))
			return c->ondiemet_print_header(buf, PAGE_SIZE);
	} else if (c->ondiemet_mode == 2) {
		if ((c->mode) && (c->print_header))
			count = c->print_header(buf, PAGE_SIZE);
		if (count < PAGE_SIZE) {
			if ((c->mode) && (c->ondiemet_print_header))
				count += c->ondiemet_print_header(buf+count, PAGE_SIZE - count);
		}
		return count;
	}

	return 0;
}

static struct kobj_attribute header_attr = __ATTR(header, 0444, header_show, NULL);

static ssize_t help_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct metdevice *c = NULL;
	ssize_t count = 0;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (c->ondiemet_mode == 0) {
		if (c->print_help)
			return c->print_help(buf, PAGE_SIZE);
	} else if (c->ondiemet_mode == 1) {
		if (c->ondiemet_print_help)
			return c->ondiemet_print_help(buf, PAGE_SIZE);
	} else if (c->ondiemet_mode == 2) {
		if (c->print_help)
			count = c->print_help(buf, PAGE_SIZE);
		if (count < PAGE_SIZE) {
			if (c->ondiemet_print_help)
				count += c->ondiemet_print_help(buf+count, PAGE_SIZE - count);
		}
		return count;
	}

	return 0;
}

static struct kobj_attribute help_attr = __ATTR(help, 0444, help_show, NULL);

static int argu_status = -1;
static ssize_t argu_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
			  size_t n)
{
	int ret = 0;
	struct metdevice *c = NULL;

	argu_status = -1;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (c->ondiemet_mode == 0) {
		if (c->process_argument)
			ret = c->process_argument(buf, (int)n);
	} else if (c->ondiemet_mode == 1) {
		if (c->ondiemet_process_argument)
			ret = c->ondiemet_process_argument(buf, (int)n);
	} else if (c->ondiemet_mode == 2) {
		if (c->process_argument)
			ret = c->process_argument(buf, (int)n);
		if (c->ondiemet_process_argument)
			ret = c->ondiemet_process_argument(buf, (int)n);
	}

	if (ret != 0)
		return -EINVAL;

	argu_status = 0;
	return n;
}

static ssize_t argu_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", argu_status);
}

static struct kobj_attribute argu_attr = __ATTR(argu, 0664, argu_show, argu_store);

static ssize_t reset_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	int ret = 0;
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	if (c->ondiemet_mode == 0) {
		if (c->reset)
			ret = c->reset();
		else
			c->mode = 0;
	} else if (c->ondiemet_mode == 1) {
		if (c->ondiemet_reset)
			ret = c->ondiemet_reset();
	} else if (c->ondiemet_mode == 2) {
		if (c->reset)
			ret = c->reset();
		else
			c->mode = 0;
		if (c->ondiemet_reset)
			ret = c->ondiemet_reset();
	}

	if (ret != 0)
		return -EINVAL;

	return n;
}

static struct kobj_attribute reset_attr = __ATTR(reset, 0220, NULL, reset_store);

static ssize_t header_read_again_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c->kobj == kobj)
			break;
	}
	if (c == NULL)
		return -ENOENT;

	return snprintf(buf, PAGE_SIZE, "%d\n", c->header_read_again);
}

static struct kobj_attribute header_read_again_attr = __ATTR(header_read_again, 0664, header_read_again_show, NULL);


int met_register(struct metdevice *met)
{
	int ret, cpu;
	struct metdevice *c;

	list_for_each_entry(c, &met_list, list) {
		if (!strcmp(c->name, met->name))
			return -EEXIST;
	}

	INIT_LIST_HEAD(&met->list);

	/* Allocate timer count for per CPU */
	met->polling_count = alloc_percpu(int);
	if (met->polling_count == NULL)
		return -EINVAL;

	for_each_possible_cpu(cpu)
		*(per_cpu_ptr(met->polling_count, cpu)) = 0;

	if (met->polling_interval > 0) {
		ret = ((met->polling_interval * sample_rate) - 1) / 1000;
		met->polling_count_reload = ret;
	} else
		met->polling_count_reload = 0;

	met->kobj = NULL;

	if (met->type == MET_TYPE_BUS)
		met->kobj = kobject_create_and_add(met->name, kobj_bus);
	else if (met->type == MET_TYPE_PMU)
		met->kobj = kobject_create_and_add(met->name, kobj_pmu);
	else if (met->type == MET_TYPE_MISC)
		met->kobj = kobject_create_and_add(met->name, kobj_misc);
	else {
		ret = -EINVAL;
		goto err_out;
	}

	if (met->kobj == NULL) {
		ret = -EINVAL;
		goto err_out;
	}

	if (met->create_subfs) {
		ret = met->create_subfs(met->kobj);
		if (ret)
			goto err_out;
	}

	ret = sysfs_create_file(met->kobj, &mode_attr.attr);
	if (ret)
		goto err_out;


	ret = sysfs_create_file(met->kobj, &ondiemet_mode_attr.attr);
	if (ret)
		goto err_out;

	ret = sysfs_create_file(met->kobj, &polling_interval_attr.attr);
	if (ret)
		goto err_out;

	ret = sysfs_create_file(met->kobj, &header_read_again_attr.attr);
	if (ret)
		goto err_out;

	if (met->print_header || met->ondiemet_print_header) {
		ret = sysfs_create_file(met->kobj, &header_attr.attr);
		if (ret)
			goto err_out;
	}

	if (met->print_help || met->ondiemet_print_help) {
		ret = sysfs_create_file(met->kobj, &help_attr.attr);
		if (ret)
			goto err_out;
	}

	if (met->process_argument || met->ondiemet_process_argument) {
		ret = sysfs_create_file(met->kobj, &argu_attr.attr);
		if (ret)
			goto err_out;
	}

	if (met->reset) {
		ret = sysfs_create_file(met->kobj, &reset_attr.attr);
		if (ret)
			goto err_out;
	}

	spin_lock_init(&met->my_lock);

	list_add(&met->list, &met_list);
	return 0;

 err_out:

	if (met->polling_count)
		free_percpu(met->polling_count);

	if (met->kobj) {
		kobject_del(met->kobj);
		kobject_put(met->kobj);
		met->kobj = NULL;
	}

	return ret;
}
EXPORT_SYMBOL(met_register);

int met_deregister(struct metdevice *met)
{
	struct metdevice *c = NULL;

	list_for_each_entry(c, &met_list, list) {
		if (c == met)
			break;
	}
	if (c != met)
		return -ENOENT;

	if (met->print_header || met->ondiemet_print_header)
		sysfs_remove_file(met->kobj, &header_attr.attr);

	if (met->print_help || met->ondiemet_print_help)
		sysfs_remove_file(met->kobj, &help_attr.attr);

	if (met->process_argument || met->ondiemet_process_argument)
		sysfs_remove_file(met->kobj, &argu_attr.attr);

	sysfs_remove_file(met->kobj, &reset_attr.attr);
	sysfs_remove_file(met->kobj, &header_read_again_attr.attr);
	sysfs_remove_file(met->kobj, &polling_interval_attr.attr);
	sysfs_remove_file(met->kobj, &mode_attr.attr);
	sysfs_remove_file(met->kobj, &ondiemet_mode_attr.attr);

	if (met->delete_subfs)
		met->delete_subfs();

	kobject_del(met->kobj);
	kobject_put(met->kobj);
	met->kobj = NULL;

	if (met->polling_count)
		free_percpu(met->polling_count);

	list_del(&met->list);
	return 0;
}
EXPORT_SYMBOL(met_deregister);

int met_set_platform(const char *plf_name, int flag)
{
	strncpy(met_platform, plf_name, sizeof(met_platform) - 1);
#if 0
	int ret;

	if (flag) {
		ret = device_create_file(met_device.this_device, &dev_attr_plf);
		if (ret != 0) {
			pr_debug("can not create device file: plf\n");
			return ret;
		}
		strncpy(met_platform, plf_name, sizeof(met_platform) - 1);
	} else
		device_remove_file(met_device.this_device, &dev_attr_plf);

#endif
	return 0;
}
EXPORT_SYMBOL(met_set_platform);

int met_set_topology(const char *topology_name, int flag)
{
	strncpy(met_topology, topology_name, sizeof(met_topology) - 1);
#if 0
	int ret;

	if (flag) {
		ret = device_create_file(met_device.this_device, &dev_attr_core_topology);
		if (ret != 0) {
			pr_debug("can not create device file: topology\n");
			return ret;
		}
		strncpy(met_topology, topology_name, sizeof(met_topology) - 1);
	} else {
		device_remove_file(met_device.this_device, &dev_attr_core_topology);
	}
#endif
	return 0;
}
EXPORT_SYMBOL(met_set_topology);

#include "met_struct.h"

void force_sample(void *unused)
{
	int cpu;
	unsigned long long stamp;
	struct metdevice *c;
	struct met_cpu_struct *met_cpu_ptr;

	if ((run != 1) || (sample_rate == 0))
		return;

	/* to avoid met tag is coming after __met_hrtimer_stop and before run=-1 */
	met_cpu_ptr = this_cpu_ptr(&met_cpu);
	if (met_cpu_ptr->work_enabled == 0)
		return;

	cpu = smp_processor_id();

	stamp = cpu_clock(cpu);

	list_for_each_entry(c, &met_list, list) {
		if (c->ondiemet_mode == 0) {
			if ((c->mode != 0) && (c->tagged_polling != NULL))
				c->tagged_polling(stamp, 0);
		} else if (c->ondiemet_mode == 1) {
			if ((c->mode != 0) && (c->ondiemet_tagged_polling != NULL))
				c->ondiemet_tagged_polling(stamp, 0);
		} else if (c->ondiemet_mode == 2) {
			if ((c->mode != 0) && (c->tagged_polling != NULL))
				c->tagged_polling(stamp, 0);
			if ((c->mode != 0) && (c->ondiemet_tagged_polling != NULL))
				c->ondiemet_tagged_polling(stamp, 0);
		}
	}
}

#define MET_SUSPEND_HAND
#ifdef MET_SUSPEND_HAND
static struct syscore_ops met_hrtimer_ops = {
	.suspend = met_hrtimer_suspend,
	.resume = met_hrtimer_resume,
};
#endif

int fs_reg(void)
{
	int ret = 0;

	ctrl_flags = 0;
	met_mode = 0;

#ifdef MET_SUSPEND_HAND
	/* suspend/resume function handle register */
	register_syscore_ops(&met_hrtimer_ops);
#endif

	calc_timer_value(sample_rate);

	ret = misc_register(&met_device);
	if (ret != 0) {
		pr_debug("misc register failed\n");
		return ret;
	}

	/* dma map config */
	/* arch_setup_dma_ops(met_device.this_device, 0, 0, NULL, false); */
	met_arch_setup_dma_ops_symbol(met_device.this_device);

	ret = device_create_file(met_device.this_device, &dev_attr_ksym);
	if (ret != 0) {
		pr_debug("can not create device file: ksym\n");
		return ret;
	}



	ret = device_create_file(met_device.this_device, &dev_attr_run);
	if (ret != 0) {
		pr_debug("can not create device file: run\n");
		return ret;
	}

#if	defined(PR_CPU_NOTIFY)
	ret = device_create_file(met_device.this_device, &dev_attr_cpu_notify);
	if (ret != 0) {
		pr_debug("can not create device file: cpu_notify\n");
		return ret;
	}
#endif

#ifdef CONFIG_CPU_FREQ
	ret = device_create_file(met_device.this_device, &dev_attr_dvfs);
	if (ret != 0) {
		pr_debug("can not create device file: dvfs\n");
		return ret;
	}
#endif

	ret = device_create_file(met_device.this_device, &dev_attr_suspend_compensation_enable);
	if (ret != 0) {
		pr_debug("can not create device file: suspend_compensation_enable\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_suspend_compensation_flag);
	if (ret != 0) {
		pr_debug("can not create device file: suspend_compensation_enable\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_ver);
	if (ret != 0) {
		pr_debug("can not create device file: ver\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_devices);
	if (ret != 0) {
		pr_debug("can not create device file: devices\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_ctrl);
	if (ret != 0) {
		pr_debug("can not create device file: ctrl\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_cpu_pmu_method);
	if (ret != 0) {
		pr_debug("can not create device file: cpu_pmu_method\n");
		return ret;
	}

#if	defined(MET_BOOT_MSG)
	ret = device_create_file(met_device.this_device, &dev_attr_bootmsg);
	if (ret != 0) {
		pr_debug("can not create device file: bootmsg\n");
		return ret;
	}
#endif

	ret = device_create_file(met_device.this_device, &dev_attr_sample_rate);
	if (ret != 0) {
		pr_debug("can not create device file: sample_rate\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_acao_test);
	if (ret != 0) {
		pr_debug("can not create device file: acao_test\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_core_topology);
	if (ret != 0) {
		pr_debug("can not create device file: topology\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_plf);
	if (ret != 0) {
		pr_debug("can not create device file: plf\n");
		return ret;
	}

	ret = device_create_file(met_device.this_device, &dev_attr_hash);
	if (ret != 0) {
		pr_debug("can not create device file: hash\n");
		return ret;
	}

	kobj_misc = kobject_create_and_add("misc", &met_device.this_device->kobj);
	if (kobj_misc == NULL) {
		pr_debug("can not create kobject: kobj_misc\n");
		return ret;
	}

	kobj_pmu = kobject_create_and_add("pmu", &met_device.this_device->kobj);
	if (kobj_pmu == NULL) {
		pr_debug("can not create kobject: kobj_pmu\n");
		return ret;
	}

	kobj_bus = kobject_create_and_add("bus", &met_device.this_device->kobj);
	if (kobj_bus == NULL) {
		pr_debug("can not create kobject: kobj_bus\n");
		return ret;
	}

	met_register(&met_cookie);
	met_register(&met_cpupmu);
	met_register(&met_memstat);
	met_register(&met_switch);
	met_register(&met_trace_event);
	met_register(&met_dummy_header);

#ifdef MET_USER_EVENT_SUPPORT
	tag_reg((struct file_operations * const) met_device.fops, &met_device.this_device->kobj);
#endif

	met_register(&met_stat);

/*	ondiemet_log_manager_init(met_device.this_device); */
/*	ondiemet_attr_init(met_device.this_device); */
	return ret;
}

void fs_unreg(void)
{
	if (run == 1)
		met_stop();

	run = -1;

	met_deregister(&met_stat);

#ifdef MET_USER_EVENT_SUPPORT
	tag_unreg();
#endif

	met_deregister(&met_dummy_header);
	met_deregister(&met_trace_event);
	met_deregister(&met_switch);
	met_deregister(&met_memstat);
	met_deregister(&met_cpupmu);
	met_deregister(&met_cookie);

	kobject_del(kobj_misc);
	kobject_put(kobj_misc);
	kobj_misc = NULL;
	kobject_del(kobj_pmu);
	kobject_put(kobj_pmu);
	kobj_pmu = NULL;
	kobject_del(kobj_bus);
	kobject_put(kobj_bus);
	kobj_bus = NULL;

	device_remove_file(met_device.this_device, &dev_attr_ksym);

	device_remove_file(met_device.this_device, &dev_attr_run);
#ifdef PR_CPU_NOTIFY
	device_remove_file(met_device.this_device, &dev_attr_cpu_notify);
#endif
#ifdef CONFIG_CPU_FREQ
	device_remove_file(met_device.this_device, &dev_attr_dvfs);
#endif
	device_remove_file(met_device.this_device, &dev_attr_suspend_compensation_enable);
	device_remove_file(met_device.this_device, &dev_attr_suspend_compensation_flag);

	device_remove_file(met_device.this_device, &dev_attr_ver);
	device_remove_file(met_device.this_device, &dev_attr_devices);
	device_remove_file(met_device.this_device, &dev_attr_sample_rate);

	device_remove_file(met_device.this_device, &dev_attr_ctrl);
	device_remove_file(met_device.this_device, &dev_attr_cpu_pmu_method);

	device_remove_file(met_device.this_device, &dev_attr_core_topology);
	device_remove_file(met_device.this_device, &dev_attr_plf);
	device_remove_file(met_device.this_device, &dev_attr_hash);

	/*ondiemet_log_manager_uninit(met_device.this_device);*/
	/*ondiemet_attr_uninit(met_device.this_device);*/

	misc_deregister(&met_device);
#ifdef MET_SUSPEND_HAND
	/* suspend/resume function handle register */
	unregister_syscore_ops(&met_hrtimer_ops);
#endif
}

unsigned int get_ctrl_flags(void)
{
	return ctrl_flags;
}
