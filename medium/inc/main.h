/**
 * @file main.h
 * @brief Public interface for main module
 */

#ifndef MEDIUM_MAIN_H_
#define MEDIUM_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Disk timer hook (required by FatFS) */
void diskTickHook(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* MEDIUM_MAIN_H_ */


