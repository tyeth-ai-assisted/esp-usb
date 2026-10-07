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
    uint16_t characteristics;           /**< Raw wHubCharacteristics (USB 2.0 Table 11-13) */
    uint8_t power_switching;            /**< D1..D0: power switching mode (usb_host_hub_power_switching_t, 3 also means none) */
    bool compound;                      /**< D2: hub is part of a compound device */
    uint8_t over_current_protection;    /**< D4..D3: 0 global, 1 per-port, 2/3 none */
    uint8_t tt_think_time;              /**< D6..D5: TT think time in units of 8 FS bit times, minus 1 (0 = 8 ... 3 = 32) */
    bool port_indicators;               /**< D7: port indicator LEDs supported */
    uint16_t pwr_on_to_pwr_good_ms;     /**< Time from power-on to power-good on a port, ms */
    uint8_t hub_contr_current_ma;       /**< bHubContrCurrent: hub controller current, mA */
} usb_host_hub_info_t;

/**
 * @brief Maximum number of ports returned by usb_host_hub_get_snapshot()
 */
#define USB_HOST_HUB_SNAPSHOT_MAX_PORTS 31

/**
 * @brief usb_host_hub_port_power() flags
 */
#define USB_HOST_HUB_PORT_POWER_FLAG_FORCE  (1U << 0)  /**< Switch even if the hub does not report per-port power switching */

/**
 * @brief Location of a downstream hub port, for the port power policy callback
 *
 * ports[] lists the port numbers from the hub on the root port down to this
 * port: a port of the hub on the root port has depth 1 ({port}), a port of a
 * hub plugged into port 2 of that hub has depth 2 ({2, port}), and so on.
 */
typedef struct {
    uint8_t hub_addr;                   /**< Device address of the hub owning the port */
    uint8_t port_num;                   /**< Port number on that hub */
    uint8_t depth;                      /**< Number of entries in ports[] */
    uint8_t ports[7];                   /**< Port numbers from the root hub down to this port */
} usb_host_hub_port_path_t;

/**
 * @brief Port power policy callback
 *
 * Called from the USB Host Library task when an external hub's port is created
 * (hub enumeration, re-enumeration after a reset or upstream power loss). Return
 * false to keep the port powered off: it then never gets powered, exactly as if
 * usb_host_hub_port_power(..., false, ...) had been called. Must not block or
 * call USB Host Library functions.
 */
typedef bool (*usb_host_hub_port_policy_cb_t)(const usb_host_hub_port_path_t *path, void *arg);

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
 * @brief Get a hub's information and the status of all of its ports in one request
 *
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 *
 * @param[in] dev_addr      Device address of the hub
 * @param[out] info         Hub information
 * @param[out] ports        Port status, ports[i] is port i + 1
 * @param[in] max_ports     Capacity of ports
 * @param[out] num_ports    Number of ports filled (min(info.num_ports, max_ports, USB_HOST_HUB_SNAPSHOT_MAX_PORTS))
 * @return see usb_host_hub_get_info()
 */
esp_err_t usb_host_hub_get_snapshot(uint8_t dev_addr, usb_host_hub_info_t *info,
                                    usb_host_hub_port_info_t *ports, size_t max_ports, size_t *num_ports);

/**
 * @brief Set the port power policy callback (NULL to remove)
 *
 * Can be called before usb_host_install(). Ports of hubs already enumerated are
 * not affected.
 */
void usb_host_hub_set_port_policy(usb_host_hub_port_policy_cb_t cb, void *arg);

/**
 * @brief Switch the power of an external hub's port (SetPortFeature/ClearPortFeature PORT_POWER)
 *
 * Powering a port off with a device attached disconnects the device, exactly like
 * unplugging it. The port stays off until it is powered on again. Powering it on
 * waits bPwrOn2PwrGood and then enumerates any connected device.
 *
 * Only hubs reporting per-port power switching (wHubCharacteristics D1..D0 = 01b)
 * switch a single port: ganged hubs switch every port (and may disturb the other
 * ports even without VBUS switches), and hubs without power switching ignore the
 * request. Such hubs are refused unless USB_HOST_HUB_PORT_POWER_FLAG_FORCE is set.
 *
 * @note Must not be called from the task that calls usb_host_lib_handle_events()
 *
 * @param[in] dev_addr  Device address of the hub
 * @param[in] port_num  Port number, starting at 1
 * @param[in] enable    true to power the port on, false to power it off
 * @param[in] flags     USB_HOST_HUB_PORT_POWER_FLAG_* flags
 * @return
 *    - ESP_OK:                 Power change started
 *    - ESP_ERR_NOT_SUPPORTED:  The hub does not support per-port power switching (and no FORCE flag)
 *    - ESP_ERR_INVALID_SIZE:   Port number out of range
 *    - ESP_ERR_INVALID_STATE:  The hub is not configured, or the port is being reset
 *    - ESP_ERR_TIMEOUT:        The hub stayed busy handling other ports
 *    - see usb_host_hub_get_info() for other errors
 */
esp_err_t usb_host_hub_port_power(uint8_t dev_addr, uint8_t port_num, bool enable, uint32_t flags);

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
