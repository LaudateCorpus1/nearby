// Copyright 2021 Google LLC
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

#include "connections/clients/windows/dart/core_adapter_dart.h"

#include <shlobj.h>

#include <cstdint>
#include <string>

#include "connections/clients/windows/advertising_options_w.h"
#include "connections/clients/windows/connection_options_w.h"
#include "connections/clients/windows/discovery_options_w.h"
#include "connections/clients/windows/listeners_w.h"
#include "connections/clients/windows/out_of_band_connection_metadata_w.h"
#include "connections/clients/windows/strategy_w.h"
#include "internal/platform/file.h"
#include "internal/platform/logging.h"

namespace location {
namespace nearby {
namespace windows {

static ConnectionRequestInfoDart *connectionRequestInfoDart;
static DiscoveryListenerDart *discoveryListenerDart;
static PayloadListenerDart *payloadListenerDart;
static Dart_Port *dartPort;

StrategyW GetStrategy(StrategyDart strategy) {
  switch (strategy) {
    case StrategyDart::P2P_CLUSTER:
      return StrategyW::kP2pCluster;
    case StrategyDart::P2P_POINT_TO_POINT:
      return StrategyW::kP2pPointToPoint;
    case StrategyDart::P2P_STAR:
      return StrategyW::kP2pStar;
  }
  return StrategyW::kNone;
}

ByteArray ConvertBluetoothMacAddress(absl::string_view address) {
  return ByteArray(address.data());
}

void DartResultCB(Status status) {
  Dart_CObject dart_object_result_callback;
  dart_object_result_callback.type = Dart_CObject_kInt64;
  dart_object_result_callback.value.as_int64 = status.value;
  const bool result =
      Dart_PostCObject_DL(*dartPort, &dart_object_result_callback);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void SetResultCallback(ResultCallbackW &callback, Dart_Port &port) {
  dartPort = &port;

  callback.result_cb = DartResultCB;
}

void PostResult(Dart_Port &result_cb, Status::Value value) {
  Dart_CObject dart_object_result_callback;
  dart_object_result_callback.type = Dart_CObject_kInt64;
  dart_object_result_callback.value.as_int64 = value;
  const bool result =
      Dart_PostCObject_DL(result_cb, &dart_object_result_callback);
  if (!result) {
    NEARBY_LOG(INFO, "Returning error to port failed.");
  }
}

void ListenerInitiatedCB(const char *endpoint_id,
                         const ConnectionResponseInfoW &connection_info) {
  NEARBY_LOG(INFO, "Advertising initiated: id=%s", endpoint_id);

  Dart_CObject dart_object_endpoint_id;
  dart_object_endpoint_id.type = Dart_CObject_kString;
  dart_object_endpoint_id.value.as_string = const_cast<char *>(endpoint_id);

  Dart_CObject dart_object_endpoint_info;
  dart_object_endpoint_info.type = Dart_CObject_kString;
  dart_object_endpoint_info.value.as_string =
      const_cast<char *>(connection_info.remote_endpoint_info);

  Dart_CObject *elements[2];
  elements[0] = &dart_object_endpoint_id;
  elements[1] = &dart_object_endpoint_info;

  Dart_CObject dart_object_initiated;
  dart_object_initiated.type = Dart_CObject_kArray;
  dart_object_initiated.value.as_array.length = 2;
  dart_object_initiated.value.as_array.values = elements;

  const bool result = Dart_PostCObject_DL(
      connectionRequestInfoDart->initiated_cb, &dart_object_initiated);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerAcceptedCB(const char *endpoint_id) {
  NEARBY_LOG(INFO, "Advertising accepted: id=%s", endpoint_id);
  Dart_CObject dart_object_accepted;
  dart_object_accepted.type = Dart_CObject_kString;
  dart_object_accepted.value.as_string = const_cast<char *>(endpoint_id);
  const bool result = Dart_PostCObject_DL(
      connectionRequestInfoDart->accepted_cb, &dart_object_accepted);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerRejectedCB(const char *endpoint_id, Status status) {
  NEARBY_LOG(INFO, "Advertising rejected: id=%s", endpoint_id);
  Dart_CObject dart_object_rejected;
  dart_object_rejected.type = Dart_CObject_kString;
  dart_object_rejected.value.as_string = const_cast<char *>(endpoint_id);
  const bool result = Dart_PostCObject_DL(
      connectionRequestInfoDart->rejected_cb, &dart_object_rejected);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerDisconnectedCB(const char *endpoint_id) {
  NEARBY_LOG(INFO, "Advertising disconnected: id=%s", endpoint_id);
  Dart_CObject dart_object_disconnected;
  dart_object_disconnected.type = Dart_CObject_kString;
  dart_object_disconnected.value.as_string = const_cast<char *>(endpoint_id);
  const bool result = Dart_PostCObject_DL(
      connectionRequestInfoDart->disconnected_cb, &dart_object_disconnected);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerBandwidthChangedCB(const char *endpoint_id, MediumW medium) {
  NEARBY_LOG(INFO, "Advertising bandwidth changed: id=%s", endpoint_id);
  Dart_CObject dart_object_bandwidth_changed;

  dart_object_bandwidth_changed.type = Dart_CObject_kString;
  dart_object_bandwidth_changed.value.as_string =
      const_cast<char *>(endpoint_id);
  const bool result =
      Dart_PostCObject_DL(connectionRequestInfoDart->bandwidth_changed_cb,
                          &dart_object_bandwidth_changed);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void StartAdvertisingDart(Core *pCore, const char *service_id,
                          ConnectionOptionsDart options_dart,
                          ConnectionRequestInfoDart info_dart,
                          Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  connectionRequestInfoDart = &info_dart;

  AdvertisingOptionsW advertising_options;
  advertising_options.strategy = GetStrategy(options_dart.strategy);
  advertising_options.auto_upgrade_bandwidth =
      options_dart.auto_upgrade_bandwidth;
  advertising_options.enforce_topology_constraints =
      options_dart.enforce_topology_constraints;
  advertising_options.allowed.bluetooth = options_dart.enable_bluetooth;
  advertising_options.allowed.ble = options_dart.enable_ble;
  advertising_options.low_power = options_dart.use_low_power_mode;
  advertising_options.fast_advertisement_service_uuid =
      options_dart.discover_fast_advertisements
          ? "0000FE2C-0000-1000-8000-00805F9B34FB"
          : "";
  advertising_options.allowed.wifi_lan = options_dart.enable_wifi_lan;
  advertising_options.allowed.web_rtc = options_dart.enable_web_rtc;

  ConnectionListenerW connectionListenerW;

  connectionListenerW.initiated_cb = ListenerInitiatedCB;
  connectionListenerW.accepted_cb = ListenerAcceptedCB;
  connectionListenerW.rejected_cb = ListenerRejectedCB;
  connectionListenerW.disconnected_cb = ListenerDisconnectedCB;
  connectionListenerW.bandwidth_changed_cb = ListenerBandwidthChangedCB;

  ConnectionRequestInfoW info{info_dart.endpoint_info,
                              strlen(info_dart.endpoint_info),
                              connectionListenerW};

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  StartAdvertising(pCore, service_id, std::move(advertising_options), info,
                   callback);
}

void StopAdvertisingDart(Core *pCore, Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  StopAdvertising(pCore, callback);
}

void ListenerEndpointFoundCB(const char *endpoint_id, const char *endpoint_info,
                             const size_t endpoint_info_size,
                             const char *str_service_id) {
  NEARBY_LOG(INFO, "Device discovered: id=%s", endpoint_id);
  NEARBY_LOG(INFO, "Device discovered: service_id=%s", str_service_id);
  NEARBY_LOG(INFO, "Device discovered: info=%s", endpoint_info);

  Dart_CObject dart_object_endpoint_id;
  dart_object_endpoint_id.type = Dart_CObject_kString;
  dart_object_endpoint_id.value.as_string = const_cast<char *>(endpoint_id);

  Dart_CObject dart_object_endpoint_info;
  dart_object_endpoint_info.type = Dart_CObject_kString;
  dart_object_endpoint_info.value.as_string = const_cast<char *>(endpoint_info);

  Dart_CObject *elements[2];
  elements[0] = &dart_object_endpoint_id;
  elements[1] = &dart_object_endpoint_info;

  Dart_CObject dart_object_found;
  dart_object_found.type = Dart_CObject_kArray;
  dart_object_found.value.as_array.length = 2;
  dart_object_found.value.as_array.values = elements;
  const bool result =
      Dart_PostCObject_DL(discoveryListenerDart->found_cb, &dart_object_found);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerEndpointLostCB(const char *endpoint_id) {
  NEARBY_LOG(INFO, "Device lost: id=%s", endpoint_id);
  Dart_CObject dart_object_lost;
  dart_object_lost.type = Dart_CObject_kString;
  dart_object_lost.value.as_string = const_cast<char *>(endpoint_id);
  const bool result =
      Dart_PostCObject_DL(discoveryListenerDart->lost_cb, &dart_object_lost);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerEndpointDistanceChangedCB(const char *endpoint_id,
                                       DistanceInfoW info) {
  NEARBY_LOG(INFO, "Device distance changed: id=%s", endpoint_id);
  Dart_CObject dart_object_distance_changed;
  dart_object_distance_changed.type = Dart_CObject_kString;
  dart_object_distance_changed.value.as_string =
      const_cast<char *>(endpoint_id);
  const bool result =
      Dart_PostCObject_DL(discoveryListenerDart->distance_changed_cb,
                          &dart_object_distance_changed);
  if (!result) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void StartDiscoveryDart(Core *pCore, const char *service_id,
                        ConnectionOptionsDart options_dart,
                        DiscoveryListenerDart listener_dart,
                        Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  discoveryListenerDart = &listener_dart;

  DiscoveryOptionsW discovery_options;
  discovery_options.strategy = GetStrategy(options_dart.strategy);
  discovery_options.allowed.bluetooth = options_dart.enable_bluetooth;
  discovery_options.allowed.ble = options_dart.enable_ble;
  discovery_options.allowed.wifi_lan = options_dart.enable_wifi_lan;
  discovery_options.allowed.web_rtc = false;
  discovery_options.enforce_topology_constraints = true;
  discovery_options.fast_advertisement_service_uuid =
      options_dart.discover_fast_advertisements
          ? "0000FE2C-0000-1000-8000-00805F9B34FB"
          : "";

  DiscoveryListenerW listener;
  listener.endpoint_found_cb = ListenerEndpointFoundCB;
  listener.endpoint_lost_cb = ListenerEndpointLostCB;
  listener.endpoint_distance_changed_cb = ListenerEndpointDistanceChangedCB;

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  StartDiscovery(pCore, service_id, std::move(discovery_options), listener,
                 callback);
}

void StopDiscoveryDart(Core *pCore, Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  StopDiscovery(pCore, callback);
}

void RequestConnectionDart(Core *pCore, const char *endpoint_id,
                           ConnectionOptionsDart options_dart,
                           ConnectionRequestInfoDart info_dart,
                           Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  ConnectionOptionsW connection_options;
  connection_options.enforce_topology_constraints = false;
  connection_options.allowed.bluetooth = options_dart.enable_bluetooth;
  connection_options.allowed.ble = options_dart.enable_ble;
  connection_options.allowed.wifi_lan = options_dart.enable_wifi_lan;
  connection_options.allowed.web_rtc = options_dart.enable_web_rtc;

  ConnectionListenerW connectionListenerW;
  connectionListenerW.initiated_cb = ListenerInitiatedCB;
  connectionListenerW.accepted_cb = ListenerAcceptedCB;
  connectionListenerW.rejected_cb = ListenerRejectedCB;
  connectionListenerW.disconnected_cb = ListenerDisconnectedCB;
  connectionListenerW.bandwidth_changed_cb = ListenerBandwidthChangedCB;

  ConnectionRequestInfoW info{info_dart.endpoint_info,
                              strlen(info_dart.endpoint_info),
                              connectionListenerW};

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  RequestConnection(pCore, endpoint_id, info, std::move(connection_options),
                    callback);
}

void ListenerPayloadCB(const char *endpoint_id, PayloadW payload) {
  NEARBY_LOG(INFO,
             "Payload callback called. id: %s, "
             "payload_id: %d, type: %d, offset: %d",
             endpoint_id, payload.GetId(), payload.GetType(),
             payload.GetOffset());

  Dart_CObject dart_object_endpoint_id;
  dart_object_endpoint_id.type = Dart_CObject_kString;
  dart_object_endpoint_id.value.as_string = const_cast<char *>(endpoint_id);

  Dart_CObject dart_object_payload_id;
  dart_object_payload_id.type = Dart_CObject_kInt64;
  dart_object_payload_id.value.as_int64 = payload.GetId();

  Dart_CObject dart_object_payload_type;
  dart_object_payload_type.type = Dart_CObject_kInt64;
  dart_object_payload_type.value.as_int64 = (int)payload.GetType();

  Dart_CObject dart_object_offset;
  dart_object_offset.type = Dart_CObject_kInt64;
  dart_object_offset.value.as_int64 = payload.GetOffset();

  Dart_CObject *elements[4];
  elements[0] = &dart_object_endpoint_id;
  elements[1] = &dart_object_payload_id;
  elements[2] = &dart_object_payload_type;
  elements[3] = &dart_object_offset;

  Dart_CObject dart_object_payload;
  dart_object_payload.type = Dart_CObject_kArray;
  dart_object_payload.value.as_array.length = 4;
  dart_object_payload.value.as_array.values = elements;

  if (!Dart_PostCObject_DL(payloadListenerDart->payload_cb,
                           &dart_object_payload)) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void ListenerPayloadProgressCB(const char *endpoint_id,
                               const PayloadProgressInfoW &info) {
  NEARBY_LOG(INFO,
             "Payload progress callback called. id: %s, "
             "payload_id: %d, bytes transferred: %d, total: %d, status: %d",
             endpoint_id, info.payload_id, info.bytes_transferred,
             info.total_bytes, info.status);
  Dart_CObject dart_object_endpoint_id;
  dart_object_endpoint_id.type = Dart_CObject_kString;
  dart_object_endpoint_id.value.as_string = const_cast<char *>(endpoint_id);

  Dart_CObject dart_object_payload_id;
  dart_object_payload_id.type = Dart_CObject_kInt64;
  dart_object_payload_id.value.as_int64 = info.payload_id;

  Dart_CObject dart_object_bytes_transferred;
  dart_object_bytes_transferred.type = Dart_CObject_kInt64;
  dart_object_bytes_transferred.value.as_int64 = info.bytes_transferred;

  Dart_CObject dart_object_total_bytes;
  dart_object_total_bytes.type = Dart_CObject_kInt64;
  dart_object_total_bytes.value.as_int64 = info.total_bytes;

  Dart_CObject dart_object_status;
  dart_object_status.type = Dart_CObject_kInt64;
  dart_object_status.value.as_int64 = (int64_t)info.status;

  Dart_CObject *elements[5];
  elements[0] = &dart_object_endpoint_id;
  elements[1] = &dart_object_payload_id;
  elements[2] = &dart_object_bytes_transferred;
  elements[3] = &dart_object_total_bytes;
  elements[4] = &dart_object_status;

  Dart_CObject dart_object_payload_progress;
  dart_object_payload_progress.type = Dart_CObject_kArray;
  dart_object_payload_progress.value.as_array.length = 5;
  dart_object_payload_progress.value.as_array.values = elements;

  if (!Dart_PostCObject_DL(payloadListenerDart->payload_progress_cb,
                           &dart_object_payload_progress)) {
    NEARBY_LOG(INFO, "Posting message to port failed.");
  }
}

void AcceptConnectionDart(Core *pCore, const char *endpoint_id,
                          PayloadListenerDart listener_dart,
                          Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  payloadListenerDart = &listener_dart;

  PayloadListenerW listener;
  listener.payload_cb = ListenerPayloadCB;
  listener.payload_progress_cb = ListenerPayloadProgressCB;

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  AcceptConnection(pCore, endpoint_id, listener, callback);
}

void DisconnectFromEndpointDart(Core *pCore, char *endpoint_id,
                                Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  ResultCallbackW callback;
  SetResultCallback(callback, result_cb);
  DisconnectFromEndpoint(pCore, endpoint_id, callback);
}

std::string GetPayloadPath(location::nearby::PayloadId payload_id) {
  PWSTR basePath;

  // Retrieves the full path of a known folder identified by the folder's
  // KNOWNFOLDERID.
  // https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
  SHGetKnownFolderPath(
      FOLDERID_Downloads,  //  rfid: A reference to the KNOWNFOLDERID that
                           //  identifies the folder.
      0,           // dwFlags: Flags that specify special retrieval options.
      NULL,        // hToken: An access token that represents a particular user.
      &basePath);  // ppszPath: When this method returns, contains the address
                   // of a pointer to a null-terminated Unicode string that
                   // specifies the path of the known folder. The calling
                   // process is responsible for freeing this resource once it
                   // is no longer needed by calling CoTaskMemFree, whether
                   // SHGetKnownFolderPath succeeds or not.
  size_t bufferSize;
  wcstombs_s(&bufferSize, NULL, 0, basePath, 0);
  char *fullpathUTF8 = new char[bufferSize + 1];
  memset(fullpathUTF8, 0, bufferSize);
  wcstombs_s(&bufferSize, fullpathUTF8, bufferSize, basePath, bufferSize - 1);
  std::string fullPath = std::string(fullpathUTF8);
  auto retval = absl::StrCat(fullPath, "\\", payload_id);
  delete[] fullpathUTF8;
  return retval;
}

void SendPayloadDart(Core *pCore, const char *endpoint_id,
                     PayloadDart payload_dart, Dart_Port result_cb) {
  if (!pCore) {
    PostResult(result_cb, Status::Value::kError);
    return;
  }

  ResultCallbackW callback;
  std::vector<std::string> endpoint_ids = {std::string(endpoint_id)};

  NEARBY_LOG(INFO, "Payload type: %d", payload_dart.type);
  switch (payload_dart.type) {
    case UNKNOWN:
    case STREAM:
      NEARBY_LOG(INFO, "Payload type not supported yet");
      PostResult(result_cb, Status::Value::kPayloadUnknown);
      break;
    case BYTE: {
      PayloadW payload(PayloadW::GenerateId(), payload_dart.data,
                       payload_dart.size);
      // TODO(jfcarroll): Fix this, SendPayload takes a char* which is wrong
      // it needs to take an array of char*
      SendPayload(pCore, endpoint_ids[0].c_str(), 1, std::move(payload),
                  callback);
    }
      SetResultCallback(callback, result_cb);
      break;
    case FILE:
      NEARBY_LOG(INFO, "File name: %s, size %d", payload_dart.data,
                 payload_dart.size);
      std::string file_name_str(payload_dart.data);

      // TODO(yanfangliu) Clean this up when John's file payload change rolls
      // out
      PayloadId id = std::hash<std::string>()(file_name_str);
      std::string download_path = GetPayloadPath(id);
      CopyFileA((LPSTR)payload_dart.data, (LPSTR)download_path.c_str(),
                /*FailIfFileAlreadyExists=*/false);
      NEARBY_LOGS(INFO) << "Copy File to " << download_path;

      InputFile input_file(std::to_string(id), payload_dart.size);
      PayloadW payload = PayloadW(id, std::move(input_file));
      // TODO(jfcarroll): Fix this, SendPayload takes a char* which is wrong
      // it needs to take an array of char*
      SendPayload(pCore, endpoint_ids[0].c_str(), 1, std::move(payload),
                  callback);

      SetResultCallback(callback, result_cb);

      break;
  }
}
}  // namespace windows
}  // namespace nearby
}  // namespace location
