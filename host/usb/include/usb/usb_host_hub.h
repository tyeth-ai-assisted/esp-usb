/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hub power switching mode, from wHubCharacteristics D1..D0
 */
typedef enum {
    USB_HOST_HUB_POWER_SWITCHING_GANGED = 0,    /**< All ports are powered at once */
    USB_HOST_HUB_POWER_SWITCHING_PER_PORT = 1,  /**< Each port is powered individually */
    USB_HOST_HUB_POWER_SWITCHING_NONE = 2,      /**< No power switching (1.0 hubs report 2 or 3) */
} usb_host_hub_power_switching_t;

/**
 * @brief Information about an external hub managed by the USB Host Library
 */
typedef struct {
    uint8_t dev_addr;                   /**< Hub's device address */
    uint8_t parent_addr;                /**< Address of the hub this hub is attached to, 0 on a root port */
    uint8_t parent_port;                /**< Port number on the parent hub, 0 on a root port */
    uint16_t vid;                       /**< idVendor */
    uint16_t pid;                       /**< idProduct */
    char manufacturer[32];              /**< Manufacturer string (ASCII, may be empty) */
    char product[32];                   /**< Product string (ASCII, may be empty) */
    uint8_t num_ports;                  /**< Number of downstream ports */
    uint8_t power_switching;            /**< wHubCharacteristics power switching mode (usb_host_hub_power_switching_t, 3 also means none) */
    bool compound;                      /**< Hub is part of a compound device */
    uint8_t over_current_protection;    /**< wHubCharacteristics over-current protection mode */
    uint16_t pwr_on_to_pwr_good_ms;     /**< Time from power-on to power-good on a port, ms */
} usb_host_hub_info_t;

/**
 * @brief Last known status of an external hub's port
 */
typedef struct {
    uint16_t port_status;               /**< wPortStatus (USB 2.0 Table 11-21) */
    uint16_t port_change;               /**< wPortChange (USB 2.0 Table 11-22) */
    bool user_power_off;                /**< Port was powered off with usb_host_hub_port_power() */
} usb_host_hub_port_info_t;

/**
 * @brief List the addresses of the configured external hubs
 *
 * @note Hubs are managed by the USB Host Library and are not reported to clients
 *       through USB_HOST_CLIENT_EVENT_NEW_DEV
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 *
 * @param[out] addrs    Hub device addresses
 * @param[in] max       Capacity of addrs
 * @param[out] count    Number of hubs (may exceed max)
 * @return
 *    - ESP_OK:                 List returned
 *    - ESP_ERR_TIMEOUT:        The USB Host Library did not process the request in time
 *    - ESP_ERR_NOT_SUPPORTED:  Hub support is disabled
 */
esp_err_t usb_host_hub_list(uint8_t *addrs, size_t max, size_t *count);

/**
 * @brief Get information about an external hub
 *
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 *
 * @param[in] dev_addr  Device address of the hub
 * @param[out] info     Hub information
 * @return
 *    - ESP_OK:                 Information returned
 *    - ESP_ERR_NOT_FOUND:      No configured hub with this address
 *    - ESP_ERR_INVALID_STATE:  The hub is not configured yet
 *    - ESP_ERR_TIMEOUT:        The USB Host Library did not process the request in time
 *    - ESP_ERR_NOT_SUPPORTED:  Hub support is disabled
 */
esp_err_t usb_host_hub_get_info(uint8_t dev_addr, usb_host_hub_info_t *info);

/**
 * @brief Get the last known status of an external hub's port
 *
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 *
 * @param[in] dev_addr  Device address of the hub
 * @param[in] port_num  Port number, starting at 1
 * @param[out] info     Port status
 * @return
 *    - ESP_OK:                 Status returned
 *    - ESP_ERR_INVALID_SIZE:   Port number out of range
 *    - see usb_host_hub_get_info() for other errors
 */
esp_err_t usb_host_hub_get_port_info(uint8_t dev_addr, uint8_t port_num, usb_host_hub_port_info_t *info);

/**
 * @brief Switch the power of an external hub's port (SetPortFeature/ClearPortFeature PORT_POWER)
 *
 * Powering a port off with a device attached disconnects the device, exactly like
 * unplugging it. The port stays off until it is powered on again. Powering it on
 * waits bPwrOn2PwrGood and then enumerates any connected device.
 *
 * @note Only hubs reporting per-port power switching switch a single port. Ganged hubs
 *       switch every port, and hubs without power switching ignore the request.
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 *
 * @param[in] dev_addr  Device address of the hub
 * @param[in] port_num  Port number, starting at 1
 * @param[in] enable    true to power the port on, false to power it off
 * @return
 *    - ESP_OK:                 Power change started
 *    - ESP_ERR_INVALID_SIZE:   Port number out of range
 *    - ESP_ERR_INVALID_STATE:  The hub is not configured, or the port is being reset
 *    - ESP_ERR_TIMEOUT:        The hub stayed busy handling other ports
 *    - see usb_host_hub_get_info() for other errors
 */
esp_err_t usb_host_hub_port_power(uint8_t dev_addr, uint8_t port_num, bool enable);

/**
 * @brief Log the state of the hub driver, external hubs, pending ports and the
 *        enumerator at WARNING level (diagnostics for stuck hubs)
 *
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 */
esp_err_t usb_host_hub_debug_dump(void);

#ifdef __cplusplus
}
#endif
