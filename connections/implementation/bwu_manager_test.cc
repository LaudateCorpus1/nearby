// Copyright 2022 Google LLC
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

#include "connections/implementation/bwu_manager.h"

#include <string>

#include "gtest/gtest.h"
#include "connections/implementation/client_proxy.h"
#include "connections/implementation/endpoint_channel.h"
#include "connections/implementation/endpoint_channel_manager.h"
#include "connections/implementation/endpoint_manager.h"
#include "connections/implementation/fake_bwu_handler.h"
#include "connections/implementation/fake_endpoint_channel.h"
#include "connections/implementation/mediums/mediums.h"
#include "connections/implementation/offline_frames.h"
#include "internal/platform/exception.h"

namespace location {
namespace nearby {
namespace connections {
namespace {

const char kServiceIdA[] = "ServiceA";
const char kServiceIdB[] = "ServiceB";
const char kEndpointId1[] = "Endpoint1";
const char kEndpointId2[] = "Endpoint2";
const char kEndpointId3[] = "Endpoint3";

class BwuManagerTest : public ::testing::TestWithParam<bool> {
 protected:
  BwuManagerTest() : support_multiple_bwu_mediums_(GetParam()) {}
  ~BwuManagerTest() override = default;

  void SetUp() override {
    FeatureFlags::GetMutableFlagsForTesting().support_multiple_bwu_mediums =
        support_multiple_bwu_mediums_;

    // Set the initial connection medium to Bluetooth for all endpoints.
    // Associate endpoints 1 and 2 with service A and endpoint 3 with service B.
    auto channel_1 =
        std::make_unique<FakeEndpointChannel>(Medium::BLUETOOTH, kServiceIdA);
    auto channel_2 =
        std::make_unique<FakeEndpointChannel>(Medium::BLUETOOTH, kServiceIdA);
    auto channel_3 =
        std::make_unique<FakeEndpointChannel>(Medium::BLUETOOTH, kServiceIdB);
    initial_channel_1_ = channel_1.get();
    initial_channel_2_ = channel_2.get();
    initial_channel_3_ = channel_3.get();
    ecm_.RegisterChannelForEndpoint(&client_, kEndpointId1,
                                    std::move(channel_1));
    ecm_.RegisterChannelForEndpoint(&client_, kEndpointId2,
                                    std::move(channel_2));
    ecm_.RegisterChannelForEndpoint(&client_, kEndpointId3,
                                    std::move(channel_3));

    // Set up fake BWU handlers for WebRTC and WifiLAN.
    absl::flat_hash_map<Medium, std::unique_ptr<BwuHandler>> handlers;
    auto fake_web_rtc = std::make_unique<FakeBwuHandler>(Medium::WEB_RTC);
    auto fake_wifi_lan =
        std::make_unique<FakeBwuHandler>(Medium::WIFI_LAN);
    fake_web_rtc_bwu_handler_ = fake_web_rtc.get();
    fake_wifi_lan_bwu_handler_ = fake_wifi_lan.get();
    handlers.emplace(Medium::WEB_RTC, std::move(fake_web_rtc));
    handlers.emplace(Medium::WIFI_LAN, std::move(fake_wifi_lan));

    BwuManager::Config config;
    config.allow_upgrade_to =
        BooleanMediumSelector{.web_rtc = true, .wifi_lan = true};

    bwu_manager_ = std::make_unique<BwuManager>(mediums_, em_, ecm_,
                                                std::move(handlers), config);

    // Don't run tasks on other threads. Avoids race conditions in tests.
    bwu_manager_->MakeSingleThreadedForTesting();
  }

  void TearDown() override { bwu_manager_->Shutdown(); }

