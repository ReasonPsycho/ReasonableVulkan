
# Engine Module

## Overview

The Engine module is the core orchestrator of the ReasonableVulkan engine, providing a robust Entity-Component-System (ECS) architecture with scene management capabilities. It serves as the central hub that coordinates all game systems, manages entity lifecycles, and provides a structured approach to game object organization through hierarchical scene graphs.

## Features

- **Entity-Component-System (ECS) Architecture**: High-performance, data-oriented design pattern
- **Scene Management**: Multiple scene support with active scene switching
- **Hierarchical Scene Graph**: Parent-child relationships between entities with transform inheritance
- **System-based Processing**: Modular system architecture for game logic separation
- **Component Type Safety**: Template-based component management with compile-time type checking
- **Entity Recycling**: Efficient entity ID management with reuse capabilities
- **Transform System**: Built-in 3D transformation handling with matrix calculations
- **Flexible Component Arrays**: Support for both regular and integral component storage

## Architecture

### Core Classes

#### `Engine`
The main singleton class that orchestrates the entire engine:
- **Scene Management**: Create, switch, and manage multiple scenes
- **Update Loop**: Centralized update coordination for all active systems
- **Singleton Pattern**: Global access point for engine functionality