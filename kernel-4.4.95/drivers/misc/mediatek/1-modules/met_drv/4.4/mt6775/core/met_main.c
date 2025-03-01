#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/dcache.h>
#include <linux/types.h>
#include <linux/dcookies.h>
#include <linux/sched.h>
#include <linux/fs.h>

#include <asm/irq_regs.h>

#include "met_struct.h"
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include <linux/printk.h>
#include <linux/of.h>

#define PLATFORM_NAME   "mt6775"

struct cpu_type_name {
	char full_name[32];
	char abbr_name[8];
};

static struct cpu_type_name met_known_cpu_type[] = {
	{"arm,cortex-a53", "CA53"},
	{"arm,cortex-a73", "CA73"},
};
#define MET_KNOWN_CPU_TYPE_COUNT \
	(sizeof(met_known_cpu_type)/sizeof(struct cpu_type_name))

static char met_cpu_topology[64];

void (*tracing_record_cmdline_symbol)(struct task_struct *tsk);
void (*met_cpu_frequency_symbol)(unsigned int frequency, unsigned int cpu_id);
int (*met_reg_switch_symbol)(void);
void (*met_unreg_switch_symbol)(void);

void (*met_arch_setup_dma_ops_symbol)(struct device *dev);
u64 (*met_perf_event_read_local_symbol)(struct perf_event *ev);
struct task_struct *(*met_kthread_create_on_cpu_symbol)(int (*threadfn)(void *data),
				void *data, unsigned int cpu,
				const char *namefmt);

static void get_cpu_type_name(const char *compatible, char *cpu_type)
{
	int i;

	for (i = 0; i < MET_KNOWN_CPU_TYPE_COUNT; i++)
		if (!strncmp(compatible, met_known_cpu_type[i].full_name,
					strlen(met_known_cpu_type[i].full_name)))
			strncpy(cpu_type, met_known_cpu_type[i].abbr_name,
					strlen(met_known_cpu_type[i].abbr_name) + 1);
}

static void met_set_cpu_topology(int core_id, int cluster_core_num)
{
	int i, buf_len = strlen(met_cpu_topology);
	struct device_node *node = NULL;
	const char *prev_cptb = NULL;
	const char *cptb;
	char cpu_type[16];

	for (i = 0; i < cluster_core_num; i++) {
		node = of_get_cpu_node(core_id + i, NULL);
		if (node) {
			cptb = of_get_property(node, "compatible", NULL);
			if (cptb) {
				get_cpu_type_name(cptb, cpu_type);
				if (prev_cptb == NULL)
					/* first write:  write core_type & core_number */
					buf_len += snprintf(met_cpu_topology + buf_len,
								sizeof(met_cpu_topology) - buf_len,
								"%s:%d", cpu_type, core_id + i);
				else if (!strncmp(prev_cptb, cptb, strlen(prev_cptb)))
					/* cpu type is the same with before */
					/* write core_number */
					buf_len += snprintf(met_cpu_topology + buf_len,
								sizeof(met_cpu_topology) - buf_len,
								",%d", core_id + i);
				else
					/* cpu type is different with before */
					/* write core_type & core_number */
					buf_len += snprintf(met_cpu_topology + buf_len,
								sizeof(met_cpu_topology) - buf_len,
								"|%s:%d", cpu_type, core_id + i);

				prev_cptb = cptb;
			}
		}
	}
}

static int met_create_cpu_topology(void)
{
	int i, len;
	struct device_node *node = NULL;
	int start_core_id = 0;
	int cluster_num = 0, cluster_core_num = 0;
	char cluster_name[16];

	node = of_find_node_by_name(NULL, "cpu-map");
	if (node) {
		cluster_num = of_get_child_count(node);

		for (i = 0; i < cluster_num; i++) {
			snprintf(cluster_name, sizeof(cluster_name), "cluster%d", i);
			node = of_find_node_by_name(NULL, cluster_name);
			if (node) {
				cluster_core_num = of_get_child_count(node);

				/* "|" use to separate different cluster */
				if (i > 0) {
					len = strlen(met_cpu_topology);
					snprintf(met_cpu_topology + len, sizeof(met_cpu_topology) - len, "|");
				}

				met_set_cpu_topology(start_core_id, cluster_core_num);

				start_core_id = cluster_core_num;
			}
		}
	}

	return strlen(met_cpu_topology);
}

static int met_kernel_symbol_get(void)
{
	int ret = 0;

	if (tracing_record_cmdline_symbol == NULL)
		tracing_record_cmdline_symbol = (void *)symbol_get(met_tracing_record_cmdline);
	if (tracing_record_cmdline_symbol == NULL)
		ret = -3;

	if (met_cpu_frequency_symbol == NULL)
		met_cpu_frequency_symbol = (void *)symbol_get(met_cpu_frequency);
	if (met_cpu_frequency_symbol == NULL)
		ret = -4;

	if (met_reg_switch_symbol == NULL)
		met_reg_switch_symbol = (void *)symbol_get(met_reg_switch);
	if (met_reg_switch_symbol == NULL)
		ret = -5;

	if (met_unreg_switch_symbol == NULL)
		met_unreg_switch_symbol = (void *)symbol_get(met_unreg_switch);
	if (met_unreg_switch_symbol == NULL)
		ret = -6;

	if (met_arch_setup_dma_ops_symbol == NULL)
		met_arch_setup_dma_ops_symbol = (void *)symbol_get(met_arch_setup_dma_ops);
	if (met_arch_setup_dma_ops_symbol == NULL)
		ret = -7;

	if (met_perf_event_read_local_symbol == NULL)
		met_perf_event_read_local_symbol = (void *)symbol_get(met_perf_event_read_local);
	if (met_perf_event_read_local_symbol == NULL)
		ret = -8;

	if (met_kthread_create_on_cpu_symbol == NULL)
		met_kthread_create_on_cpu_symbol = (void *)symbol_get(met_kthread_create_on_cpu);
	if (met_kthread_create_on_cpu_symbol == NULL)
		ret = -9;

	return ret;
}

DEFINE_PER_CPU(struct met_cpu_struct, met_cpu);

static int __init met_drv_init(void)
{
	int cpu;
	int ret;
	int cpu_topology_len;
	struct met_cpu_struct *met_cpu_ptr;

	for_each_possible_cpu(cpu) {
		met_cpu_ptr = &per_cpu(met_cpu, cpu);
		/* snprintf(&(met_cpu_ptr->name[0]), sizeof(met_cpu_ptr->name), "met%02d", cpu); */
		met_cpu_ptr->cpu = cpu;
	}

	ret = met_kernel_symbol_get();
	if (ret)
		return ret;
	fs_reg();

	met_set_platform(PLATFORM_NAME, 1);

	cpu_topology_len = met_create_cpu_topology();
	if (cpu_topology_len)
		met_set_topology(met_cpu_topology, 1);

	core_plf_init();

	return 0;
}

static void __exit met_drv_exit(void)
{
	if (met_cpu_frequency_symbol)
		symbol_put(met_cpu_frequency);
	if (met_reg_switch_symbol)
		symbol_put(met_reg_switch);
	if (met_unreg_switch_symbol)
		symbol_put(met_unreg_switch);
	if (tracing_record_cmdline_symbol)
		symbol_put(met_tracing_record_cmdline);

	core_plf_exit();
	fs_unreg();

}
module_init(met_drv_init);
module_exit(met_drv_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");
