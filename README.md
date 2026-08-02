# P2C# P2C

P2C is a small multiplayer arena prototype built in **Unreal Engine 5.7** with **C++**.

The project focuses on multiplayer architecture rather than content volume. It demonstrates a complete listen-server flow: session discovery, lobby readiness, seamless travel, server-authoritative arena gameplay, round resolution, score persistence, and returning to the lobby for another round.

## Branching Strategy

The latest stable version of the project is available on the `main` branch.

Ongoing development is carried out on the `develop` branch. New features and fixes are implemented on separate feature branches and then merged into `develop` before being promoted to `main`.

Feature branches have intentionally not been deleted after merging. They are kept to provide full visibility into the development process, including the history of individual features, fixes, experiments, and implementation decisions.

## Current status

The core gameplay loop is complete and playable with a listen server and multiple LAN clients:

```text
Main Menu
    > Host / Find / Join Session
    > Lobby
    > Ready Check
    > Arena
    > Elimination Round
    > Round Summary
    > Lobby
    > Next Round
```

The current development backend is **Online Subsystem Null**, used for LAN testing in the editor and local packaged builds. Steam integration is intentionally kept separate from gameplay and lobby logic and is reserved for packaged-build testing.

## Features

### Sessions and lobby

- Host LAN sessions through a listen server.
- Search for and join available sessions.
- Display readable host names in the session browser.
- Replicated lobby player list.
- Ready-state synchronization.
- Match points displayed in the lobby.
- Only the host can start the match.
- Match start requires the connected players to be ready.
- Seamless travel from Lobby to Arena.
- Basic disconnect handling.

### Arena gameplay

- Replicated third-person characters using Unreal's standard character movement networking.
- Server-authoritative bomb-based elimination rounds.
- Stamina cost for gameplay actions.
- Server-controlled stamina pickups.
- Player elimination and spectator overview.
- Replicated alive-player count and arena phase.
- Round winner selection and match-point award.
- Round summary shown to every connected player.
- Return travel to the lobby after the round.
- Match points preserved across Lobby > Arena > Lobby travel.
- Multiple consecutive rounds supported.

### Presentation

- Delegate-driven HUD updates rather than UI polling every frame.
- Arena HUD for stamina, match points, alive-player count.
- Niagara bomb explosion effect.
- Cosmetic explosion triggered through an unreliable multicast RPC and implemented in Blueprint.
- C++ gameplay logic with Blueprint-configurable assets and presentation.

## Networking model

P2C uses a **listen-server, server-authoritative** architecture.

The host acts as both:

- the authoritative server,
- a local player.

Remote players connect as clients. Gameplay decisions are made by the server, while clients request actions and present replicated results.

### Authority rules

The server is responsible for:

- validating gameplay actions,
- modifying stamina,
- controlling pickups,
- selecting and resolving bomb events,
- eliminating players,
- deciding when a round ends,
- selecting the round winner,
- awarding match points,
- initiating server travel.

Clients are responsible for:

- local input,
- sending action requests to the server,
- displaying replicated state,
- playing cosmetic feedback.

### Replication choices

The project follows several rules to reduce unnecessary network traffic:

- No custom transform RPC is sent every frame; movement uses `ACharacter` and `CharacterMovementComponent` replication.
- Persistent gameplay state is replicated through `GameState`, `PlayerState`, and replicated components.
- UI reacts to replication callbacks and delegates.
- Critical client-to-server actions use reliable server RPCs.
- Cosmetic effects use unreliable multicast RPCs.
- Match points are stored in `PlayerState`, so they survive seamless travel.

Example cosmetic RPC:

```cpp
UFUNCTION(NetMulticast, Unreliable)
void MulticastPlayExplosionEffect();
```

The multicast invokes a Blueprint presentation event locally on every relevant machine. The Niagara system itself does not need to replicate.

## Main Unreal responsibilities

### GameInstance services

Session and connection logic persists independently of individual maps.

Key responsibilities include:

- create session,
- find sessions,
- join session,
- connection recovery,
- travel coordination.

Relevant project types include the online-session service, connection-recovery service, and multiplayer-session subsystem.

### Lobby GameMode and GameState

The lobby layer manages:

- connected players,
- ready states,
- host-only match start,
- validation that the match may begin,
- travel to the arena.

### Arena GameMode and GameState

The arena layer manages:

- arena initialization,
- gameplay phase,
- alive-player tracking,
- round completion,
- winner selection,
- point awards,
- summary flow,
- return travel to the lobby.

`GameMode` remains server-only. Shared match state is exposed through the replicated arena `GameState`.

### PlayerState

`AP2CPlayerState` stores player-owned data that must survive pawn destruction and seamless travel, including:

- player name,
- ready state,
- alive state,
- match points.

### PlayerController

`AP2CPlayerController` is responsible for local-player flow and owner-to-server communication, including:

- creating the correct widget for the current map,
- retrying HUD creation when replicated sources become available,
- sending gameplay and lobby requests,
- switching local input and presentation state.

### Character and stats component

`AP2CCharacter` uses Unreal's standard networked character movement. Player stamina is handled by a dedicated stats component and propagated to the local HUD through delegates.

### Bomb and pickups

The bomb and stamina pickups are controlled by the server. Blueprint subclasses provide meshes, Niagara systems, and other presentation assets without moving gameplay authority out of C++.

## C++ and Blueprint split

### C++

C++ owns:

- network authority,
- replication,
- RPC declarations and validation flow,
- session services,
- lobby and arena state,
- scoring,
- elimination,
- stamina logic,
- round flow,
- travel.

### Blueprints

Blueprints own or configure:

- meshes and materials,
- widget layouts,
- Niagara systems,
- visual feedback,
- presentation events,
- designer-facing defaults.

Example presentation hook:

```cpp
UFUNCTION(BlueprintImplementableEvent, Category = "P2C|Bomb|Presentation")
void BP_PlayExplosionEffect();
```

## Requirements

- Unreal Engine **5.7**
- Windows 64-bit development environment
- A C++ toolchain supported by the installed Unreal Engine version
- LAN access for tests using Online Subsystem Null

## Building the editor target

1. Clone the repository.
2. Right-click `P2C.uproject` and select **Generate Visual Studio project files**.
3. Build the editor target:

```text
Target: P2CEditor
Platform: Win64
Configuration: Development Editor
```

After a successful build, open `P2C.uproject`.

> Changes to reflected C++ declarations such as `UFUNCTION`, `UPROPERTY`, delegates, or Blueprint events should be compiled with the editor closed. Avoid Hot Reload for those changes.

## Running a local multiplayer test

For a quick editor test:

1. Open the Main Menu map.
2. Set Play mode to **New Editor Window (PIE)**.
3. Start two or three player windows.
4. In one window, create a LAN session.
5. In the remaining windows, search for and join the session.
6. Mark players ready in the lobby.
7. Start the match as the host.
8. Complete the arena round and verify the summary and return to lobby.

For tests on separate machines, run compatible builds on the same LAN. One player hosts and the remaining players search for the advertised session.