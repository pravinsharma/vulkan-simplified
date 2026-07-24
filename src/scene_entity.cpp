#include "vks/scene_entity.hpp"

#include "vks/material.hpp"
#include "vks/mesh.hpp"
#include "vks/types.hpp"

#include <algorithm>
#include <cstdint>
#include <future>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace vks {

struct Entity::Impl {
    EntityId id = InvalidEntity;
    Transform transform;
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
};

struct Scene::Impl {
    std::vector<std::unique_ptr<Entity::Impl>> entities;
    std::vector<MeshComponent> meshComponents;
    std::vector<EntityId> freeIds;
    EntityId nextId = 1u;

    EntityId allocate() {
        if (!freeIds.empty()) {
            EntityId id = freeIds.back();
            freeIds.pop_back();
            return id;
        }
        return nextId++;
    }
};

template <typename... Components>
class QueryView<Components...>::Iterator::Impl {
public:
    using Elem = Entity::Impl;
    std::vector<Elem*> elements;
    size_t index = 0;

    explicit Impl(const Scene::Impl& scene) {
        for (const auto& e : scene.entities) {
            if (!e) continue;
            if (hasAll<Components...>(*e)) {
                elements.push_back(e.get());
            }
        }
    }

    bool ok() const { return index < elements.size(); }
    void advance() { ++index; }
    std::tuple<Components&...> get() const {
        return deref<0, Components...>(*elements[index]);
    }

private:
    template <size_t, typename First, typename... Rest>
    bool hasAll(Elem& e) {
        return getPtr<First>(e) != nullptr && hasAll<Rest...>(e);
    }
    template <size_t>
    bool hasAll(Elem&) { return true; }

    template <typename T>
    static T* getPtr(Elem& e) {
        if constexpr (std::is_same_v<T, Transform>) return &e.transform;
        else if constexpr (std::is_same_v<T, Mesh>) return e.mesh;
        else if constexpr (std::is_same_v<T, Material>) return e.material;
        else return nullptr;
    }

    template <size_t I, typename First, typename... Rest>
    std::tuple<Components&...> deref(Elem& e) {
        return std::tuple_cat(std::tuple<First&>(*getPtr<First>(e)), deref<I + 1, Rest...>(e));
    }
    template <size_t I>
    std::tuple<> deref(Elem&) { return {}; }
};

template <typename... Components>
bool QueryView<Components...>::Iterator::operator!=(const Iterator& other) const {
    return (pimpl ? pimpl->ok() : false) != (other.pimpl ? other.pimpl->ok() : false);
}

template <typename... Components>
void QueryView<Components...>::Iterator::operator++() {
    if (pimpl) pimpl->advance();
}

template <typename... Components>
std::tuple<Components&...> QueryView<Components...>::Iterator::operator*() const {
    return pimpl->get();
}

template <typename... Components>
QueryView<Components...>::Iterator::~Iterator() = default;

template <typename... Components>
QueryView<Components...>::Iterator::Iterator(std::unique_ptr<typename Iterator::Impl> impl)
    : pimpl(std::move(impl)) {}

template <typename... Components>
typename QueryView<Components...>::Iterator QueryView<Components...>::begin() const {
    return Iterator(std::make_unique<typename Iterator::Impl>(*static_cast<const Scene::Impl*>(scene_)));
}

template <typename... Components>
typename QueryView<Components...>::Iterator QueryView<Components...>::end() const {
    return Iterator(nullptr);
}

template <typename... Components>
QueryView<Components...>::~QueryView() = default;

template <typename... Components>
QueryView<Components...>::QueryView(class Scene* scene) : scene_(scene) {}

Scene::Scene() : pimpl(std::make_unique<Impl>()) {}
Scene::~Scene() = default;

EntityBuilder Scene::create() {
    return EntityBuilder(*this);
}

EntityBuilder::EntityBuilder(Scene& s) : scene(&s) {}

EntityId EntityBuilder::commit() {
    return static_cast<Scene*>(scene)->createEntity(*this);
}

EntityBuilder& EntityBuilder::withMesh(const Mesh& m) { mesh = &m; return *this; }
EntityBuilder& EntityBuilder::withMaterial(const Material& mat) { material = &mat; return *this; }
EntityBuilder& EntityBuilder::withTransform(const Transform& t) { transform = t; return *this; }

EntityId Scene::createEntity(const EntityBuilder& builder) {
    EntityId id = pimpl->allocate();
    auto e = std::make_unique<Entity::Impl>();
    e->id = id;
    e->transform = builder.transform;
    e->mesh = builder.mesh;
    e->material = builder.material;
    if (static_cast<size_t>(id) >= pimpl->entities.size()) {
        pimpl->entities.resize(static_cast<size_t>(id) + 1);
        pimpl->meshComponents.resize(static_cast<size_t>(id) + 1);
    }
    pimpl->entities[static_cast<size_t>(id)] = std::move(e);
    pimpl->meshComponents[static_cast<size_t>(id)] = MeshComponent{};
    return id;
}

void Scene::setMeshComponent(EntityId id, uint32_t gpuMeshIndex) {
    if (id == InvalidEntity || id >= pimpl->meshComponents.size()) return;
    pimpl->meshComponents[static_cast<size_t>(id)].gpuMeshIndex = gpuMeshIndex;
}

const Mesh& Scene::getMeshRaw(EntityId id) const {
    if (id == InvalidEntity || id >= pimpl->entities.size() || !pimpl->entities[id]) {
        throw std::runtime_error("Entity not found");
    }
    return *pimpl->entities[static_cast<size_t>(id)]->mesh;
}

Entity& Scene::get(EntityId id) {
    if (id == InvalidEntity || id >= pimpl->entities.size() || !pimpl->entities[id]) {
        throw std::runtime_error("Entity not found");
    }
    static thread_local Entity ref;
    ref.pimpl = pimpl->entities[id].get();
    return ref;
}

void Scene::destroy(EntityId id) {
    if (id == InvalidEntity || id >= pimpl->entities.size()) return;
    pimpl->entities[static_cast<size_t>(id)].reset();
    pimpl->freeIds.push_back(id);
}

template <typename... Components>
QueryView<Components...> Scene::query() {
    return QueryView<Components...>(pimpl.get());
}

namespace {

template <typename... Components>
std::vector<EntityId> collect_matching(const Scene::Impl& scene) {
    std::vector<EntityId> result;
    result.reserve(scene.entities.size());
    for (const auto& e : scene.entities) {
        if (!e) continue;
        bool match = true;
        ((match = match && has_component<Components>(*e)) && ...);
        if (match) {
            result.push_back(e->id);
        }
    }
    return result;
}

template <typename T>
bool has_component(const Entity::Impl& e) {
    if constexpr (std::is_same_v<T, Transform>) return true;
    else if constexpr (std::is_same_v<T, Mesh>) return e.mesh != nullptr;
    else if constexpr (std::is_same_v<T, Material>) return e.material != nullptr;
    else return false;
}

}

template <typename... Components>
std::future<std::vector<EntityId>> Scene::queryAsync() {
    return std::async(std::launch::async, collect_matching<Components...>, std::cref(*pimpl));
}

EntityId Entity::id() const {
    return pimpl ? pimpl->id : InvalidEntity;
}

Transform& Entity::transform() {
    return pimpl->transform;
}

const Transform& Entity::transform() const {
    return pimpl->transform;
}

const Mesh& Entity::mesh() const {
    return *pimpl->mesh;
}

const Material& Entity::material() const {
    return *pimpl->material;
}

}
