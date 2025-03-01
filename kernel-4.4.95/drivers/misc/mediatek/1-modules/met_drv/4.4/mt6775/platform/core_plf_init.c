#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/kallsyms.h>
#include "core/met_drv.h"
#include "core/interface.h"
#include "core_plf_init.h"

#undef	DEBUG

#ifdef MET_GPU
/*
 *   GPU
 */
bool (*mtk_get_gpu_loading_symbol)(unsigned int *pLoading);
bool (*mtk_get_gpu_block_symbol)(unsigned int *pBlock);
bool (*mtk_get_gpu_idle_symbol)(unsigned int *pIdle);
bool (*mtk_get_gpu_dvfs_from_symbol)(MTK_GPU_DVFS_TYPE *peType, unsigned long *pulFreq);
bool (*mtk_get_gpu_sub_loading_symbol)(unsigned int *pLoading);
bool (*mtk_get_3D_fences_count_symbol)(int *pi32Count);
bool (*mtk_get_gpu_memory_usage_symbol)(unsigned int *pMemUsage);
bool (*mtk_get_gpu_power_loading_symbol)(unsigned int *pLoading);
bool (*mtk_get_custom_boost_gpu_freq_symbol)(unsigned long *pulFreq);
bool (*mtk_get_custom_upbound_gpu_freq_symbol)(unsigned long *pulFreq);
bool (*mtk_get_vsync_based_target_freq_symbol)(unsigned long *pulFreq);
bool (*mtk_get_vsync_offset_event_status_symbol)(unsigned int *pui32EventStatus);
bool (*mtk_get_vsync_offset_debug_status_symbol)(unsigned int *pui32EventStatus);
bool (*mtk_enable_gpu_perf_monitor_symbol)(bool enable);
bool (*mtk_get_gpu_pmu_init_symbol)(GPU_PMU *pmus, int pmu_size, int *ret_size);
bool (*mtk_get_gpu_pmu_swapnreset_symbol)(GPU_PMU *pmus, int pmu_size);
bool (*mtk_get_gpu_pmu_deinit_symbol)(void);
bool (*mtk_get_gpu_pmu_swapnreset_stop_symbol)(void);

unsigned int (*mt_gpufreq_get_cur_freq_symbol)(void);
unsigned int (*mt_gpufreq_get_thermal_limit_freq_symbol)(void);

bool (*mtk_register_gpu_power_change_symbol)(const char *name, gpu_power_change_notify_fp callback);
bool (*mtk_unregister_gpu_power_change_symbol)(const char *name);
#endif /* MET_GPU */

#ifdef MET_VCOREDVFS
/*
 *   VCORE DVFS
 */
#include <mtk_vcorefs_governor.h>
#include <mtk_spm_vcore_dvfs.h>

void (*spm_vcorefs_register_handler_symbol)(vcorefs_handler_t handler);
void (*vcorefs_register_req_notify_symbol)(vcorefs_req_handler_t handler);

char *(*governor_get_kicker_name_symbol)(int id);
int (*vcorefs_get_opp_info_num_symbol)(void);
char **(*vcorefs_get_opp_info_name_symbol)(void);
unsigned int *(*vcorefs_get_opp_info_symbol)(void);
int (*vcorefs_get_src_req_num_symbol)(void);
char **(*vcorefs_get_src_req_name_symbol)(void);
unsigned int *(*vcorefs_get_src_req_symbol)(void);
int (*vcorefs_enable_debug_isr_symbol)(bool);

int (*vcorefs_get_hw_opp_symbol)(void);
int (*vcorefs_get_curr_vcore_symbol)(void);
int (*vcorefs_get_curr_ddr_symbol)(void);
u32 (*spm_vcorefs_get_MD_status_symbol)(void);
int (*vcorefs_get_num_opp_symbol)(void);
int *kicker_table_symbol;

#endif /* MET_VCOREDVFS */

#ifdef MET_EMI
void *(*mt_cen_emi_base_get_symbol)(void);
#endif /* MET_EMI */

#ifdef MET_PTPOD
unsigned int (*mt_gpufreq_get_cur_volt_symbol)(void);
unsigned int (*mt_cpufreq_get_cur_volt_symbol)(unsigned int cluster_id);
#endif /* MET_PTPOD */

