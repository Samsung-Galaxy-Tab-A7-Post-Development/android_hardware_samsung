#ifndef _MFGLOADER_H_
#define _MFGLOADER_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);

#ifdef __cplusplus
}
#endif

#endif /* _MFGLOADER_H_ */
