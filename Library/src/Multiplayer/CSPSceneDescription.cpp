/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/Multiplayer/CSPSceneDescription.h"
#include "CSP/Common/List.h"
#include "CSP/Systems/MCS/MCSSceneData.h"
#include "Multiplayer/MCS/MCSSceneDescription.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/SpaceEntityStatePatcher.h"
#include "Json/JsonSerializer.h"
#include <numeric>

namespace csp::multiplayer
{
CSPSceneDescription::CSPSceneDescription(const csp::common::List<csp::common::String>& SceneDescriptionJson)

{
    // Unpack the list into a single JSON string.
    // The reason this JSON is packed into a list _at all_ is merely a wrapper generator workaround,
    // csp::common::Strings cannot be passed as heap objects, and these SceneDescriptions can be large
    // enough to blow the stack
    this->SceneDescriptionJson = std::accumulate(SceneDescriptionJson.begin(), SceneDescriptionJson.end(), csp::common::String {});
}

csp::common::Array<csp::multiplayer::SpaceEntity*> CSPSceneDescription::CreateEntities(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner) const
{
    mcs::SceneDescription SceneDescription;
    csp::json::JsonDeserializer::Deserialize(SceneDescriptionJson.c_str(), SceneDescription);

    csp::common::Array<csp::multiplayer::SpaceEntity*> Entities { SceneDescription.Objects.size() };

    size_t ObjectsIndex = 0;
    for (const auto& Object : SceneDescription.Objects)
    {
        auto Entity = SpaceEntityStatePatcher::NewFromObjectMessage(Object, RealtimeEngine, RemoteScriptRunner, LogSystem);

        Entities[ObjectsIndex] = Entity.release();
        ObjectsIndex++;
    }

    return Entities;
}

csp::common::String CSPSceneDescription::SerializeEntities(const csp::common::IRealtimeEngine& RealtimeEngine)
{
    const csp::common::List<csp::multiplayer::SpaceEntity*>* AllEntities = RealtimeEngine.GetAllEntities();

    mcs::SceneDescription SceneDescription;
    SceneDescription.Objects.reserve(AllEntities->Size());

    for (size_t i = 0; i < AllEntities->Size(); ++i)
    {
        SceneDescription.Objects.push_back(SpaceEntityStatePatcher::CreateObjectMessageFromSpaceEntity(*(*AllEntities)[i]));
    }

    return csp::json::JsonSerializer::Serialize(SceneDescription);
}

}

namespace
{
struct FullCheckpointData
{
    const csp::multiplayer::mcs::SceneDescription& Description;
    const csp::systems::mcs::SceneData& Data;
};

struct FullCheckpointDataWrapper
{
    const FullCheckpointData& Checkpoint;
};
}

void ToJson(csp::json::JsonSerializer& Serializer, const FullCheckpointDataWrapper& Obj)
{
    Serializer.SerializeMember("objectMessages", Obj.Checkpoint.Description.Objects);
    ::ToJson(Serializer, Obj.Checkpoint.Data);
}

void ToJson(csp::json::JsonSerializer& Serializer, const FullCheckpointData& Obj)
{
    FullCheckpointDataWrapper Wrapper { Obj };
    Serializer.SerializeMember("data", Wrapper);
}

namespace
{
csp::multiplayer::mcs::SceneDescription BuildSceneDescriptionFromEngine(const csp::common::IRealtimeEngine& RealtimeEngine)
{
    const csp::common::List<csp::multiplayer::SpaceEntity*>* AllEntities = RealtimeEngine.GetAllEntities();

    csp::multiplayer::mcs::SceneDescription SceneDescription;
    SceneDescription.Objects.reserve(AllEntities->Size());

    for (size_t i = 0; i < AllEntities->Size(); ++i)
    {
        SceneDescription.Objects.push_back(
            csp::multiplayer::SpaceEntityStatePatcher::CreateObjectMessageFromSpaceEntity(*(*AllEntities)[i]));
    }

    return SceneDescription;
}
}

namespace csp::multiplayer
{

csp::common::String CSPSceneDescription::SerializeCheckpoint(
    const csp::common::IRealtimeEngine& RealtimeEngine, const csp::systems::mcs::SceneData& SceneData)
{
    auto SceneDescription = BuildSceneDescriptionFromEngine(RealtimeEngine);
    FullCheckpointData Checkpoint { SceneDescription, SceneData };
    return csp::json::JsonSerializer::Serialize(Checkpoint);
}

csp::common::String CSPSceneDescription::SerializeCheckpoint(const csp::common::IRealtimeEngine& RealtimeEngine) const
{
    // Parse scene data from the original checkpoint JSON
    csp::systems::mcs::SceneData SceneData;
    csp::json::JsonDeserializer::Deserialize(SceneDescriptionJson.c_str(), SceneData);

    return SerializeCheckpoint(RealtimeEngine, SceneData);
}

}
