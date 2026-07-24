#pragma once

#include <cstdint>
#include <future>
#include <span>
#include <vector>

#include "vks/material.hpp"
#include "vks/mesh.hpp"
#include "vks/types.hpp"

namespace vks {

struct MeshComponent {
    uint32_t gpuMeshIndex = UINT32_MAX;
};

struct Transform {
    Vec3 position = {0, 0, 0};
    Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 scale = {1, 1, 1};
};

using EntityId = uint32_t;
static constexpr EntityId InvalidEntity = 0;

class Scene;

class Entity {
public:
    Entity() = default;
    ~Entity() = default;

    EntityId id() const;

    Transform& transform();
    const Transform& transform() const;

    const Mesh& mesh() const;
    const Material& material() const;

private:
    friend class Scene;
    struct Impl;
    Impl* pimpl = nullptr;
};

class EntityBuilder {
public:
    EntityBuilder& withMesh(const Mesh& mesh);
    EntityBuilder& withMaterial(const Material& material);
    EntityBuilder& withTransform(const Transform& transform);
    EntityId commit();

private:
    friend class Scene;
    explicit EntityBuilder(Scene& scene);
    Scene* scene = nullptr;
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    Transform transform;
};

template <typename... Components>
class QueryView {
public:
    class Iterator {
    public:
        bool operator!=(const Iterator& other) const;
        void operator++();
        std::tuple<Components&...> operator*() const;
        ~Iterator();
    private:
        friend class QueryView;
        class Impl;
        std::unique_ptr<Impl> pimpl;
        explicit Iterator(std::unique_ptr<Impl> impl);
    };

    ~QueryView();
    explicit QueryView(class Scene* scene);
    Iterator begin() const;
    Iterator end() const;

private:
    class Scene* scene_ = nullptr;
};

class Scene {
public:
    Scene();
    ~Scene();

    EntityBuilder create();
    Entity& get(EntityId id);
    void destroy(EntityId id);
    EntityId createEntity(const EntityBuilder& builder);

    void setMeshComponent(EntityId id, uint32_t gpuMeshIndex);
    const Mesh& getMeshRaw(EntityId id) const;

    template <typename... Components>
    QueryView<Components...> query();

    template <typename... Components>
    std::future<std::vector<EntityId>> queryAsync();

private:
    friend class EntityBuilder;
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
