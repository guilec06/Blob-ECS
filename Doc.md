# Entity Component System documentation
Made by Guillaume LECOCQ

*For detailed method behaviour, the in-code comments remain the reference. Some recently added methods (tags, metadata, priorities) are not yet documented there.*

## Table of Contents
- [Structure](#structure)
- [The ECS](#the-ecs)
- [Components](#components)
- [Tags](#tags)
- [Entities](#entities)
- [Systems](#systems)
- [Debug checks and limits](#debug-checks-and-limits)

## Structure

This ECS revolves around a virtual representation of entities: an entity is only an ID.
The main class is the `ECS` class and is the one to be manipulated by the user.
Component pools, the registry and the system interface are mostly used through the `ECS` class.

The library is header-only and needs C++20 (concepts). Everything lives in the `ECS` namespace.

Types, constants and classes the user can freely manipulate:
```cpp
using EntityID = std::uint32_t;   // This is how entities are represented
using SysPriority = std::uint8_t; // Lower value = system is updated earlier

constexpr EntityID NULL_ENTITY = UINT32_MAX;
constexpr SysPriority SYSTEM_PRIORITY_DEFAULT = 10;

// Concept defining Systems classes, they must derive from ISystem
template<typename T>
concept SystemClass = std::is_base_of_v<ISystem, T>;

// Concept for empty types, these are stored as tags (see "Tags")
template<typename T>
concept TagType = std::is_empty_v<T>;

// A default-constructible type. Defined but not enforced by the ECS templates: any type that can
// be constructed can be a component (see "Components")
template<typename T>
concept ComponentType = std::is_default_constructible_v<T>;

class ECS;     // This is the core class of the ECS, this is what the user is going to use
class ISystem; // System Interface, used to create systems
```
Beside that, there are also types and classes that the ECS uses in the background:
```cpp
using EntityComponentMask = std::bitset<128>; // One bit per registered component type, per entity

struct EntityMetadata {           // Struct of arrays, indexed by EntityID
    std::vector<std::uint16_t> generation;
    std::vector<EntityComponentMask> component_mask;
    std::vector<bool> alive;
};

struct SystemEntry { ISystem *sys; SysPriority priority; }; // An enabled system

class Registry;                          // Owns one pool per registered component type
class IComponentPool;                    // ComponentPool interface

template<typename T, bool isTag = std::is_empty_v<T>>
class ComponentPool : public IComponentPool; // Sparse set storage (or a bit set for tags)
```

## The ECS

*For detailed method documentation please refer to the in-code documentation*

The ECS class has the following methods. Query methods take an output buffer, which they clear and then fill.

| Template types | Return type | Method | Parameters | Description |
|----------------|-------------|--------|------------|-------------|
|                | `std::size_t`  | `currentEntityCount()` |                        | Returns the current active entity count |
|                | `bool`         | `entityIsActive`       | `EntityID`             | Returns if the entity is active |
|                | `EntityID`     | `entityCreate()`       |                        | Creates an entity (recycling a freed ID if any) and returns its ID |
|                | `void`         | `entityDelete`         | `EntityID`             | Deletes the entity and every component attached to it |
|                | `std::uint16_t`| `entityGetMetaGeneration` | `EntityID`          | Returns how many times this ID has been recycled |
|                | `EntityComponentMask &` | `entityGetMetaComponentMask` | `EntityID` | Returns the bit mask of components attached to the entity |
| `Components...`| `void` | `getEntitiesByComponentsAllOf` | `std::vector<EntityID> &` | Fills the buffer with entities that have ALL specified components |
| `Components...`| `void` | `getEntitiesByComponentsAnyOf` | `std::vector<EntityID> &` | Fills the buffer with entities that have ANY specified component |
| `Tags...`      | `void` | `getEntitiesByTagsAllOf` | `std::vector<EntityID> &` | Same as `AllOf`, for empty types only |
| `Tags...`      | `void` | `getEntitiesByTagsAnyOf` | `std::vector<EntityID> &` | Same as `AnyOf`, for empty types only |
|                | `void` | `entityGetAllComponents` | `std::unordered_map<uint16_t, void *> &`, `EntityID` | Fills the map with a pointer to each non-tag component of the entity, keyed by component type ID |
| `T`            | `void` | `registerComponent`    |                        | Registers a component type to the ECS |
| `T`            | `bool` | `componentExists`      |                        | Checks if a component is registered |
| `T`            | `bool` | `entityHasComponent`   | `EntityID`             | Checks if an entity has a component attached |
| `T`            | `T &`  | `entityAddComponent`   | `EntityID`, `Args&&...`| Adds a component to an entity, constructing it from `Args`, and returns a reference to it. Throws `ComponentAlreadyAttached` if it is already attached |
| `T` (non-empty)| `T &` / `const T &` | `entityGetComponent` | `EntityID`     | Returns a reference to the component attached to the entity |
| `T`            | `void` | `entityRemoveComponent`| `EntityID`             | Removes the component attached to the entity |
| `T` (empty)    | `bool` | `entityHasTag`         | `EntityID`             | Checks if the entity has the tag |
| `T` (empty)    | `void` | `entityAddTag`         | `EntityID`             | Adds the tag to the entity |
| `T` (empty)    | `void` | `entityRemoveTag`      | `EntityID`             | Removes the tag from the entity |
| `T`            | `ComponentPool<T> &` | `getPool` |                    | Returns the ComponentPool of said component |
| `SystemClass T, Args...` | `T &` | `addSystem`  | `Args&&...`            | Constructs a system from `Args`, adds it enabled with the default priority, and returns a reference to it |
| `SystemClass T`| `void` | `toggleSystem`         |                        | Toggles the system on or off |
| `SystemClass T`| `bool` | `systemIsEnabled`      |                        | Checks if the system is enabled |
| `SystemClass T`| `void` | `systemSetPriority`    | `SysPriority`          | Sets the system's priority (lower runs first) |
| `SystemClass T`| `T &`  | `getSystem`            |                        | Returns the system instance |
|                | `void` | `Update`               | `uint32_t msecs = 0`   | Calls `Update()` on every enabled system, ordered by priority |

The `Registry` is a public member of the ECS (`ecs.registry`).

## Components

Components are probably the most important aspect of an ECS. In this ecs, a component can be anything (a class, a struct, a typedef etc...). Non-empty types are stored as data, empty types are stored as [tags](#tags).
```cpp
struct comp_a { int value; };
class comp_b {
    public:
        comp_b(int v) : m_v(v) {}
    private:
        int m_v;
};
typedef std::size_t comp_c;
using comp_d = std::string;
```
Before using any component when the program runs, you have to register them to your ECS instance:
```cpp
ECS::ECS ecs;
ecs.registerComponent<comp_a>();
ecs.registerComponent<comp_b>();
ecs.registerComponent<comp_c>();
ecs.registerComponent<comp_d>();
```
Using an unregistered component is a programming error, see [Debug checks and limits](#debug-checks-and-limits).

Components are used only by using their types, as a result, any entity can only have a single component of each type. Components and Entities are mapped together inside the code, a single ID (EntityID) can get you access to every component attached to that entity.

`entityAddComponent` forwards its arguments to the component's constructor and returns a reference to the new component:
```cpp
ecs.entityAddComponent<comp_a>(entity);                       // default construction, needs a default constructor
ecs.entityAddComponent<comp_b>(entity, 42);                   // constructed from arguments
ecs.entityAddComponent<comp_d>(entity, "hello");
```

Components are stored in a ComponentPool of their own type. It is not to the user to access such pool directly, however if you have studied the code structure deeply enough, you are still given the option to get the raw pool:
```cpp
ComponentPool<comp_a> &comp_pool = ecs.getPool<comp_a>();

// Dense storage, as a struct of arrays (components[i] belongs to entities[i])
DenseComponent<comp_a> &data = comp_pool.getData();
std::vector<comp_a> &values = data.components;
std::vector<ECS::EntityID> &owners = data.entities;

// Every entity that has this component
const std::vector<ECS::EntityID> &entities = comp_pool.getActiveEntities();
```
#### Be careful ! Removing a component moves the last component of the pool into the freed slot, references and pool positions are not stable !

## Tags

A tag is an empty type (`std::is_empty_v<T>`). Tags carry no data, so their pool is a simple bit set. Tags are registered like components but are manipulated with their own methods:
```cpp
struct Enemy {};
struct Frozen {};

ecs.registerComponent<Enemy>();
ecs.registerComponent<Frozen>();

ecs.entityAddTag<Enemy>(entity);
bool is_enemy = ecs.entityHasTag<Enemy>(entity);
ecs.entityRemoveTag<Enemy>(entity);

std::vector<ECS::EntityID> frozen_enemies;
ecs.getEntitiesByTagsAllOf<Enemy, Frozen>(frozen_enemies);
```
Tags count towards the [128 component types limit](#debug-checks-and-limits). `entityGetComponent` cannot be used on a tag.

## Entities

Entities are represented by an ID, it is referring to a single entity, however if that entity is destroyed and new entities are created, beware that this ID might now refer to a new, different entity.

IDs are recycled throughout the execution (the most recently freed ID is reused first), limiting the growth of IDs. Each time an ID is recycled its generation is incremented, you can read it with `entityGetMetaGeneration()` to detect a stale ID.

### Using components

You can create an entity and get its ID by calling the following method of the ECS class:
```cpp
ECS::ECS ecs;
ECS::EntityID entity = ecs.entityCreate();
```
Your `entity` now exists within the program as a simple ID. You can manipulate that `entity` by adding or requesting `components`
```cpp
using exampleComponent = std::size_t;

// Remember to register a component before using it
ecs.registerComponent<exampleComponent>();

// the entityAddComponent() method returns a reference to the newly created component
exampleComponent &comp = ecs.entityAddComponent<exampleComponent>(entity);
ecs.entityRemoveComponent<exampleComponent>(entity); // Removes the component from the entity
```
Using multiple components:
```cpp
using compA = std::size_t;
struct compB { int v; };

ECS::ECS ecs;
ecs.registerComponent<compA>();
ecs.registerComponent<compB>();

ECS::EntityID e_1 = ecs.entityCreate();
ECS::EntityID e_2 = ecs.entityCreate();
ECS::EntityID e_3 = ecs.entityCreate();
ECS::EntityID e_4 = ecs.entityCreate();

ecs.entityAddComponent<compA>(e_1);
ecs.entityAddComponent<compB>(e_1);

ecs.entityAddComponent<compA>(e_2);
ecs.entityAddComponent<compA>(e_3);
ecs.entityAddComponent<compA>(e_4);

// Query results are written into a buffer that you own and can reuse between calls
std::vector<ECS::EntityID> buff;

ecs.getEntitiesByComponentsAllOf<compA, compB>(buff); // [e_1]
ecs.getEntitiesByComponentsAllOf<compA>(buff);        // [e_1, e_2, e_3, e_4]
ecs.getEntitiesByComponentsAnyOf<compA, compB>(buff); // [e_1, e_2, e_3, e_4]
```
The order of the entities in the buffer is unspecified, do not rely on it. `AllOf` starts from the smallest pool among the queried components, and tests the remaining components with the entity's bit mask.

You can also check if an entity has a component:
```cpp
bool has_comp = ecs.entityHasComponent<compA>(entity);
```

Deleting an entity removes every component and tag attached to it:
```cpp
ecs.entityDelete(entity);
bool active = ecs.entityIsActive(entity); // false
```

## Systems

Using systems is pretty simple, you can create your custom systems by deriving the ISystem interface:

```cpp
class MySystem : public ECS::ISystem {
    public:
        MySystem(int speed) : m_speed(speed) {}

        void Update(ECS::ECS &ecs, uint32_t msecs) override;

    private:
        int m_speed;
        std::vector<ECS::EntityID> m_buff;
};
```

Then in the Update() method you can perform actions using the ecs reference (`msecs` is the value given to `ECS::Update()`):
```cpp
void MySystem::Update(ECS::ECS &ecs, uint32_t msecs)
{
    ecs.getEntitiesByComponentsAllOf<exampleComponent>(m_buff);

    // Do something...
}
```
To add your system to the ecs you can do so like that, the arguments are forwarded to the system's constructor. The ECS owns the system and returns a reference to it:
```cpp
MySystem &sys = ecs.addSystem<MySystem>(5);
```
Systems are identified by their type. You can toggle your system to turn it on/off during runtime, check if it is enabled or not and get it back:
```cpp
ecs.toggleSystem<MySystem>();
bool is_on = ecs.systemIsEnabled<MySystem>();
MySystem &same_sys = ecs.getSystem<MySystem>();
```
Every enabled system has its Update() method called when the ECS's Update() method is called, in order of priority. Systems have a priority (`SysPriority`, default `SYSTEM_PRIORITY_DEFAULT` = 10), the lowest value is updated first, the order between systems of equal priority is unspecified:
```cpp
ecs.systemSetPriority<MySystem>(1); // Now updates before default priority systems
ecs.Update(16);
```
Note that toggling a system off then on again resets its priority to the default, and that only one system of each type can exist.

## Debug checks and limits

- **`BLOB_DEBUG`**: by design, the ECS trusts the caller in release builds. Checks that could be avoided statically (for instance using or querying a component that was never registered) are only enabled when `BLOB_DEBUG` is defined. In release builds, using an unregistered component is undefined behaviour. In debug builds `getPool()` throws `ERROR::UnregisteredComponent`.
- **Component types limit**: at most 128 component types (tags included) can be registered per ECS, registering more throws `ERROR::TooManyComponentTypes`.
- **Memory**: the `Registry` holds fixed-size tables indexed by component type ID (around 640 KB). Do not create an `ECS` on the stack for large programs, allocate it on the heap or make it static.
- **Exceptions**: `ERROR::ComponentAlreadyAttached` is thrown when adding a component an entity already has.