  // Upgrade to |medium|, close down the BLUETOOTH channel, return the upgraded
  // endpoint channel.
  FakeEndpointChannel* FullyUpgradeEndpoint(const std::string& endpoint_id,
                                            Medium medium) {
    FakeBwuHandler* handler = nullptr;
    switch (medium) {
      case Medium::WEB_RTC:
        handler = fake_web_rtc_bwu_handler_;
        break;
      case Medium::WIFI_LAN:
        handler = fake_wifi_lan_bwu_handler_;
        break;
      default:
        return nullptr;
    }
    bwu_manager_->InitiateBwuForEndpoint(&client_, endpoint_id, medium);
    FakeEndpointChannel* upgraded_channel =
        handler->NotifyBwuManagerOfIncomingConnection(
            handler->handle_initialize_calls().size() - 1, bwu_manager_.get());
    ExceptionOr<OfflineFrame> last_write_frame =
        parser::FromBytes(parser::ForBwuLastWrite());
    bwu_manager_->OnIncomingFrame(last_write_frame.result(), endpoint_id,
                                  &client_, Medium::BLUETOOTH);
    ExceptionOr<OfflineFrame> safe_to_close_frame =
        parser::FromBytes(parser::ForBwuSafeToClose());
    bwu_manager_->OnIncomingFrame(safe_to_close_frame.result(), endpoint_id,
                                  &client_, Medium::BLUETOOTH);

    return upgraded_channel;
  }

  bool support_multiple_bwu_mediums_;
  ClientProxy client_;
  EndpointChannelManager ecm_;
  EndpointManager em_{&ecm_};
  // It's okay there are no actual Mediums (i.e., implementations). These won't
  // be needed if we pass in an explict medium to InitiateBwuForEndpoint.
  Mediums mediums_;
  FakeEndpointChannel* initial_channel_1_ = nullptr;
  FakeEndpointChannel* initial_channel_2_ = nullptr;
  FakeEndpointChannel* initial_channel_3_ = nullptr;
  FakeBwuHandler* fake_web_rtc_bwu_handler_ = nullptr;
  FakeBwuHandler* fake_wifi_lan_bwu_handler_ = nullptr;
  std::unique_ptr<BwuManager> bwu_manager_;
};

TEST_P(BwuManagerTest, InitiateBwu_Success) {
  // Initiate BWU, and send BANDWIDTH_UPGRADE_NEGOTIATION.UPGRADE_PATH_AVAILABLE
  // to the Responder over the initial Bluetooth channel.
  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId1, Medium::WEB_RTC);

  // The appropriate upgrade medium handler is informed of the BWU initiation.
  EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());
  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_initialize_calls().size());
  EXPECT_EQ(
      std::string(kServiceIdA) + BaseBwuHandler::GetUpgradePostfixForTesting(),
      fake_web_rtc_bwu_handler_->handle_initialize_calls()[0].service_id);
  EXPECT_EQ(
      kEndpointId1,
      fake_web_rtc_bwu_handler_->handle_initialize_calls()[0].endpoint_id);

  // Establish the incoming connection on the new medium. Verify that the
  // upgrade channel replaces the initial channel.
  std::shared_ptr<EndpointChannel> shared_initial_channel_1 =
      ecm_.GetChannelForEndpoint(kEndpointId1);
  EXPECT_EQ(initial_channel_1_, shared_initial_channel_1.get());
  FakeEndpointChannel* upgraded_channel =
      fake_web_rtc_bwu_handler_->NotifyBwuManagerOfIncomingConnection(
          /*initialize_call_index=*/0u, bwu_manager_.get());
  EXPECT_EQ(upgraded_channel, ecm_.GetChannelForEndpoint(kEndpointId1).get());

  // Confirm that upgrade channel is paused until initial channel is shut down.
  EXPECT_TRUE(upgraded_channel->IsPaused());
  EXPECT_FALSE(initial_channel_1_->is_closed());

