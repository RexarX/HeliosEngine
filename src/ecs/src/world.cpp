#include <pch.hpp>

#include <helios/ecs/world.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/builtin_messages.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/entity/entity.hpp>

namespace helios::ecs {

World::World() {
  AddBuiltinMessages();
}

void World::Update() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::World::Update");

  Flush();
  messages_.Update();
}

void World::Flush() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::World::Flush");

  entity_manager_.Flush(
      [this](Entity entity) { component_manager_.InitEntity(entity); });
  command_queue_.ExecuteAll(*this);
}

void World::Clear() {
  command_queue_.Clear();
  component_manager_.Clear();
  entity_manager_.Clear();
  resources_.Clear();
  messages_.Clear();
  AddBuiltinMessages();
}

void World::ClearEntities() noexcept {
  component_manager_.ClearData();
  entity_manager_.Clear();
}

Entity World::CreateEntity() {
  Entity entity = entity_manager_.Create();
  component_manager_.InitEntity(entity);
  messages_.Write(EntityAddedMsg(entity));
  return entity;
}

void World::DestroyEntity(Entity entity) {
  HELIOS_ASSERT(!entity_manager_.NeedsFlush(),
                "Flush reserved entities before destruction!");
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  component_manager_.RemoveEntity(entity);
  entity_manager_.Destroy(entity);
  messages_.Write(EntityDestroyedMsg(entity));
}

void World::TryDestroyEntity(Entity entity) {
  HELIOS_ASSERT(!entity_manager_.NeedsFlush(),
                "Flush reserved entities before destruction!");
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if (!entity_manager_.Validate(entity)) {
    return;
  }

  component_manager_.TryRemoveEntity(entity);
  entity_manager_.Destroy(entity);
  messages_.Write(EntityDestroyedMsg(entity));
}

void World::ClearComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  component_manager_.Clear(entity);
  messages_.Write(ComponentsClearedMsg{entity});
}

void World::AddBuiltinMessages() {
  AddMessage<EntityAddedMsg>();
  AddMessage<EntityDestroyedMsg>();
  AddMessage<ComponentsClearedMsg>();
}

}  // namespace helios::ecs
