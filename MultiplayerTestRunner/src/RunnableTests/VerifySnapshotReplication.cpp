/*
 * Copyright 2026 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VerifySnapshotReplication.h"

#include <CSP/Common/Array.h>
#include <CSP/Multiplayer/OnlineRealtimeEngine.h>
#include <CSP/Multiplayer/SpaceEntity.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace VerifySnapshotReplication
{

namespace
{
    constexpr auto FetchTimeout = std::chrono::seconds(30);
    constexpr auto TickInterval = std::chrono::milliseconds(100);
    constexpr const char* ExpectedParentName = "ReplicationParent";
    constexpr const char* ExpectedChildName = "ReplicationChild";
}

void RunTest(csp::multiplayer::OnlineRealtimeEngine& RealtimeEngine)
{
    // SpaceRAII has already waited for the initial entity fetch to complete before handing control
    // here, so replicated entities from the driver's SetSpaceState should be present. Poll a short
    // window with TickEntities so any still-in-flight patches (e.g. the post-hoc parent rewrite)
    // can land before we assert.
    const auto Start = std::chrono::steady_clock::now();
    while (RealtimeEngine.GetNumEntities() < 2)
    {
        RealtimeEngine.TickEntities();
        if (std::chrono::steady_clock::now() - Start > FetchTimeout)
        {
            break;
        }
        std::this_thread::sleep_for(TickInterval);
    }
    RealtimeEngine.TickEntities();

    const size_t Count = RealtimeEngine.GetNumEntities();
    if (Count != 2)
    {
        throw std::runtime_error(
            "VerifySnapshotReplication: expected 2 replicated entities in the space, observed " + std::to_string(Count) + ".");
    }

    csp::multiplayer::SpaceEntity* Parent = RealtimeEngine.FindSpaceEntity(ExpectedParentName);
    csp::multiplayer::SpaceEntity* Child = RealtimeEngine.FindSpaceEntity(ExpectedChildName);
    if (Parent == nullptr)
    {
        throw std::runtime_error("VerifySnapshotReplication: entity 'ReplicationParent' not found in space.");
    }
    if (Child == nullptr)
    {
        throw std::runtime_error("VerifySnapshotReplication: entity 'ReplicationChild' not found in space.");
    }

    if (Child->GetParentEntity() != Parent)
    {
        throw std::runtime_error("VerifySnapshotReplication: 'ReplicationChild' is not parented to 'ReplicationParent' on the server.");
    }

    const auto* Children = Parent->GetChildEntities();
    if (Children == nullptr || Children->Size() != 1 || (*Children)[0] != Child)
    {
        throw std::runtime_error("VerifySnapshotReplication: parent's child list does not contain exactly 'ReplicationChild'.");
    }
}

} // namespace VerifySnapshotReplication