  // Receive BANDWIDTH_UPGRADE_NEGOTIATION.LAST_WRITE_TO_PRIOR_CHANNEL and then
  // BANDWIDTH_UPGRADE_NEGOTIATION.SAFE_TO_CLOSE_PRIOR_CHANNEL from the
  // Responder device to trigger the shutdown of the initial Bluetooth channel.
  ExceptionOr<OfflineFrame> last_write_frame =
      parser::FromBytes(parser::ForBwuLastWrite());
  bwu_manager_->OnIncomingFrame(last_write_frame.result(), kEndpointId1,
                                &client_, Medium::BLUETOOTH);
  ExceptionOr<OfflineFrame> safe_to_close_frame =
      parser::FromBytes(parser::ForBwuSafeToClose());
  bwu_manager_->OnIncomingFrame(safe_to_close_frame.result(), kEndpointId1,
                                &client_, Medium::BLUETOOTH);

  // Confirm that upgrade channel is resumed after initial channel is shut down.
  // Note: If we didn't grab the shared initial channel pointer above, this
  // channel would have already been destroyed.
  auto old_channel =
      dynamic_cast<FakeEndpointChannel*>(shared_initial_channel_1.get());
  EXPECT_FALSE(upgraded_channel->IsPaused());
  EXPECT_TRUE(old_channel->is_closed());
  EXPECT_EQ(proto::connections::DisconnectionReason::UPGRADED,
            old_channel->disconnection_reason());
}

TEST_P(BwuManagerTest,
       InitiateBwu_Error_DontUpgradeIfAlreadyConenctedOverTheRequestedMedium) {
  FullyUpgradeEndpoint(kEndpointId1, Medium::WEB_RTC);
  EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());

  // Ignore request to upgrade to WebRTC if we're already connected.
  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId1, Medium::WEB_RTC);
  EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());
}

TEST_P(BwuManagerTest, InitiateBwu_Error_NoMediumHandler) {
  // Try to upgrade to a medium without a handler. Should just early return with
  // no action.
  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId1,
                                       Medium::WIFI_HOTSPOT);

  // Make sure none of the other medium handlers are called.
  EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());
  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_initialize_calls().size());
}

TEST_P(BwuManagerTest, InitiateBwu_Error_UpgradeAlreadyInProgress) {
  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId1, Medium::WEB_RTC);
  EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());

  // Try to upgrade an endpoint that already has an ungrade in progress. Should
  // just early return with no action.
  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId1,
                                       Medium::WIFI_LAN);
  EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());
  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_initialize_calls().size());
}

TEST_P(BwuManagerTest,
       InitiateBwu_Error_FailedToWriteUpgradePathAvailableFrame) {
  // Make the initial endpoint channel fail when writing the
  // UPGRADE_PATH_AVAILABLE frame.
  initial_channel_1_->set_write_output(Exception{Exception::kIo});

  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId1, Medium::WEB_RTC);

  // After we notify the WebRTC handler, we try to write the
  // UPGRADE_PATH_AVAILABLE frame, but fail by just early returning.
  EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_initialize_calls().size());

  // However, we do not record an in-progress attempt. So, if we see an incoming
  // connection over WebRTC, we ignore it. In other words, the initial BLUETOOTH
  // channel is still used.
  EXPECT_EQ(initial_channel_1_, ecm_.GetChannelForEndpoint(kEndpointId1).get());
  FakeEndpointChannel* upgraded_channel =
      fake_web_rtc_bwu_handler_->NotifyBwuManagerOfIncomingConnection(
          /*initialize_call_index=*/0u, bwu_manager_.get());
  EXPECT_NE(upgraded_channel, ecm_.GetChannelForEndpoint(kEndpointId1).get());
  EXPECT_EQ(initial_channel_1_, ecm_.GetChannelForEndpoint(kEndpointId1).get());
}

