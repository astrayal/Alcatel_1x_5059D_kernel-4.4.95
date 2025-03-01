#ifndef __CORE_PLF_INIT_H__
#define __CORE_PLF_INIT_H__

extern struct miscdevice met_device;

/*
 *   MET External Symbol
 */

#ifdef MET_GPU
/*
 *   GPU
 */
#include <mtk_gpu_utility.h>
extern bool mtk_get_gpu_loading(unsigned int *pLoading);
extern bool mtk_get_gpu_block(unsigned int *pBlock);
extern bool mtk_get_gpu_idle(unsigned int *pIdle);
extern bool mtk_get_gpu_dvfs_from(MTK_GPU_DVFS_TYPE *peType, unsigned long *pulFreq);
extern bool mtk_get_gpu_sub_loading(unsigned int *pLoading);
extern bool mtk_get_3D_fences_count(int *pi32Count);
extern bool mtk_get_gpu_memory_usage(unsigned int *pMemUsage);
extern bool mtk_get_gpu_power_loading(unsigned int *pLoading);
extern bool mtk_get_custom_boost_gpu_freq(unsigned int *pui32FreqLevel);
extern bool mtk_get_custom_upbound_gpu_freq(unsigned int *pui32FreqLevel);
extern bool mtk_get_vsync_based_target_freq(unsigned long *pulFreq);
extern bool mtk_get_vsync_offset_event_status(unsigned int *pui32EventStatus);
extern bool mtk_get_vsync_offset_debug_status(unsigned int *pui32EventStatus);
extern bool mtk_enable_gpu_perf_monitor(bool enable);
extern bool mtk_get_gpu_pmu_init(GPU_PMU *pmus, int pmu_size, int *ret_size);
extern bool mtk_get_gpu_pmu_swapnreset(GPU_PMU *pmus, int pmu_size);

extern bool (*mtk_get_gpu_loading_symbol)(unsigned int *pLoading);
extern bool (*mtk_get_gpu_block_symbol)(unsigned int *pBlock);
extern bool (*mtk_get_gpu_idle_symbol)(unsigned int *pIdle);
extern bool (*mtk_get_gpu_dvfs_from_symbol)(MTK_GPU_DVFS_TYPE *peType, unsigned long *pulFreq);
extern bool (*mtk_get_gpu_sub_loading_symbol)(unsigned int *pLoading);
extern bool (*mtk_get_3D_fences_count_symbol)(int *pi32Count);
extern bool (*mtk_get_gpu_memory_usage_symbol)(unsigned int *pMemUsage);
extern bool (*mtk_get_gpu_power_loading_symbol)(unsigned int *pLoading);
extern bool (*mtk_get_custom_boost_gpu_freq_symbol)(unsigned long *pulFreq);
extern bool (*mtk_get_custom_upbound_gpu_freq_symbol)(unsigned long *pulFreq);
extern bool (*mtk_get_vsync_based_target_freq_symbol)(unsigned long *pulFreq);
extern bool (*mtk_get_vsync_offset_event_status_symbol)(unsigned int *pui32EventStatus);
extern bool (*mtk_get_vsync_offset_debug_status_symbol)(unsigned int *pui32EventStatus);
extern bool (*mtk_enable_gpu_perf_monitor_symbol)(bool enable);
extern bool (*mtk_get_gpu_pmu_init_symbol)(GPU_PMU *pmus, int pmu_size, int *ret_size);
extern bool (*mtk_get_gpu_pmu_swapnreset_symbol)(GPU_PMU *pmus, int pmu_size);

typedef void (*gpu_power_change_notify_fp) (int power_on);
extern bool mtk_register_gpu_power_change(const char *name, gpu_power_change_notify_fp callback);
extern bool mtk_unregister_gpu_power_change(const char *name);
extern bool (*mtk_register_gpu_power_change_symbol)(const char *name,
					gpu_power_change_notify_fp callback);
extern bool (*mtk_unregister_gpu_power_change_symbol)(const char *name);

#include <mtk_gpufreq.h>
extern unsigned int (*mt_gpufreq_get_cur_freq_symbol)(void);
extern unsigned int (*mt_gpufreq_get_thermal_limit_freq_symbol)(void);

extern struct metdevice met_gpu;
extern struct metdevice met_gpudvfs;
extern struct metdevice met_gpumem;
extern struct metdevice met_gpupwr;
extern struct metdevice met_gpu_pmu;
#endif /* MET_GPU */

#ifdef MET_VCOREDVFS
/*
 *   VCORE DVFS
 */
#include <mtk_spm.h>
#include <mtk_vcorefs_manager.h>
extern void (*spm_vcorefs_register_handler_symbol)(vcorefs_handler_t handler);
extern void (*vcorefs_register_req_notify_symbol)(vcorefs_req_handler_t handler);

extern char *(*governor_get_kicker_name_symbol)(int id);
extern int (*vcorefs_get_opp_info_num_symbol)(void);
extern char **(*vcorefs_get_opp_info_name_symbol)(void);
extern unsigned int *(*vcorefs_get_opp_info_symbol)(void);
extern int (*vcorefs_get_src_req_num_symbol)(void);
extern char **(*vcorefs_get_src_req_name_symbol)(void);
extern unsigned int *(*vcorefs_get_src_req_symbol)(void);
extern int (*vcorefs_enable_debug_isr_symbol)(bool);
extern int (*vcorefs_get_num_opp_symbol)(void);
extern int *kicker_table_symbol;

extern char *governor_get_kicker_name(int id);
extern int vcorefs_get_opp_info_num(void);
extern char **vcorefs_get_opp_info_name(void);
extern unsigned int *vcorefs_get_opp_info(void);
extern int vcorefs_get_src_req_num(void);
extern char **vcorefs_get_src_req_name(void);
extern unsigned int *vcorefs_get_src_req(void);
extern int vcorefs_enable_debug_isr(bool);
extern int vcorefs_get_num_opp(void);

extern int (*vcorefs_get_hw_opp_symbol)(void);
extern int (*vcorefs_get_curr_vcore_symbol)(void);
extern int (*vcorefs_get_curr_ddr_symbol)(void);
extern u32 (*spm_vcorefs_get_MD_status_symbol)(void);

extern struct metdevice met_vcoredvfs;
#endif /* MET_VCOREDVFS */

#ifdef MET_EMI
extern void *(*mt_cen_emi_base_get_symbol)(void);
extern void *mt_cen_emi_base_get(void);
extern struct metdevice met_sspm_emi;
#endif /* MET_EMI */

#endif /*__CORE_PLF_INIT_H__*/
