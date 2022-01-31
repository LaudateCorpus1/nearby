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

#include "connections/clients/windows/core_adapter.h"

#include "absl/strings/str_format.h"
#include "connections/core.h"

namespace location {
namespace nearby {
namespace windows {

Core *InitCore(connections::ServiceControllerRouter *router) {
  return new nearby::connections::Core(router);
}

void CloseCore(Core *pCore) {
  if (pCore) {
    pCore->StopAllEndpoints(
        {.result_cb = std::function<void(Status)>{[](Status) {}}});
    delete pCore;
  }
}

void StartAdvertising(Core *pCore, const char *service_id,
                      AdvertisingOptionsW advertising_options_w,
                      ConnectionRequestInfoW info, ResultCallbackW callback) {
  if (pCore) {
    connections::ConnectionRequestInfo crInfo =
        connections::ConnectionRequestInfo();

    crInfo.endpoint_info = ByteArray(info.endpoint_info);
    crInfo.listener = std::move(*(info.listener.GetpImpl()));

    connections::AdvertisingOptions advertising_options;

    advertising_options.allowed.ble = advertising_options_w.allowed.ble;
    advertising_options.allowed.bluetooth =
        advertising_options_w.allowed.bluetooth;
    advertising_options.allowed.web_rtc = advertising_options_w.allowed.web_rtc;
    advertising_options.allowed.wifi_lan =
        advertising_options_w.allowed.wifi_lan;
    advertising_options.auto_upgrade_bandwidth =
        advertising_options_w.auto_upgrade_bandwidth;
    advertising_options.enable_bluetooth_listening =
        advertising_options_w.enable_bluetooth_listening;
    advertising_options.enable_webrtc_listening =
        advertising_options_w.enable_webrtc_listening;
    advertising_options.enforce_topology_constraints =
        advertising_options_w.enforce_topology_constraints;
    advertising_options.fast_advertisement_service_uuid =
        advertising_options_w.fast_advertisement_service_uuid;
    advertising_options.is_out_of_band_connection =
        advertising_options_w.is_out_of_band_connection;
    advertising_options.low_power = advertising_options_w.low_power;
    if (advertising_options_w.strategy == StrategyW::kNone)
      advertising_options.strategy = connections::Strategy::kNone;
    if (advertising_options_w.strategy == StrategyW::kP2pCluster)
      advertising_options.strategy = connections::Strategy::kP2pCluster;
    if (advertising_options_w.strategy == StrategyW::kP2pPointToPoint)
      advertising_options.strategy = connections::Strategy::kP2pPointToPoint;
    if (advertising_options_w.strategy == StrategyW::kP2pStar)
      advertising_options.strategy = connections::Strategy::kP2pStar;

    pCore->StartAdvertising(service_id, advertising_options, crInfo,
                            *callback.GetpImpl());
  }
}

void StopAdvertising(connections::Core *pCore, ResultCallbackW callback) {
  if (pCore) {
    pCore->StopAdvertising(*callback.GetpImpl());
  }
}

void StartDiscovery(connections::Core *pCore, const char *service_id,
                    windows::DiscoveryOptionsW discovery_options_w,
                    windows::DiscoveryListenerW listener,
                    windows::ResultCallbackW callback) {
  if (pCore) {
    connections::DiscoveryListener discoveryListener =
        std::move(*listener.GetpImpl());

    connections::DiscoveryOptions discovery_options;

    discovery_options.allowed.ble = discovery_options_w.allowed.ble;
    discovery_options.allowed.bluetooth = discovery_options_w.allowed.bluetooth;
    discovery_options.allowed.web_rtc = discovery_options_w.allowed.web_rtc;
    discovery_options.allowed.wifi_lan = discovery_options_w.allowed.wifi_lan;
    discovery_options.auto_upgrade_bandwidth =
        discovery_options_w.auto_upgrade_bandwidth;
    discovery_options.enforce_topology_constraints =
        discovery_options_w.enforce_topology_constraints;
    if (discovery_options_w.fast_advertisement_service_uuid) {
      discovery_options.fast_advertisement_service_uuid =
          std::string(discovery_options_w.fast_advertisement_service_uuid);
    }
    discovery_options.is_out_of_band_connection =
        discovery_options_w.is_out_of_band_connection;
    discovery_options.keep_alive_interval_millis =
        discovery_options_w.keep_alive_interval_millis;
    discovery_options.keep_alive_timeout_millis =
        discovery_options_w.keep_alive_timeout_millis;
    if (discovery_options_w.strategy == StrategyW::kNone)
      discovery_options.strategy = connections::Strategy::kNone;
    if (discovery_options_w.strategy == StrategyW::kP2pCluster)
      discovery_options.strategy = connections::Strategy::kP2pCluster;
    if (discovery_options_w.strategy == StrategyW::kP2pPointToPoint)
      discovery_options.strategy = connections::Strategy::kP2pPointToPoint;
    if (discovery_options_w.strategy == StrategyW::kP2pStar)
      discovery_options.strategy = connections::Strategy::kP2pStar;

    pCore->StartDiscovery(service_id, discovery_options, discoveryListener,
                          std::move(*callback.GetpImpl()));
  }
}

void StopDiscovery(connections::Core *pCore, ResultCallbackW callback) {
  if (pCore) {
    pCore->StopDiscovery(*callback.GetpImpl());
  }
}

void InjectEndpoint(connections::Core *pCore, char *service_id,
                    OutOfBandConnectionMetadataW metadata,
                    ResultCallbackW callback) {
  if (pCore) {
    connections::OutOfBandConnectionMetadata outOfBandConnectionMetadata;
    outOfBandConnectionMetadata.endpoint_id = metadata.endpoint_id;
    outOfBandConnectionMetadata.endpoint_info = {metadata.endpoint_info,
                                                 metadata.endpoint_info_size};
    outOfBandConnectionMetadata.medium = metadata.medium;
    outOfBandConnectionMetadata.remote_bluetooth_mac_address = {
        metadata.remote_bluetooth_mac_address,
        metadata.remote_bluetooth_mac_address_size};

    pCore->InjectEndpoint(service_id, outOfBandConnectionMetadata,
                          *callback.GetpImpl());
  }
}

void RequestConnection(connections::Core *pCore, const char *endpoint_id,
                       windows::ConnectionRequestInfoW info,
                       windows::ConnectionOptionsW connection_options_w,
                       ResultCallbackW callback) {
  if (pCore) {
    connections::ConnectionRequestInfo connectionRequestInfo =
        connections::ConnectionRequestInfo();
    connectionRequestInfo.endpoint_info = ByteArray(info.endpoint_info);
    connectionRequestInfo.listener = std::move(*info.listener.GetpImpl());

    connections::ConnectionOptions connection_options;
    connection_options.allowed.ble = connection_options_w.allowed.ble;
    connection_options.allowed.bluetooth =
        connection_options_w.allowed.bluetooth;
    connection_options.allowed.web_rtc = connection_options_w.allowed.web_rtc;
    connection_options.allowed.wifi_lan = connection_options_w.allowed.wifi_lan;
    connection_options.auto_upgrade_bandwidth =
        connection_options_w.auto_upgrade_bandwidth;
    connection_options.enable_bluetooth_listening =
        connection_options_w.enable_bluetooth_listening;
    connection_options.enable_webrtc_listening =
        connection_options_w.enable_webrtc_listening;
    connection_options.enforce_topology_constraints =
        connection_options_w.enforce_topology_constraints;
    if (connection_options_w.fast_advertisement_service_uuid) {
      connection_options.fast_advertisement_service_uuid =
          std::string(connection_options_w.fast_advertisement_service_uuid);
    }
    connection_options.is_out_of_band_connection =
        connection_options_w.is_out_of_band_connection;
    connection_options.keep_alive_interval_millis =
        connection_options_w.keep_alive_interval_millis;
    connection_options.keep_alive_timeout_millis =
        connection_options_w.keep_alive_timeout_millis;
    connection_options.low_power = connection_options_w.low_power;
    if (connection_options_w.remote_bluetooth_mac_address) {
      connection_options.remote_bluetooth_mac_address =
          ByteArray(connection_options_w.remote_bluetooth_mac_address,
                    strlen(connection_options_w.remote_bluetooth_mac_address));
    }
    if (connection_options_w.strategy == StrategyW::kNone)
      connection_options.strategy = connections::Strategy::kNone;
    if (connection_options_w.strategy == StrategyW::kP2pCluster)
      connection_options.strategy = connections::Strategy::kP2pCluster;
    if (connection_options_w.strategy == StrategyW::kP2pPointToPoint)
      connection_options.strategy = connections::Strategy::kP2pPointToPoint;
    if (connection_options_w.strategy == StrategyW::kP2pStar)
      connection_options.strategy = connections::Strategy::kP2pStar;

    pCore->RequestConnection(endpoint_id, connectionRequestInfo,
                             connection_options, *callback.GetpImpl());
  }
}

void AcceptConnection(connections::Core *pCore, const char *endpoint_id,
                      PayloadListenerW listener, ResultCallbackW callback) {
  if (pCore) {
    connections::PayloadListener payload_listener =
        std::move(*listener.GetpImpl());
    pCore->AcceptConnection(endpoint_id, payload_listener,
                            *callback.GetpImpl());
  }
}

void RejectConnection(connections::Core *pCore, const char *endpoint_id,
                      ResultCallbackW callback) {
  if (pCore) {
    pCore->RejectConnection(endpoint_id, *callback.GetpImpl());
  }
}

void SendPayload(connections::Core *pCore,
                 // todo(jfcarroll) this is being exported, needs to be
                 // refactored to return a plain old c type
                 const char *endpoint_ids, size_t endpoint_ids_size,
                 windows::PayloadW payloadw, ResultCallbackW callback) {
  if (pCore) {
    auto payload = payloadw.GetpImpl();
    std::string payloadData = std::string(endpoint_ids);
    absl::Span<const std::string> span{&payloadData, 1};
    pCore->SendPayload(span, std::move(*payload), *callback.GetpImpl());
  }
}

void CancelPayload(connections::Core *pCore, std::int64_t payload_id,
                   ResultCallbackW callback) {
  if (pCore) {
    pCore->CancelPayload(payload_id, *callback.GetpImpl());
  }
}

void DisconnectFromEndpoint(connections::Core *pCore, char *endpoint_id,
                            ResultCallbackW callback) {
  if (pCore) {
    pCore->DisconnectFromEndpoint(endpoint_id, *callback.GetpImpl());
  }
}

void StopAllEndpoints(connections::Core *pCore, ResultCallbackW callback) {
  if (pCore) {
    pCore->StopAllEndpoints(*callback.GetpImpl());
  }
}

void InitiateBandwidthUpgrade(connections::Core *pCore, char *endpoint_id,
                              ResultCallbackW callback) {
  if (pCore) {
    pCore->InitiateBandwidthUpgrade(endpoint_id, *callback.GetpImpl());
  }
}

const char *GetLocalEndpointId(connections::Core *pCore) {
  if (pCore) {
    std::string endpoint_id = pCore->GetLocalEndpointId();
    char *result = new char[endpoint_id.length() + 1];
    absl::SNPrintF(result, endpoint_id.length() + 1, "%s", endpoint_id);
    return result;
  }
  return "Null-Core";
}

connections::ServiceControllerRouter *InitServiceControllerRouter() {
  return new connections::ServiceControllerRouter();
}

void CloseServiceControllerRouter(
    connections::ServiceControllerRouter *pRouter) {
  if (pRouter) {
    delete pRouter;
  }
}

}  // namespace windows
}  // namespace nearby
}  // namespace location
