
#ifndef __HID_HOOKS__H__
#define __HID_HOOKS__H__

//#define ENABLE_HID 1

#ifdef __cplusplus
extern "C"{
#endif

#define HID_ERROR_DEVICEFAILURE -1
#define HID_ERROR_NODEVICEFOUND -2
#define HID_ERROR_NOTCOMPILED -3
#define HID_ERROR_NOTIMPLEMENTED -4

#define HID_EVENTS_HOOK 3
#define HID_EVENTS_HANGUP 4
#define HID_EVENTS_MUTE 5
#define HID_EVENTS_UNMUTE 6
#define HID_EVENTS_VOLUP 7
#define HID_EVENTS_VOLDOWN 8
#define HID_EVENTS_SMART 9
#define HID_EVENTS_FLASHUP 10
#define HID_EVENTS_FLASHDOWN 11
#define HID_EVENTS_TALK 12

#define HID_EVENTS_KEY0 13
#define HID_EVENTS_KEY1 14
#define HID_EVENTS_KEY2 15
#define HID_EVENTS_KEY3 16
#define HID_EVENTS_KEY4 16
#define HID_EVENTS_KEY5 17
#define HID_EVENTS_KEY6 18
#define HID_EVENTS_KEY7 19
#define HID_EVENTS_KEY8 20
#define HID_EVENTS_KEY9 21
#define HID_EVENTS_KEYSTAR 22
#define HID_EVENTS_KEYPOUND 23
#define HID_EVENTS_KEYERASE 24

struct hid_device;
struct hid_device_desc;

typedef struct hid_hooks {
	struct hid_device *jdevice; //internal only
	struct hid_device_desc *desc; //internal only
} hid_hooks_t;

/**
 * Stop HID control and release amsip ressource.
 *
 * @param ph            pointer to hid_hooks_t struct
 */
int hid_hooks_stop(hid_hooks_t *ph);

/**
 * Start HID control. First known device
 * will be used.
 *
 * @param ph            pointer to hid_hooks_t struct
 */
int hid_hooks_start(hid_hooks_t *ph);

/**
 * Start HID control for a specific known device.
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param vendorid      hexadecimal vendor id of device
 * @param productid     hexadecimal product id of device
 */
int hid_hooks_start_device(hid_hooks_t *ph, int vendorid, int productid);

/**
 * Set presence indicator
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param val           device dependent value
 */
int hid_hooks_set_presence_indicator(hid_hooks_t *ph, int value);

/**
 * Enable or Disable ringer.
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param enable        0/1: disable/enable
 */
int hid_hooks_set_ringer(hid_hooks_t *ph, int enable);

/**
 * Enable or Disable audio.
 * IMPORTANT:
 * To be able to get HOOK event, the audio must be disabled.
 * Thus, when there is no active call, the implementation
 * must make sure the audio is disabled.
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param enable        0/1: disable/enable
 */
int hid_hooks_set_audioenabled(hid_hooks_t *ph, int enable);

/**
 * Enable or Disable mute.
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param enable        0/1: disable/enable
 */
int hid_hooks_set_mute(hid_hooks_t *ph, int enable);

/**
 * Enable or Disable message waiting indicator.
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param enable        0/1: disable/enable
 */
int hid_hooks_set_messagewaiting(hid_hooks_t *ph, int enable);

/**
 * Enable or Disable send call indicator.
 *
 * @param ph            pointer to hid_hooks_t struct
 * @param enable        0/1: disable/enable
 */
int hid_hooks_set_sendcalls(hid_hooks_t *ph, int enable);

/**
 * Get current mute status
 * Return 0/>0 disabled/enabled
 *
 * @param ph            pointer to hid_hooks_t struct
 */
int hid_hooks_get_mute(hid_hooks_t *ph);

/**
 * Get current audio status
 * Return 0/>0 disabled/enabled
 *
 * @param ph            pointer to hid_hooks_t struct
 */
int hid_hooks_get_audioenabled(hid_hooks_t *ph);

/**
 * Get event from device in polling mode.
 * Return HID_EVENTS_XXXX which value are >0
 * Return value ==0 if no event.
 * Return value <0 upon error. (Note: Some errors might be recoverable.)
 *
 * @param ph            pointer to hid_hooks_t struct
 */
int hid_hooks_get_events(hid_hooks_t *ph);

#ifdef __cplusplus
}
#endif

#endif