static int met_symbol_get(void)
{
#define _MET_SYMBOL_GET(_func_name_) \
	do { \
		_func_name_##_symbol = (void *)symbol_get(_func_name_); \
		if (_func_name_##_symbol == NULL) { \
			pr_debug("MET ext. symbol : %s is not found!\n", #_func_name_); \
			PR_BOOTMSG_ONCE("MET ext. symbol : %s is not found!\n", #_func_name_); \
		} \
	} while (0)


#ifdef MET_GPU
	_MET_SYMBOL_GET(mtk_get_gpu_loading);
	_MET_SYMBOL_GET(mtk_get_gpu_block);
	_MET_SYMBOL_GET(mtk_get_gpu_idle);
	_MET_SYMBOL_GET(mtk_get_gpu_dvfs_from);
	_MET_SYMBOL_GET(mtk_get_gpu_sub_loading);
	_MET_SYMBOL_GET(mtk_get_3D_fences_count);
	_MET_SYMBOL_GET(mtk_get_gpu_memory_usage);
	_MET_SYMBOL_GET(mtk_get_gpu_power_loading);
	_MET_SYMBOL_GET(mtk_get_custom_boost_gpu_freq);
	_MET_SYMBOL_GET(mtk_get_custom_upbound_gpu_freq);
	_MET_SYMBOL_GET(mtk_get_vsync_based_target_freq);
	_MET_SYMBOL_GET(mtk_get_vsync_offset_event_status);
	_MET_SYMBOL_GET(mtk_get_vsync_offset_debug_status);
	_MET_SYMBOL_GET(mtk_enable_gpu_perf_monitor);
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_init);
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_swapnreset);
	_MET_SYMBOL_GET(mt_gpufreq_get_cur_freq);
	_MET_SYMBOL_GET(mt_gpufreq_get_thermal_limit_freq);
	_MET_SYMBOL_GET(mtk_register_gpu_power_change);
	_MET_SYMBOL_GET(mtk_unregister_gpu_power_change);
#if 0 /* not implement yet */
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_swapnreset_stop);
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_deinit);
#endif
#endif /* MET_GPU */

#ifdef MET_VCOREDVFS
	_MET_SYMBOL_GET(spm_vcorefs_register_handler);
	_MET_SYMBOL_GET(vcorefs_register_req_notify);

	_MET_SYMBOL_GET(governor_get_kicker_name);
	_MET_SYMBOL_GET(vcorefs_get_opp_info_num);
	_MET_SYMBOL_GET(vcorefs_get_opp_info_name);
	_MET_SYMBOL_GET(vcorefs_get_opp_info);
	_MET_SYMBOL_GET(vcorefs_get_src_req_num);
	_MET_SYMBOL_GET(vcorefs_get_src_req_name);
	_MET_SYMBOL_GET(vcorefs_get_src_req);
	_MET_SYMBOL_GET(vcorefs_enable_debug_isr);

	_MET_SYMBOL_GET(vcorefs_get_hw_opp);
	_MET_SYMBOL_GET(vcorefs_get_curr_vcore);
	_MET_SYMBOL_GET(vcorefs_get_curr_ddr);
	_MET_SYMBOL_GET(spm_vcorefs_get_MD_status);
	_MET_SYMBOL_GET(vcorefs_get_num_opp);
	_MET_SYMBOL_GET(kicker_table);
#endif

#ifdef MET_EMI
	_MET_SYMBOL_GET(mt_cen_emi_base_get);
#endif

#ifdef MET_PTPOD
	_MET_SYMBOL_GET(mt_gpufreq_get_cur_volt);
	_MET_SYMBOL_GET(mt_cpufreq_get_cur_volt);
#endif

	return 0;
}

static int met_symbol_put(void)
{
#define _MET_SYMBOL_PUT(_func_name_) { \
		if (_func_name_##_symbol) { \
			symbol_put(_func_name_); \
			_func_name_##_symbol = NULL; \
		} \
	}

