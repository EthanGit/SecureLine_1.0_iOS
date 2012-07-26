#ifndef __HDD_GUID__H__
#define __HDD_GUID__H__

#ifdef __cplusplus
extern "C"{
#endif

typedef struct am_hdd_guid {
	char volume_name[256];
	char volume_physicalname[256];
	char volume_serial[22];
	char volume_guid[42];
} hdd_guid_t;

typedef struct am_proc_guid {
	char proc_name[256];
	char proc_serial[22];
	char proc_guid[42];
} proc_guid_t;

typedef struct am_mb_guid {
	char mb_name[256];
	char mb_serial[22];
	char mb_guid[42];
} mb_guid_t;

int hdd_guid_get(hdd_guid_t *hg);

int proc_guid_get(proc_guid_t *hg);

int mb_guid_get(mb_guid_t *hg);

#ifdef __cplusplus
}
#endif

#endif