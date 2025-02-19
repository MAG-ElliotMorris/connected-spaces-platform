#pragma once

#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ReplicatedValue.h"
#include "Olympus/OlympusCommon.h"

#include <functional>

OLY_START_IGNORE
#ifdef OLY_TESTS
class OlympusEngine_SerialisationTests_SpaceEntityUserSignalRSerialisationTest_Test;
class OlympusEngine_SerialisationTests_SpaceEntityUserSignalRDeserialisationTest_Test;
class OlympusEngine_SerialisationTests_SpaceEntityObjectSignalRSerialisationTest_Test;
class OlympusEngine_SerialisationTests_SpaceEntityObjectSignalRDeserialisationTest_Test;
#endif
OLY_END_IGNORE

namespace oly_multiplayer
{

class SpaceEntity;
class ComponentScriptInterface;

/**
 * @brief Represents the type of component.
 */
enum class ComponentType
{
    Invalid,
    Core,
    UIController_DEPRECATED,
    StaticModel,
    AnimatedModel,
    MediaSurface_DEPRECATED,
    VideoPlayer,
    ImageSequencer_DEPRECATED,
    ExternalLink,
    AvatarData,
    Light,
    Button,
    Image,
    ScriptData,
    Custom,
    Conversation,
    Portal,
    Audio,
    Spline,
    Collision,
    Reflection,
    Fog
};

/**
 * @brief The base class for all components, provides mechanisms for dirtying properties and subscribing to events on property changes.
 */
class OLY_API ComponentBase
{
    OLY_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceEntity;
    friend class EntityScriptInterface;
#ifdef OLY_TESTS
    friend class ::OlympusEngine_SerialisationTests_SpaceEntityUserSignalRSerialisationTest_Test;
    friend class ::OlympusEngine_SerialisationTests_SpaceEntityUserSignalRDeserialisationTest_Test;
    friend class ::OlympusEngine_SerialisationTests_SpaceEntityObjectSignalRSerialisationTest_Test;
    friend class ::OlympusEngine_SerialisationTests_SpaceEntityObjectSignalRDeserialisationTest_Test;
#endif
    /** @endcond */
    OLY_END_IGNORE

public:
    typedef std::function<void(ComponentBase*, const oly_common::String&, const oly_common::String&)> EntityActionHandler;

    virtual ~ComponentBase();

    /**
     * @brief Get the Id for this component. This is set when calling SpaceEntity::AddComponent
     * and is autogenerated with the intention of being unique to its instance.
     * @return The Id.
     */
    uint16_t GetId();
    /**
     * @brief Get the ComponentType of the component.
     * @return The type of the component as an enum.
     */
    ComponentType GetComponentType();

    /**
     * @brief Get a map of the replicated values defined for this component.
     * The index of the map represents a unique index for the property,
     * intended to be defined in the inherited component as an enum of available properties keys.
     * @return A map of the replicated values, keyed by their unique key.
     */
    const oly_common::Map<uint32_t, ReplicatedValue>* GetProperties();

    /**
     * @brief Get the parent SpaceEntity for this component. Components can only attach to one parent.
     * @return A pointer to the parent SpaceEntity.
     */
    SpaceEntity* GetParent();

    /**
     * @brief Part of the scripting interface, allows you to subscribe to a property change and assign a script message to execute when activated.
     */
    OLY_NO_EXPORT void SubscribeToPropertyChange(uint32_t PropertyKey, oly_common::String Message);

    // TODO: Add documentation comment
    void RegisterActionHandler(const oly_common::String& InAction, EntityActionHandler ActionHandler);
    // TODO: Add documentation comment
    void UnregisterActionHandler(const oly_common::String& InAction);

    // TODO: Add documentation comment
    void InvokeAction(const oly_common::String& InAction, const oly_common::String& InActionParams);

protected:
    ComponentBase();
    ComponentBase(ComponentType Type, SpaceEntity* Parent);

    const ReplicatedValue& GetProperty(uint32_t Key) const;
    void SetProperty(uint32_t Key, const ReplicatedValue& Value);
    void RemoveProperty(uint32_t Key);
    void SetProperties(const oly_common::Map<uint32_t, ReplicatedValue>& Value);

    virtual void SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value);

    virtual void OnRemove();

    OLY_START_IGNORE
    void SetScriptInterface(ComponentScriptInterface* ScriptInterface);
    ComponentScriptInterface* GetScriptInterface();
    OLY_END_IGNORE

    SpaceEntity* Parent;
    uint16_t Id;
    ComponentType Type;
    oly_common::Map<uint32_t, ReplicatedValue> Properties;
    oly_common::Map<uint32_t, ReplicatedValue> DirtyProperties;

    ComponentScriptInterface* ScriptInterface;

    oly_common::Map<oly_common::String, EntityActionHandler> ActionMap;
};

} // namespace oly_multiplayer
