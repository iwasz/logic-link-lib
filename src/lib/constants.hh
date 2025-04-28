/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdint>

/*
 * Common library for cli and gui.
 */

/**
 * This size has to be requested by the USB host (PC). After sending exactly this
 * amount, USB bulk transaction finishes, and the client application running on the
 * host immediately issues another transfer of this size.
 */
// constexpr uint32_t DEFAULT_USB_TRANSFER_SIZE_B = 10 * 65536;
// constexpr uint32_t DEFAULT_USB_TRANSFER_SIZE_B = 32768;
constexpr uint32_t DEFAULT_USB_TRANSFER_SIZE_B = 16384;

/**
 * This size gets transferred from the buffer to the USB peripheral at once.
 */
constexpr uint32_t DEFAULT_USB_BLOCK_SIZE_B = 8192;