TEST_P(BwuManagerTest, InitiateBwu_Revert_OnDisconnect_MultipleEndpoints) {
  // Connect all endpoints for service A to WebRTC.
  FullyUpgradeEndpoint(kEndpointId1, Medium::WEB_RTC);   // service A
  FullyUpgradeEndpoint(kEndpointId2, Medium::WEB_RTC);   // service A

  std::string upgrade_service_id =
      std::string(kServiceIdA) + BaseBwuHandler::GetUpgradePostfixForTesting();

  EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
  EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
  {
    // Disconnect the first WebRTC endpoint. We don't expect a revert until the
    // last WebRTC endpoint for the service is disconnected.
    CountDownLatch latch(1);
    ecm_.UnregisterChannelForEndpoint(kEndpointId1);
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id,
                                       kEndpointId1, latch);
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId1,
              fake_web_rtc_bwu_handler_->disconnect_calls()[0].endpoint_id);
    EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
  }
  {
    // Disconnect the second WebRTC endpoint. We expect a revert.
    CountDownLatch latch(1);
    ecm_.UnregisterChannelForEndpoint(kEndpointId2);
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id,
                                       kEndpointId2, latch);
    EXPECT_EQ(2u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId2,
              fake_web_rtc_bwu_handler_->disconnect_calls()[1].endpoint_id);
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
    EXPECT_EQ(upgrade_service_id,
              fake_web_rtc_bwu_handler_->handle_revert_calls()[0].service_id);
  }
}
TEST_P(BwuManagerTest, InitiateBwu_Revert_OnDisconnect_MultipleServices) {
  FullyUpgradeEndpoint(kEndpointId2, Medium::WIFI_LAN);  // service A
  FullyUpgradeEndpoint(kEndpointId3, Medium::WIFI_LAN);  // service B

  std::string upgrade_service_id_A =
      std::string(kServiceIdA) + BaseBwuHandler::GetUpgradePostfixForTesting();
  std::string upgrade_service_id_B =
      std::string(kServiceIdB) + BaseBwuHandler::GetUpgradePostfixForTesting();

  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
  {
    CountDownLatch latch(1);
    // Note: EndpointId1 is also connected via BLUETOOTH still.
    EXPECT_EQ(3u, ecm_.GetConnectedEndpointsCount());
    ecm_.UnregisterChannelForEndpoint(kEndpointId2);
    EXPECT_EQ(2u, ecm_.GetConnectedEndpointsCount());
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id_A,
                                       kEndpointId2, latch);
    EXPECT_EQ(1u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId2,
              fake_wifi_lan_bwu_handler_->disconnect_calls()[0].endpoint_id);

    // With the support_multiple_bwu_mediums flag enabled, we have more granular
    // per-service tracking. So, we can revert for each service when the last
    // endpoint of that medium for the service goes down. With the flag
    // disabled, we only look at the _total_ number of connected endpoints and
    // revert all services when all endpoints are disconnected.
    if (support_multiple_bwu_mediums_) {
      EXPECT_EQ(1u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
      EXPECT_EQ(
          upgrade_service_id_A,
          fake_wifi_lan_bwu_handler_->handle_revert_calls()[0].service_id);
    } else {
      EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
    }
  }
  {
    CountDownLatch latch(1);
    ecm_.UnregisterChannelForEndpoint(kEndpointId3);
    EXPECT_EQ(1u, ecm_.GetConnectedEndpointsCount());
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id_B,
                                       kEndpointId3, latch);
    EXPECT_EQ(2u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId3,
              fake_wifi_lan_bwu_handler_->disconnect_calls()[1].endpoint_id);
    EXPECT_EQ(2u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
  }
}

