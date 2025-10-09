#include <emscripten.h>
#include <emscripten/bind.h>

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include <optional>
#include <unordered_map>

using namespace emscripten;

EMSCRIPTEN_DECLARE_VAL_TYPE(EntityCreatedCb);
EMSCRIPTEN_DECLARE_VAL_TYPE(SpaceEntityPromise)

// Needs a value constructor to work as a return, which EMSCRIPTEN_DECLARE_VAL_TYPE dosen't have
/*
struct SpaceEntityPromise : public ::emscripten::val {
  using val::val;
  explicit SpaceEntityPromise(val other)
    : val(std::move(other)) {}

  explicit SpaceEntityPromise(const val& other) : val(other) {}
};
*/

class EntityCreatedCallbackJSShim : public csp::multiplayer::EntityCreatedCallback
{
public:
    EntityCreatedCallbackJSShim(EntityCreatedCb jsCB)
        : m_jsCallback(jsCB)
    {
    }
    void Call(csp::multiplayer::SpaceEntity* e) override { m_jsCallback(emscripten::val(e)); }

private:
    EntityCreatedCb m_jsCallback;
};

class EntityCreatedPromiseJSShim : public csp::multiplayer::EntityCreatedCallback
{
public:
    EntityCreatedPromiseJSShim(emscripten::val Resolve, emscripten::val Reject)
        : m_Resolve(Resolve)
        , m_Reject(Reject)
    {
    }
    void Call(csp::multiplayer::SpaceEntity* e) override { m_Resolve(emscripten::val(e)); }

private:
    emscripten::val m_Resolve;
    emscripten::val m_Reject;
};

// This is just to prove that we can keep the callbacks alive. You could use shared_ptrs instead but it'd be a little more intricate.
// Obviously cleaning these up will be a problem, honestly just cleaning up one-shot CBs on point of call, and having a "deregisterCB"
// for long-lived ones is probably the best. Simple, cavemany. No-one really cares to deregister long lived callbacks anyhow.
static std::unordered_map<uint64_t, EntityCreatedCallbackJSShim> Callbacks;
static std::unordered_map<uint64_t, EntityCreatedPromiseJSShim> AsyncCallbacks; // Wonder if these could be the same buffer?

// You're probably wondering why we don't just bind directly to the member function?
// This is a tsgen limitation, as it currently cant understand std::function, maybe it will/does in a future emscripten version?
// Sidenote: There's advantages to this, it's a good place to add shims, frees us from needing to keep the exact
// same signatures between Web and C++, and can add web only logs and whatnot.
static void IRealtimeEngine_createEntity_js(csp::common::IRealtimeEngine& Self, const std::string& Name,
    const csp::multiplayer::SpaceTransform& SpaceTransform, const std::optional<uint64_t>& Parent, EntityCreatedCb jsEntityCreatedCallback)
{
    static uint64_t CBId = 0;
    CBId++;
    Callbacks.emplace(CBId, EntityCreatedCallbackJSShim { jsEntityCreatedCallback });

    auto& cb = Callbacks.at(CBId);
    Self.CreateEntity(Name, SpaceTransform, Parent, cb);
}

// This is JS, don't get scared. We just need handles to the promise control knobs.
EM_JS(EM_VAL, MakeJSPromise, (), {
    var res, rej;
    var p = new Promise(function(r, j) {
        res = r;
        rej = j;
    });
    // Convert the JS object to an emval handle (C type, it has to be C to use EM_JS,
    // you should immediately take C++ ownership of it as a ::val with val::take_ownership())
    return Emval.toHandle({ p : p, res : res, rej : rej });
});

static SpaceEntityPromise IRealtimeEngine_createEntity_js_async(csp::common::IRealtimeEngine& Self, const std::string& Name,
    const csp::multiplayer::SpaceTransform& SpaceTransform, const std::optional<uint64_t>& Parent)
{

    // Direcly call JS to get a { p, res, rej } structure without needing a (potentially impossible) JS->C++ conversion.
    emscripten::val PromisePack = val::take_ownership(MakeJSPromise());
    emscripten::val Promise = PromisePack["p"];
    emscripten::val Resolve = PromisePack["res"];
    emscripten::val Reject = PromisePack["rej"];

    static uint64_t CBId = 0;
    CBId++;
    AsyncCallbacks.emplace(CBId, EntityCreatedPromiseJSShim { Resolve, Reject });
    auto& cb = Callbacks.at(CBId);

    Self.CreateEntity(Name, SpaceTransform, Parent, cb);

    return SpaceEntityPromise(Promise);
}

/*
static SpaceEntityPromise IRealtimeEngine_createEntity_js_async(csp::common::IRealtimeEngine& Self, const std::string& Name,
    const csp::multiplayer::SpaceTransform& SpaceTransform, const std::optional<uint64_t>& Parent)
{

    // Direcly call JS to get a { p, res, rej } structure without needing a (potentially impossible) JS->C++ conversion.
    emscripten::val PromisePack = val::take_ownership(MakeJSPromise());
    emscripten::val Promise = PromisePack["p"];
    emscripten::val Resolve = PromisePack["res"];
    emscripten::val Reject = PromisePack["rej"];

    try
    {
        Self.CreateEntity(
            Name, SpaceTransform, Parent, [Resolve = std::move(Resolve)](csp::multiplayer::SpaceEntity* e) { Resolve(e ? val(e) : val::null()); });
    }
    catch (const std::exception& ex)
    {
        Reject(val(ex.what()));
    }
    catch (...)
    {
        Reject(val("CreateEntity threw an unknown C++ exception"));
    }

    return SpaceEntityPromise(Promise);
}
    */

EMSCRIPTEN_BINDINGS(csp_emscripten_bindings)
{
    // Registrations (vector and map also exist at least)
    register_optional<uint64_t>();

    /// DATA TYPES

    class_<csp::common::Vector3>("Vector3").constructor<>().constructor<float, float, float>();

    class_<csp::common::Vector4>("Vector4").constructor<>().constructor<float, float, float, float>();

    class_<csp::multiplayer::SpaceTransform>("SpaceTransform")
        .constructor<>()
        .constructor<const csp::common::Vector3&, const csp::common::Vector4&, const csp::common::Vector3&>();

    /// SPACE ENTITY
    class_<csp::multiplayer::SpaceEntity>("SpaceEntity");
    /// REALTIME ENGINE

    // Enrich the ::val callback type with a signature
    register_type<EntityCreatedCb>("(entity: SpaceEntity | null) => void");
    register_type<SpaceEntityPromise>("Promise<SpaceEntity | null>");
    // register_type<SpaceEntityPromise>("Promise<SpaceEntity | null>");

    // Argument names in TS are defined here by the param list (undocumented feature!
    // https://github.com/emscripten-core/emscripten/issues/22001#issuecomment-2133940668)
    class_<csp::common::IRealtimeEngine>("IRealtimeEngine")
        .function("createEntity(name, transform, parent, entityCreatedCallback)", &IRealtimeEngine_createEntity_js, allow_raw_pointers())
        .function("createEntityAsync(name, transform, parent)", &IRealtimeEngine_createEntity_js_async, allow_raw_pointers());

    // Subclass
    class_<csp::multiplayer::OfflineRealtimeEngine, base<csp::common::IRealtimeEngine>>("OfflineRealtimeEngine")
        .constructor<>(); // Will need a default constructor
}

// Anchor symbol:
extern "C" void __csp_force_link_bindings() { }
//[[maybe_unused]]
// static const void* _keep_vec3_typeinfo = &typeid(csp::common::Vector3);