#ifdef MET_GPU
	_MET_SYMBOL_PUT(mtk_get_gpu_loading);
	_MET_SYMBOL_PUT(mtk_get_gpu_block);
	_MET_SYMBOL_PUT(mtk_get_gpu_idle);
	_MET_SYMBOL_PUT(mtk_get_gpu_dvfs_from);
	_MET_SYMBOL_PUT(mtk_get_gpu_sub_loading);
	_MET_SYMBOL_PUT(mtk_get_3D_fences_count);
	_MET_SYMBOL_PUT(mtk_get_gpu_memory_usage);
	_MET_SYMBOL_PUT(mtk_get_gpu_power_loading);
	_MET_SYMBOL_PUT(mtk_get_custom_boost_gpu_freq);
	_MET_SYMBOL_PUT(mtk_get_custom_upbound_gpu_freq);
	_MET_SYMBOL_PUT(mtk_get_vsync_based_target_freq);
	_MET_SYMBOL_PUT(mtk_get_vsync_offset_event_status);
	_MET_SYMBOL_PUT(mtk_get_vsync_offset_debug_status);
	_MET_SYMBOL_PUT(mtk_enable_gpu_perf_monitor);
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_init);
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_swapnreset);
	_MET_SYMBOL_PUT(mt_gpufreq_get_cur_freq);
	_MET_SYMBOL_PUT(mt_gpufreq_get_thermal_limit_freq);
	_MET_SYMBOL_PUT(mtk_register_gpu_power_change);
	_MET_SYMBOL_PUT(mtk_unregister_gpu_power_change);
#if 0 /* not implement yet */
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_swapnreset_stop);
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_deinit);
#endif
#endif /* mET_GPU */

#ifdef MET_VCOREDVFS
	_MET_SYMBOL_PUT(spm_vcorefs_register_handler);
	_MET_SYMBOL_PUT(vcorefs_register_req_notify);

	_MET_SYMBOL_PUT(governor_get_kicker_name);
	_MET_SYMBOL_PUT(vcorefs_get_opp_info_num);
	_MET_SYMBOL_PUT(vcorefs_get_opp_info_name);
	_MET_SYMBOL_PUT(vcorefs_get_opp_info);
	_MET_SYMBOL_PUT(vcorefs_get_src_req_num);
	_MET_SYMBOL_PUT(vcorefs_get_src_req_name);
	_MET_SYMBOL_PUT(vcorefs_get_src_req);
	_MET_SYMBOL_PUT(vcorefs_enable_debug_isr);

	_MET_SYMBOL_PUT(vcorefs_get_hw_opp);
	_MET_SYMBOL_PUT(vcorefs_get_curr_vcore);
	_MET_SYMBOL_PUT(vcorefs_get_curr_ddr);
	_MET_SYMBOL_PUT(spm_vcorefs_get_MD_status);
	_MET_SYMBOL_PUT(vcorefs_get_num_opp);
	_MET_SYMBOL_PUT(kicker_table);
#endif

#ifdef MET_EMI
	_MET_SYMBOL_PUT(mt_cen_emi_base_get);
#endif

#ifdef MET_PTPOD
	_MET_SYMBOL_PUT(mt_gpufreq_get_cur_volt);
	_MET_SYMBOL_PUT(mt_cpufreq_get_cur_volt);
#endif

	return 0;
}

int core_plf_init(void)
{
	/*initial met external symbol*/
	met_symbol_get();

#ifdef MET_GPU
	met_register(&met_gpu);
	met_register(&met_gpudvfs);
	met_register(&met_gpumem);
	met_register(&met_gpupwr);
	met_register(&met_gpu_pmu);
#endif

#ifdef MET_VCOREDVFS
	met_register(&met_vcoredvfs);
#endif

#ifdef MET_EMI
	met_register(&met_sspm_emi);
#endif

#ifdef MET_PTPOD
	met_register(&met_ptpod);
#endif

	return 0;
}

void core_plf_exit(void)
{
	/*release met external symbol*/
	met_symbol_put();

#ifdef MET_GPU
	met_deregister(&met_gpu);
	met_deregister(&met_gpudvfs);
	met_deregister(&met_gpumem);
	met_deregister(&met_gpupwr);
	met_deregister(&met_gpu_pmu);
#endif

#ifdef MET_VCOREDVFS
	met_deregister(&met_vcoredvfs);
#endif

#ifdef MET_EMI
	met_deregister(&met_sspm_emi);
#endif

#ifdef MET_PTPOD
	met_deregister(&met_ptpod);
#endif
}
