#ifndef UTIL_TLV_LIST_H
#define UTIL_TLV_LIST_H

/** \file
  * \brief Support for lists of type-length-value items.
  * \author Sander Vanheule (Universiteit Gent)
  */

#include "memspace.h"
#include <stdint.h>

/** \brief Item in a TLV list.
  * \details Type-length-value or TLV lists provide a storage object for multiple data types.
  *   The type provides an identifier which can be used to interpret the data, while the length
  *   ensures this interpretation is not required to correctly parse the list.
  *   The value is provided as a pointer to the data, with an associated memspace indication.
  */
struct dp_tlv_item_t {
  uint8_t type; ///< Type of display data.
  uint8_t length; ///< Length of this field's data not including the two type and length bytes.
  enum memspace_t memspace; ///< Memory region the TLV data resides in.
  const void* data; ///< Pointer to the memory block contain the TLV data.
};

/// Last field in TLV list with length 0. Same value as unprogrammed flash/EEPROM to lower
/// chances of get_tlv_list_length_P() failing when it is provided with a bad pointer.
#define TLV_TYPE_END 0xFF

/// Calculate the total length of the TLV list stored in program memory,
/// not including a ::TLV_TYPE_END field.
uint16_t get_tlv_list_length_P(const struct dp_tlv_item_t* tlv_data);

/// Macro for TLV list item
#define TLV_ENTRY(type, memspace, address) {type, sizeof(*address), memspace, address}

/// Macro for last item in TLV list
#define TLV_END {TLV_TYPE_END, 0, MEMSPACE_NONE, 0}

#endif // UTIL_TLV_LIST_H
