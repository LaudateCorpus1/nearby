// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CORE_INTERNAL_MEDIUMS_BLE_V2_BLE_PERIPHERAL_H_
#define CORE_INTERNAL_MEDIUMS_BLE_V2_BLE_PERIPHERAL_H_

#include "internal/platform/byte_array.h"

namespace location {
namespace nearby {
namespace connections {
namespace mediums {

// TODO(b/213835576): The peripheral class is for NearbyConnections to transmit
// "advertisement byte array" when peripheral discovered in
// 'discovered_peripheral_track' class, which is not the same as the one in
// BluetoothAdapter. We need to see how to differentiate between this
// BlePeripheral and BleV2Peripheral in BluetoothAdapter.
class BlePeripheral {
 public:
  BlePeripheral() = default;
  explicit BlePeripheral(const ByteArray& id) : id_(id) {}
  BlePeripheral(const BlePeripheral&) = default;
  BlePeripheral& operator=(const BlePeripheral&) = default;
  BlePeripheral(BlePeripheral&&) = default;
  BlePeripheral& operator=(BlePeripheral&&) = default;
  ~BlePeripheral() = default;

  bool IsValid() const { return !id_.Empty(); }
  ByteArray GetId() const { return id_; }

 private:
  // A unique identifier for this peripheral. It is the BLE advertisement it
  // was found on.
  ByteArray id_;
};

}  // namespace mediums
}  // namespace connections
}  // namespace nearby
}  // namespace location

#endif  // CORE_INTERNAL_MEDIUMS_BLE_V2_BLE_PERIPHERAL_H_
