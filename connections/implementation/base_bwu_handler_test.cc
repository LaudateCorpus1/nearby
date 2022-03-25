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

#include "connections/implementation/base_bwu_handler.h"

#include "gtest/gtest.h"

namespace location {
namespace nearby {
namespace connections {
namespace {

class BaseBwuHandlerTest : public ::testing::Test {
 protected:
  class BwuHandlerImpl : public BaseBwuHandler {
   public:
    using Medium = location::nearby::proto::connections::Medium;

    // The arguments passed to the BwuHandler methods. Not all values are set
    // for every method.
    struct InputData {
      ClientProxy* client = nullptr;
      absl::optional<std::string> service_id;
      absl::optional<std::string> endpoint_id;
    };

    BwuHandlerImpl() : BaseBwuHandler(BwuNotifications{}) {}
    ~BwuHandlerImpl() override = default;

    const std::vector<InputData>& handle_initialize_calls() const {
      return handle_initialize_calls_;
    }
    const std::vector<InputData>& handle_revert_calls() const {
      return handle_revert_calls_;
    }
    void set_handle_initialize_output(ByteArray bytes) {
      handle_initialize_output_ = bytes;
    }

   private:
    // BwuHandler:
    std::unique_ptr<EndpointChannel> CreateUpgradedEndpointChannel(
        ClientProxy* client, const std::string& service_id,
        const std::string& endpoint_id,
        const UpgradePathInfo& upgrade_path_info) final {
      return nullptr;
    }
    Medium GetUpgradeMedium() const final { return Medium::UNKNOWN_MEDIUM; }
    void OnEndpointDisconnect(ClientProxy* client,
                              const std::string& endpoint_id) final {}

    // BaseBwuHandler:
    ByteArray HandleInitializeUpgradedMediumForEndpoint(
        ClientProxy* client, const std::string& upgrade_service_id,
        const std::string& endpoint_id) final {
      handle_initialize_calls_.push_back({.client = client,
                                          .service_id = upgrade_service_id,
                                          .endpoint_id = endpoint_id});
      return handle_initialize_output_;
    }
    void HandleRevertInitiatorStateForService(
        const std::string& upgrade_service_id) final {
      handle_revert_calls_.push_back({.service_id = upgrade_service_id});
    }

    ByteArray handle_initialize_output_;
    std::vector<InputData> handle_initialize_calls_;
    std::vector<InputData> handle_revert_calls_;
  };

  BaseBwuHandlerTest() = default;
  ~BaseBwuHandlerTest() override = default;