TEST_P(BwuManagerTest,
       InitiateBwu_Revert_OnDisconnect_MultipleServicesAndEndpoints) {
  // Need support_multiple_bwu_mediums_ to run this test with multiple mediums.
  if (!support_multiple_bwu_mediums_) return;

  FullyUpgradeEndpoint(kEndpointId1, Medium::WEB_RTC);   // service A
  FullyUpgradeEndpoint(kEndpointId2, Medium::WIFI_LAN);  // service A
  FullyUpgradeEndpoint(kEndpointId3, Medium::WIFI_LAN);  // service B

  std::string upgrade_service_id_A =
      std::string(kServiceIdA) + BaseBwuHandler::GetUpgradePostfixForTesting();
  std::string upgrade_service_id_B =
      std::string(kServiceIdB) + BaseBwuHandler::GetUpgradePostfixForTesting();

  // Verify that the medium's BWU handler only gets notified to revert the
  // medium--for example, stop accepting connections on the socket--once every
  // endpoint of that medium for the service is disconnected.
  EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
  EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
  EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
  {
    CountDownLatch latch(1);
    ecm_.UnregisterChannelForEndpoint(kEndpointId1);
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id_A,
                                       kEndpointId1, latch);
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId1,
              fake_web_rtc_bwu_handler_->disconnect_calls()[0].endpoint_id);
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
    EXPECT_EQ(upgrade_service_id_A,
              fake_web_rtc_bwu_handler_->handle_revert_calls()[0].service_id);

    EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(0u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
  }
  {
    CountDownLatch latch(1);
    ecm_.UnregisterChannelForEndpoint(kEndpointId2);
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id_A,
                                       kEndpointId2, latch);
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());

    EXPECT_EQ(1u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId2,
              fake_wifi_lan_bwu_handler_->disconnect_calls()[0].endpoint_id);
    EXPECT_EQ(1u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
    EXPECT_EQ(upgrade_service_id_A,
              fake_wifi_lan_bwu_handler_->handle_revert_calls()[0].service_id);
  }
  {
    CountDownLatch latch(1);
    ecm_.UnregisterChannelForEndpoint(kEndpointId3);
    bwu_manager_->OnEndpointDisconnect(&client_, upgrade_service_id_B,
                                       kEndpointId3, latch);
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());

    EXPECT_EQ(2u, fake_wifi_lan_bwu_handler_->disconnect_calls().size());
    EXPECT_EQ(kEndpointId3,
              fake_wifi_lan_bwu_handler_->disconnect_calls()[1].endpoint_id);
    EXPECT_EQ(2u, fake_wifi_lan_bwu_handler_->handle_revert_calls().size());
    EXPECT_EQ(upgrade_service_id_B,
              fake_wifi_lan_bwu_handler_->handle_revert_calls()[1].service_id);
  }
}

TEST_P(BwuManagerTest, InitiateBwu_Revert_OnUpgradeFailure) {
  FullyUpgradeEndpoint(kEndpointId1, Medium::WEB_RTC);  // service A
  FullyUpgradeEndpoint(kEndpointId2, Medium::WEB_RTC);  // service A

  bwu_manager_->InitiateBwuForEndpoint(&client_, kEndpointId3, Medium::WEB_RTC);
  fake_web_rtc_bwu_handler_->NotifyBwuManagerOfIncomingConnection(
      /*initialize_call_index=*/2u, bwu_manager_.get());

  BwuHandler::UpgradePathInfo info;
  info.set_medium(BwuHandler::UpgradePathInfo::WEB_RTC);
  ExceptionOr<OfflineFrame> upgrade_failure =
      parser::FromBytes(parser::ForBwuFailure(info));
  bwu_manager_->OnIncomingFrame(upgrade_failure.result(), kEndpointId3,
                                &client_, Medium::WEB_RTC);

  if (support_multiple_bwu_mediums_) {
    EXPECT_EQ(1u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
    EXPECT_EQ(std::string(kServiceIdB) +
                  BaseBwuHandler::GetUpgradePostfixForTesting(),
              fake_web_rtc_bwu_handler_->handle_revert_calls()[0].service_id);
  } else {
    // With the flag disabled, we don't revert if there are still connected
    // endpoints.
    EXPECT_EQ(0u, fake_web_rtc_bwu_handler_->handle_revert_calls().size());
  }
}

INSTANTIATE_TEST_SUITE_P(BwuManagerTest, BwuManagerTest, testing::Bool());

}  // namespace
}  // namespace connections
}  // namespace nearby
}  // namespace location
