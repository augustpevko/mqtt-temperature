/**
 * @file aug_wifi_scan.h
 * @brief Scans the access points and find not busy Wi-Fi channel.  
 * @note This module is supposed to be used when neither
 *       station nor access point mode is initialized.
 */

#if !defined(WIFI_SCAN_H)
#define AUG_WIFI_SCAN_H
#include <stdint.h>

/**
 * @brief Returns the least busy Wi-Fi channel among available access points.
 * @return uint8_t Number of the least busy Wi-Fi channel.
 */
uint8_t aug_get_least_freq_channel(void);

#endif