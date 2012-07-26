
#ifdef __cplusplus
extern "C"{
#endif

struct plantronics_device;

typedef struct {
	struct plantronics_device *pdevice;
} plantronics_hooks_t;

int plantronics_hooks_stop(plantronics_hooks_t *ph);
int plantronics_hooks_start(plantronics_hooks_t *ph);

int plantronics_hooks_set_ringer(plantronics_hooks_t *ph, bool enable);
int plantronics_hooks_set_audioenabled(plantronics_hooks_t *ph, bool enable);

bool plantronics_hooks_get_mute(plantronics_hooks_t *ph);
bool plantronics_hooks_get_audioenabled(plantronics_hooks_t *ph);
bool plantronics_hooks_get_isattached(plantronics_hooks_t *ph);

#ifdef __cplusplus
}
#endif
