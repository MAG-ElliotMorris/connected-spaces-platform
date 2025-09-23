#include <emscripten/bind.h>
#include <emscripten.h>

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include <optional>

using namespace emscripten;


EMSCRIPTEN_DECLARE_VAL_TYPE(EntityCreatedCb);
EMSCRIPTEN_DECLARE_VAL_TYPE(SpaceEntityPromise)

//Needs a value constructor to work as a return, which EMSCRIPTEN_DECLARE_VAL_TYPE dosen't have
/*
struct SpaceEntityPromise : public ::emscripten::val {
  using val::val;                        
  explicit SpaceEntityPromise(val other)      
    : val(std::move(other)) {}

  explicit SpaceEntityPromise(const val& other) : val(other) {}   
};
*/


// You're probably wondering why we don't just bind directly to the member function?
// This is a tsgen limitation, as it currently cant understand std::function, maybe it will/does in a future emscripten version?
// Sidenote: There's advantages to this, it's a good place to add shims, frees us from needing to keep the exact
// same signatures between Web and C++, and can add web only logs and whatnot.
static void IRealtimeEngine_createEntity_js(
    csp::common::IRealtimeEngine& Self,
    const std::string& Name,
    const csp::multiplayer::SpaceTransform& SpaceTransform,
    const std::optional<uint64_t>& Parent,
    EntityCreatedCb jsEntityCreatedCallback)
{
    Self.CreateEntity(Name, SpaceTransform, Parent,
        [cb = jsEntityCreatedCallback](csp::multiplayer::SpaceEntity* e) {
            cb(e ? emscripten::val(e) : emscripten::val::null());
        });
}

// Use the emscripten api to essentially implement a 1:1 JS promise object
/*
static SpaceEntityPromise IRealtimeEngine_createEntity_js_async(csp::common::IRealtimeEngine& Self,
                        const std::string& Name,
                        const csp::multiplayer::SpaceTransform& SpaceTransform,
                        const std::optional<uint64_t>& Parent)
{
    // Resolve and Reject are how promises are implemented in JS
    auto executor = std::function<void(emscripten::val resolve, emscripten::val reject)>(
        [&](emscripten::val resolve, emscripten::val reject) {
            try {
                Self.CreateEntity(Name, SpaceTransform, Parent,
                    // Success path, is either null or not null ... shocker!
                    [resolve = std::move(resolve)](csp::multiplayer::SpaceEntity* e) {
                        resolve(e ? emscripten::val(e) : emscripten::val::null());
                    }
                );
            // Errors, reject promise
            } catch (const std::exception& ex) {
                reject(emscripten::val(ex.what()));
            } catch (...) {
                reject(emscripten::val("CreateEntity threw an unknown C++ exception"));
            }
        });

    // Return a javascript promise object wrapping our promise implementation. Neat!
    return SpaceEntityPromise(emscripten::val::global("Promise").new_(val(executor)));
}
*/

//This is JS, don't get scared. We just need handles to the promise control knobs.
EM_JS(EM_VAL, MakeJSPromise, (), {
  var res, rej;
  var p = new Promise(function(r, j){ res = r; rej = j; });
  // Convert the JS object to an emval handle (C type, it has to be C to use EM_JS,
  // you should immediately take C++ ownership of it as a ::val with val::take_ownership())
  return Emval.toHandle({ p: p, res: res, rej: rej });
});

static SpaceEntityPromise IRealtimeEngine_createEntity_js_async(csp::common::IRealtimeEngine& Self,
                        const std::string& Name,
                        const csp::multiplayer::SpaceTransform& SpaceTransform,
                        const std::optional<uint64_t>& Parent){

    // Direcly call JS to get a { p, res, rej } structure without needing a (potentially impossible) JS->C++ conversion.
    emscripten::val PromisePack = val::take_ownership(MakeJSPromise());
    emscripten::val Promise = PromisePack["p"];
    emscripten::val Resolve = PromisePack["res"];
    emscripten::val Reject  = PromisePack["rej"];

   try {
        Self.CreateEntity(Name, SpaceTransform, Parent,
            [Resolve = std::move(Resolve)](csp::multiplayer::SpaceEntity* e) {
                Resolve(e ? val(e) : val::null());
            }
        );
    } catch (const std::exception& ex) {
        Reject(val(ex.what()));
    } catch (...) {
        Reject(val("CreateEntity threw an unknown C++ exception"));
    }

    return SpaceEntityPromise(Promise);
}


EMSCRIPTEN_BINDINGS(csp_emscripten_bindings)
{
    //Registrations (vector and map also exist at least)
    register_optional<uint64_t>();

    /// DATA TYPES

    class_<csp::common::Vector3>("Vector3")
    .constructor<>() 
    .constructor<float, float, float>();

  
    class_<csp::common::Vector4>("Vector4")
    .constructor<>() 
    .constructor<float, float, float, float>();

    class_<csp::multiplayer::SpaceTransform>("SpaceTransform")
    .constructor<>() 
    .constructor<const csp::common::Vector3&, const csp::common::Vector4&, const csp::common::Vector3&>();
    

    /// SPACE ENTITY
    class_<csp::multiplayer::SpaceEntity>("SpaceEntity");
    /// REALTIME ENGINE

   
    //Enrich the ::val callback type with a signature
    register_type<EntityCreatedCb>("(entity: SpaceEntity | null) => void");
    register_type<SpaceEntityPromise>("Promise<SpaceEntity | null>");
   // register_type<SpaceEntityPromise>("Promise<SpaceEntity | null>");


    //Argument names in TS are defined here by the param list (undocumented feature! https://github.com/emscripten-core/emscripten/issues/22001#issuecomment-2133940668)
    class_<csp::common::IRealtimeEngine>("IRealtimeEngine")
        .function("createEntity(name, transform, parent, entityCreatedCallback)", &IRealtimeEngine_createEntity_js, allow_raw_pointers())
        .function("createEntityAsync(name, transform, parent)", &IRealtimeEngine_createEntity_js_async, allow_raw_pointers()); 
        

    //Subclass
    class_<csp::multiplayer::OfflineRealtimeEngine, base<csp::common::IRealtimeEngine>>("OfflineRealtimeEngine")
    .constructor<>();  //Will need a default constructor
    
    
}

// Anchor symbol:
extern "C" void __csp_force_link_bindings() {}
//[[maybe_unused]]
//static const void* _keep_vec3_typeinfo = &typeid(csp::common::Vector3);
