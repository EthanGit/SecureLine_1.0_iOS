/*
  The amsip program is a modular SIP softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/
#ifdef __cplusplus
extern "C"{
#endif

int amsiptools_wizard_stop();
int amsiptools_wizard_start(const char *in_card, const char *out_card);
int amsiptools_wizard_play(char *wavfile);
//int amsiptools_wizard_startrecord(const char *in_card, const char *out_card, char *wavfile);

#ifdef __cplusplus
}
#endif
