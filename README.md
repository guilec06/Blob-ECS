# Blob-ECS

A high-performance, lightweight Entity Component System (ECS) library written in modern C++20.

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](https://www.gnu.org/licenses/lgpl-2.1)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

## Overview

Blob-ECS is a custom Entity Component System designed for maximum performance and type safety. On an Apple M5 it reads about **1.27 billion components per second** and adds about **270 million components per second** (see [Benchmarks](#benchmarks) for the full, reproducible numbers).

### Key Features

#### **High Performance**
- **Sparse Set Architecture**: O(1) component access, insertion, and removal
- **Cache-Friendly Design**: Dense component storage for optimal memory locality
- **Vector-Based Lookup**: Direct array indexing with component type IDs (no hash computation)
- **Zero-Cost Abstractions**: Compile-time optimizations with C++20 concepts
- **Signature Masks**: Each entity keeps a bit mask of its components, so multi-component queries start from the smallest pool and filter with a single mask test
- **Struct-of-Arrays Pools**: Components and their owning entity IDs are stored in separate packed arrays
- **Benchmark Results**: ~1.27 billion sequential component reads/s and ~270M adds/s on an Apple M5, see [Benchmarks](#benchmarks)

#### **Type Safety**
- **Compile-Time Type Checking**: C++20 concepts and `requires` clauses restrict tag and data APIs to the right kinds of types
- **Strong Type System**: Components are type-safe throughout execution
- **Template Metaprogramming**: Type errors caught at compile time, not runtime
- **Constructor Forwarding**: Components are built in place from the arguments given to `entityAddComponent`, they do not need a default constructor
- **Tag Components**: Empty types are detected automatically and stored as tags in a compact bit set

#### **Memory Efficiency**
- **Minimal Entity Overhead**: Entities are simple uint32_t IDs, with a generation counter, alive flag and component mask kept in struct-of-arrays metadata
- **Packed Component Storage**: Components stored contiguously in memory
- **Efficient Sparse Arrays**: Fast entity-to-component mapping with minimal waste
- **Automatic Memory Management**: RAII-compliant resource handling

#### **Clean Architecture**
- **Separation of Concerns**: Entities, Components, and Systems are decoupled
- **Flexible Querying**: Query entities by component combinations (AllOf, AnyOf) or by tags, results are written into a buffer you own and can reuse
- **System Management**: Type-indexed systems owned by the ECS, with enable/disable support and priorities
- **Debug Checks**: Redundant safety checks are only compiled in when `BLOB_DEBUG` is defined

### Core Design Principles

1. **Entities as Pure IDs**: Entities are lightweight identifiers (uint32_t), with generations to detect recycled IDs
2. **Sparse Set Storage**: Components use sparse sets for O(1) operations with cache efficiency
3. **Type-Indexed Pools**: Each component type has its own pool, accessed via compile-time type IDs
4. **Data-Oriented Design**: Components are stored in packed arrays for better CPU cache utilization
5. **Header-Only Library**: Easy integration, no linking required

### Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Component Access | O(1) | Direct sparse set lookup |
| Component Add/Remove | O(1) | Swap-and-pop for removal |
| Entity Query | O(n) | Scans the smallest queried pool, other components are checked with a bit mask |
| Entity Creation | O(1) | Reuses freed entity IDs (most recent first) |
| Memory Usage | Sparse | Grows with active entities, not max ID |

### Architecture Overview

```
Registry (Component Manager)
├── ComponentPool<Transform> (Sparse Set)
│   ├── Dense:  components [Transform, Transform, ...]
│   │           entities   [id, id, ...]
│   └── Sparse: [index, NULL, index, ...]
├── ComponentPool<Velocity> (Sparse Set)
└── ComponentPool<Enemy> (Tag pool, empty type -> bit set)

ECS (Entity Manager + Systems)
├── EntityMetadata: generation[], component_mask[], alive[]
├── Free IDs: [id, id, ...]
├── Systems: type -> ISystem (owned), enabled list ordered by priority
└── Registry (component storage)
```

## Installation

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- git (for cloning)

### Download
With git for auto download:
```bash
git clone https://github.com/guilec06/Blob-ECS.git blob_ecs
```
OR for manual download:
- Download .zip from GitHub
- Extract into `blob_ecs` directory

### Usage
```bash
cd blob_ecs/
cp *.hpp your/project/directory
```

Or add the `blob_ecs` directory (the headers are in its root) to your include path:
```bash
g++ -std=c++20 -I/path/to/blob_ecs your_code.cpp -o your_program
```

*Note: The directory `blob_ecs` can be replaced with anything but ensure consistency in your directory names*

## Quick Start

### Basic Example

```cpp
#include "ECS.hpp"

// Define components
struct Position {
    float x, y;
};

struct Velocity {
    float vx, vy;
};

// Empty types are stored as tags
struct Enemy {};

int main() {
    ECS::ECS ecs;
    
    // Register component types
    ecs.registerComponent<Position>();
    ecs.registerComponent<Velocity>();
    
    // Create an entity
    ECS::EntityID player = ecs.entityCreate();
    
    // Add components
    // Constructor arguments are forwarded to the component
    ecs.entityAddComponent<Position>(player, 100.0f, 200.0f);
    ecs.entityAddComponent<Velocity>(player, 5.0f, 0.0f);
    
    // Query entities with specific components (results are written into the buffer)
    std::vector<ECS::EntityID> entities;
    ecs.getEntitiesByComponentsAllOf<Position, Velocity>(entities);
    
    // Access components
    for (auto entity : entities) {
        auto& p = ecs.entityGetComponent<Position>(entity);
        auto& v = ecs.entityGetComponent<Velocity>(entity);
        
        p.x += v.vx;
        p.y += v.vy;
    }
    
    return 0;
}
```

### Using Systems

```cpp
#include "ECS.hpp"

class MovementSystem : public ECS::ISystem {
public:
    void Update(ECS::ECS& ecs, uint32_t msecs) override {
        ecs.getEntitiesByComponentsAllOf<Position, Velocity>(m_entities);
        
        for (auto entity : m_entities) {
            auto& pos = ecs.entityGetComponent<Position>(entity);
            auto& vel = ecs.entityGetComponent<Velocity>(entity);
            
            pos.x += vel.vx;
            pos.y += vel.vy;
        }
    }

private:
    std::vector<ECS::EntityID> m_entities; // Reused between updates
};

int main() {
    ECS::ECS ecs;
    
    ecs.registerComponent<Position>();
    ecs.registerComponent<Velocity>();

    // The ECS constructs and owns the system, constructor arguments are forwarded
    MovementSystem& movement = ecs.addSystem<MovementSystem>();

    // Lower priority values are updated first (default is 10)
    ecs.systemSetPriority<MovementSystem>(1);
    
    // Game loop
    while (running) {
        ecs.Update(dt); // Calls Update() on every enabled system, ordered by priority
    }
    
    return 0;
}
```

## API Reference

### Entity Management

```cpp
// Create a new entity
EntityID entity = ecs.entityCreate();

// Destroy entity and all its components
ecs.entityDelete(entity);

// Check if entity is active
bool active = ecs.entityIsActive(entity);

// How many times this ID has been recycled (detects stale IDs)
uint16_t generation = ecs.entityGetMetaGeneration(entity);
```

### Component Management

```cpp
// Register a component type (required before use)
ecs.registerComponent<MyComponent>();

// Add component to entity (returns reference), arguments go to the constructor
auto& comp = ecs.entityAddComponent<MyComponent>(entity, /* constructor args... */);

// Get component from entity
auto& comp = ecs.entityGetComponent<MyComponent>(entity);

// Remove component from entity
ecs.entityRemoveComponent<MyComponent>(entity);

// Check if entity has component
bool has = ecs.entityHasComponent<MyComponent>(entity);
```

### Tags

Empty types are tags, they carry no data and use their own methods:

```cpp
struct Enemy {};
ecs.registerComponent<Enemy>();

ecs.entityAddTag<Enemy>(entity);
bool is_enemy = ecs.entityHasTag<Enemy>(entity);
ecs.entityRemoveTag<Enemy>(entity);
```

### Entity Queries

Queries clear the buffer you pass and fill it with matching entities. The order of the results is unspecified.

```cpp
std::vector<ECS::EntityID> entities;

// Get all entities with ALL specified components
ecs.getEntitiesByComponentsAllOf<Transform, Velocity>(entities);

// Get all entities with ANY of the specified components
ecs.getEntitiesByComponentsAnyOf<Weapon, Armor>(entities);

// Same queries for tags (empty types)
ecs.getEntitiesByTagsAllOf<Enemy, Frozen>(entities);
ecs.getEntitiesByTagsAnyOf<Enemy, Frozen>(entities);
```

### System Management

```cpp
// Add a system (constructed from the arguments), returns a reference to it
PhysicsSystem& physics = ecs.addSystem<PhysicsSystem>(/* constructor args... */);

// Disable/enable system
ecs.toggleSystem<PhysicsSystem>();
bool enabled = ecs.systemIsEnabled<PhysicsSystem>();

// Change the update order (lower value is updated first, default is 10)
ecs.systemSetPriority<PhysicsSystem>(5);

// Get a system back
PhysicsSystem& same = ecs.getSystem<PhysicsSystem>();

// Update all enabled systems (msecs is passed to each system's Update)
ecs.Update(dt);
```

### Debug Checks and Limits

- Define `BLOB_DEBUG` to enable the checks that are skipped in release builds (for example, using an unregistered component throws `ECS::ERROR::UnregisteredComponent`). In release builds the ECS trusts the caller.
- At most 128 component types (tags included) can be registered per ECS, registering more throws `ECS::ERROR::TooManyComponentTypes`.
- The `Registry` holds fixed-size tables (around 640 KB), allocate the `ECS` on the heap or make it static rather than putting it on the stack.

See [Doc.md](Doc.md) for a longer description of the API.

## Performance Tips

1. **Pre-register components**: Call `registerComponent()` during initialization, not in hot paths
2. **Reuse query buffers**: Keep the `std::vector<EntityID>` you pass to queries around, so no allocation happens per query
3. **Batch operations**: Process entities in groups rather than individually
4. **Prefer tags for flags**: Empty marker types use a compact bit set instead of a sparse set
5. **Avoid frequent add/remove**: Component addition/removal in tight loops can fragment memory

## Benchmarks

Measured on an Apple M5 (10 cores, 24 GB, macOS), Apple clang 21, `-std=c++20 -O3 -DNDEBUG`. Each figure is the median of 15 runs (7 at 1M entities) after a warm-up, on a fresh ECS, then the median of 3 full runs of the program. Rates are in millions of operations per second, higher is better.

The benchmark program lives on the `benchmarks` branch (`cd benchmark && make run`). It only uses the API subset shared by every version of the library, so the same file also runs against older versions. The last column compares with the design that preceded the current pool/registry/query rework (array-of-structs pools, sorted query caches, no signature masks).

| Operation | 100k entities | 1M entities | Current vs previous design (100k / 1M) |
|-----------|--------------:|------------:|:--------------------------------------:|
| Component read, sequential IDs | 1272 | 1272 | 2.7x / 2.7x |
| Component read, random IDs | 960 | 741 | 2.3x / 2.9x |
| `entityHasComponent` | 1123 | 1116 | 1.6x / 1.6x |
| Add component (1 or 2 per entity) | ~270 | ~265 | 1.0x - 1.1x |
| Add + read a 64-byte component | 283 | 283 | 1.1x / 1.1x |
| Entity creation | 269 | 272 | 0.78x / 0.71x |
| Remove component (random order) | 234 | 77 | 1.0x / 0.6x |
| Entity deletion (2 components) | 25 | 15 | 0.18x / 0.23x |
| Spawn + despawn lifecycle | 18 | 17 | 0.5x / 0.5x |
| Churn (delete half, respawn, 10 rounds) | 15 | 9 | 0.5x / 0.4x |
| Query AllOf, 2 components, all match | 631 | 634 | 1.3x / 1.5x |
| Query AllOf, 2 components, 10% match | 5997 | 5616 | 8.1x / 8.0x |
| Query AllOf, 3 components | 1232 | 1266 | 3.0x / 3.2x |
| Query AnyOf, 2 components | 640 | 589 | more than 1000x |
| Movement system frame (query + 2 reads + write) | 319 | 320 | 2.2x / 2.2x |

Query rates count `n` entities per query (`n` = total entities, whatever the number of matches). The AnyOf comparison is only indicative: the previous implementation removed duplicates with a linear search and was quadratic.

Memory: an empty `ECS` with its registry takes about 700 KB (the previous design: 64 KB), and an entity with three small components costs about 100 bytes in total.

What these numbers say:
- Reads, presence checks and multi-component queries are considerably faster than before, and queries that filter down to a small subset benefit the most.
- Entity creation, and especially **entity deletion**, are slower than in the previous design. `Registry::disableEntity` currently tests every one of the 128 mask bits on each deletion, and creation now maintains generation, mask and alive metadata. Workloads that spawn and despawn entities at a high rate are affected.
- The previous design's queries did not compile as committed, they were measured with a small local patch.

## License

This library is licensed under the GNU Lesser General Public License v2.1 (LGPL-2.1).

```
Blob ECS - A lightweight Entity Component System library
Copyright (C) 2025 LECOCQ Guillaume

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
```

See [LICENSE](LICENSE) for full license text.

## What role did AI occupy in this project?

Blob-ECS is designed and written by its author. The architecture, the API and the library code (every `.hpp` file) are human work.

AI (Claude, through Claude Code) is used as an assistant, never as a developer. Its role is limited to:

- **Brainstorming**: talking through design options before deciding on one
- **Reviewer**: reading diffs and pointing out bugs or risky spots
- **Advisor**: giving opinions on trade-offs, conventions and project organisation
- **Small bug fixes**: occasional minor fixes, such as an accidental rename

Beyond that, AI also helped draft and update the written documentation (`README.md`, `Doc.md`) and the benchmark program on the `benchmarks` branch, both of which are checked against the code.

AI is not listed as a contributor in the git history.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Author

**Guillaume LECOCQ** - [guilec06](https://github.com/guilec06)

---

*Built with performance and type safety in mind*
