/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_network_monitor_private.idl,
 *   modified Wed Sep  4 14:16:07 2013.
 */

#ifndef PPAPI_C_PRIVATE_PPB_NETWORK_MONITOR_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_NETWORK_MONITOR_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_NETWORKMONITOR_PRIVATE_INTERFACE_0_3 \
    "PPB_NetworkMonitor_Private;0.3"
#define PPB_NETWORKMONITOR_PRIVATE_INTERFACE \
    PPB_NETWORKMONITOR_PRIVATE_INTERFACE_0_3

/**
 * @file
 * This file defines the <code>PPB_NetworkMonitor_Private</code> interface.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_NetworkMonitor_Private</code> provides access to
 * notifications of network configuration changes.
 */
struct PPB_NetworkMonitor_Private_0_3 {
  /**
   * Creates a Network Monitor resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying one instance of
   * a module.
   *
   * @return A <code>PP_Resource</code> corresponding to a network monitor or 0
   * on failure.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Gets current network configuration. When called for the first time,
   * completes as soon as the current network configuration is received from
   * the browser. Each consequent call will wait for network list changes,
   * returning a new <code>PPB_NetworkList</code> resource every time.
   *
   * @param[in] network_monitor A <code>PP_Resource</code> corresponding to a
   * network monitor.
   * @param[out] network_list The <code>PPB_NetworkList<code> resource with the
   * current state of network interfaces.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   * <code>PP_ERROR_NOACCESS</code> will be returned if the caller doesn't have
   * required permissions.
   */
  int32_t (*UpdateNetworkList)(PP_Resource network_monitor,
                               PP_Resource* network_list,
                               struct PP_CompletionCallback callback);
  /**
   * Determines if the specified <code>resource</code> is a
   * <code>NetworkMonitor</code> object.
   *
   * @param[in] resource A <code>PP_Resource</code> resource.
   *
   * @return Returns <code>PP_TRUE</code> if <code>resource</code> is
   * a <code>PPB_NetworkMonitor_Private</code>, <code>PP_FALSE</code>
   * otherwise.
   */
  PP_Bool (*IsNetworkMonitor)(PP_Resource resource);
};

typedef struct PPB_NetworkMonitor_Private_0_3 PPB_NetworkMonitor_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_NETWORK_MONITOR_PRIVATE_H_ */

