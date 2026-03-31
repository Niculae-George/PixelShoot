# PixelShoot

PixelShoot is a simple multiplayer top-down shooter prototype built with Raylib and ENet.

## Project Status

This project is unfinished and currently in prototype stage.

You can already host and join a lobby, generate a shared map, move, and shoot, but the full competitive loop (including complete score rules and match flow) is still in progress.

## Objective And Scope

The core goal is a lightweight multiplayer game where players compete for score in short matches.

Current scope:
- Host or join a lobby
- Start a shared match from the host
- Spawn all players on a deterministic map (same map for everyone)
- Move and shoot in real time

Planned scope (not complete yet):
- Full score tracking and winner logic
- Better in-game synchronization and hit validation
- Match restart and round lifecycle
- UI/UX polish and balancing

## High-Level Architecture

PixelShoot uses a client-server model with two executables:

- Client: mygame
	- UI and rendering using Raylib
	- Game flow managed through stage/state system
	- Sends/receives network messages through ENet

- Server: server
	- Lightweight relay/authority process on port 7777
	- Handles lobby player counting before game starts
	- Broadcasts game start and deterministic world seed
	- Relays gameplay packets between connected peers during match

### Client State Flow

- MenuStage
	- Player chooses Host Lobby or Join Lobby
	- Join flow includes target IP input

- LobbyStage
	- Host can auto-launch local server process
	- Client connects to server and receives player count updates
	- Host sends Start Game command

- GameStage
	- Receives map seed from server
	- Generates world layout from the same seed
	- Handles movement, shooting, collisions, camera, fog-of-war style mask

### World Generation

World generation is deterministic from a seed sent by the server. The generator:
- Splits map into territories
- Creates walls and corridors
- Preserves safe spawn zones
- Validates basic path connectivity

This ensures all clients can build equivalent maps from the same seed.

### Networking Notes

Current packet flow includes:
- PLAYER_COUNT update (lobby)
- START_GAME + map seed
- In-game relay packets (server forwards peer traffic)

The protocol is intentionally minimal right now and will evolve as scoring/combat validation is expanded.

## Controls

- Move: WASD or Arrow Keys
- Shoot: Left Mouse Button

## How To Run

### Requirements

- CMake 3.16+
- A C++ compiler with modern C++ support (project is configured for C++26)
- Windows is currently the primary tested environment

### Build

From project root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

This builds:
- mygame (client)
- server (dedicated lightweight server)

### Start A Local Match

1. Run mygame.
2. Click Host Lobby.
3. The client tries to auto-start server in the same output folder.
4. Click Start Game in lobby.

If auto-start fails, launch server manually from the build output and then host again.

### Join A Host

1. Run mygame on another machine (or instance).
2. Enter host IP in the join IP field.
3. Click Join Lobby.
4. Wait for host to start the match.

## Troubleshooting

- Ensure port 7777 is available and allowed in firewall.
- If you change the PRODUCTION_BUILD option, clear build output and rebuild.
- Host and clients should run the same build revision.

## Naming Note

The codebase still contains legacy naming such as PixelFight in some UI strings. The intended project name is PixelShoot.