  ClientProxy client_;
  BwuHandlerImpl handler_;
};

TEST_F(BaseBwuHandlerTest, InitializeAndRevert) {
  ByteArray expected_output{"not empty"};
  handler_.set_handle_initialize_output(expected_output);

  // Initialize two upgrade endpoints for service A and one for service B.
  // Notably, verify that the service ID is given a suffix.
  EXPECT_EQ(expected_output, handler_.InitializeUpgradedMediumForEndpoint(
                                 &client_, /*service_id=*/"A",
                                 /*endpoint_id=*/"1"));
  EXPECT_EQ(1u, handler_.handle_initialize_calls().size());
  EXPECT_EQ(&client_, handler_.handle_initialize_calls()[0].client);
  EXPECT_EQ(std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
            handler_.handle_initialize_calls()[0].service_id);
  EXPECT_EQ("1", handler_.handle_initialize_calls()[0].endpoint_id);

  EXPECT_EQ(expected_output, handler_.InitializeUpgradedMediumForEndpoint(
                                 &client_, /*service_id=*/"A",
                                 /*endpoint_id=*/"2"));
  EXPECT_EQ(2u, handler_.handle_initialize_calls().size());
  EXPECT_EQ(&client_, handler_.handle_initialize_calls()[1].client);
  EXPECT_EQ(std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
            handler_.handle_initialize_calls()[1].service_id);
  EXPECT_EQ("2", handler_.handle_initialize_calls()[1].endpoint_id);

  EXPECT_EQ(expected_output, handler_.InitializeUpgradedMediumForEndpoint(
                                 &client_, /*service_id=*/"B",
                                 /*endpoint_id=*/"1"));
  EXPECT_EQ(3u, handler_.handle_initialize_calls().size());
  EXPECT_EQ(&client_, handler_.handle_initialize_calls()[2].client);
  EXPECT_EQ(std::string("B") + BaseBwuHandler::GetUpgradePostfixForTesting(),
            handler_.handle_initialize_calls()[2].service_id);
  EXPECT_EQ("1", handler_.handle_initialize_calls()[2].endpoint_id);

  // Revert all endpoints, and verify that the child class is only notified of
  // the revert for the last endpoint corresponding to the service.
  handler_.RevertInitiatorState(
      std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"1");
  EXPECT_EQ(0u, handler_.handle_revert_calls().size());

  handler_.RevertInitiatorState(
      std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"2");
  EXPECT_EQ(1u, handler_.handle_revert_calls().size());
  EXPECT_EQ(std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
            handler_.handle_revert_calls()[0].service_id);

  handler_.RevertInitiatorState(
      std::string("B") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"1");
  EXPECT_EQ(2u, handler_.handle_revert_calls().size());
  EXPECT_EQ(std::string("B") + BaseBwuHandler::GetUpgradePostfixForTesting(),
            handler_.handle_revert_calls()[1].service_id);
}

TEST_F(BaseBwuHandlerTest, InitializeAndRevertAll) {
  ByteArray expected_output{"not empty"};
  handler_.set_handle_initialize_output(expected_output);

  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"A",
                                               /*endpoint_id=*/"1");
  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"A",
                                               /*endpoint_id=*/"2");
  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"B",
                                               /*endpoint_id=*/"1");

  // Call the function that reverts all services at once.
  handler_.RevertInitiatorState();
  EXPECT_EQ(2u, handler_.handle_revert_calls().size());
}

TEST_F(BaseBwuHandlerTest, Initialize_Failure_EmptyUpgradePathAvailableFrame) {
  ByteArray expected_output{};
  handler_.set_handle_initialize_output(expected_output);

  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"A",
                                               /*endpoint_id=*/"1");
  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"A",
                                               /*endpoint_id=*/"2");
  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"B",
                                               /*endpoint_id=*/"1");
  EXPECT_EQ(3u, handler_.handle_initialize_calls().size());

  // Call the function that reverts everything at once. But, there should be no
  // registered endpoints because the UPGRADE_PATH_AVAILABLE frames were empty.
  handler_.RevertInitiatorState();
  EXPECT_EQ(0u, handler_.handle_revert_calls().size());
}

TEST_F(BaseBwuHandlerTest, Initialize_StillWorkWithUpgradeServiceIdSuffix) {
  ByteArray expected_output{"not empty"};
  handler_.set_handle_initialize_output(expected_output);

  // The method should be robust and not add _another_ upgrade suffix
  handler_.InitializeUpgradedMediumForEndpoint(
      &client_, /*service_id=*/
      std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"1");
  handler_.RevertInitiatorState(
      std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"1");
  EXPECT_EQ(1u, handler_.handle_revert_calls().size());
}

TEST_F(BaseBwuHandlerTest, Revert_Failure_CantFindService) {
  ByteArray expected_output{"not empty"};
  handler_.set_handle_initialize_output(expected_output);
  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"A",
                                               /*endpoint_id=*/"1");

  handler_.RevertInitiatorState(
      std::string("B") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"1");
  EXPECT_EQ(0u, handler_.handle_revert_calls().size());
}

TEST_F(BaseBwuHandlerTest, Revert_Failure_CantFindEndpoint) {
  ByteArray expected_output{"not empty"};
  handler_.set_handle_initialize_output(expected_output);
  handler_.InitializeUpgradedMediumForEndpoint(&client_, /*service_id=*/"A",
                                               /*endpoint_id=*/"1");

  handler_.RevertInitiatorState(
      std::string("A") + BaseBwuHandler::GetUpgradePostfixForTesting(),
      /*endpoint_id=*/"2");
  EXPECT_EQ(0u, handler_.handle_revert_calls().size());
}

}  // namespace
}  // namespace connections
}  // namespace nearby
}  // namespace location
