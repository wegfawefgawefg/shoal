# Rules

shoal plays the Advance Wars 2 rules as Advance Wars By Web (AWBW) uses them, with no luck
roll, no COs and no fog. Numbers come from awbw.amarriner.com (units.php, damage.php,
terrain.php), read 2026-10-28 and again 2026-10-29 for air and sea. Where AWBW gives one number
per pair, the AW2 damage chart on warswiki.org/wiki/Damage_chart (read 2026-10-29) says which
weapon fires it and what the machine gun does once primary ammo is gone.

## Units

Ground: Infantry, Mech, Recon, Tank, Md.Tank, Neotank, Artillery, Rockets, Anti-Air,
Missiles, APC. Air: Fighter, Bomber, B-Copter, T-Copter. Sea: Battleship, Cruiser, Lander,
Sub. Cost, move, move type, ammo, fuel, daily fuel and range are in `src/rules/units.hpp`,
the damage tables in `src/rules/damage.hpp`. The Dual Strike units (Stealth, Black Boat,
Carrier, Megatank, Piperunner, Oozium, Black Bomb) are not in.

| unit | cost | move | type | ammo | fuel | per day | range |
|---|---|---|---|---|---|---|---|
| Fighter | 20000 | 9 | air | 9 | 99 | 5 | 1 |
| Bomber | 22000 | 7 | air | 9 | 99 | 5 | 1 |
| B-Copter | 9000 | 6 | air | 6 | 99 | 2 | 1 |
| T-Copter | 5000 | 6 | air | 0 | 99 | 2 | 1 |
| Battleship | 28000 | 5 | sea | 9 | 99 | 1 | 2-6 |
| Cruiser | 18000 | 6 | sea | 9 | 99 | 1 | 1 |
| Lander | 12000 | 6 | lander | 0 | 99 | 1 | 1 |
| Sub | 20000 | 5 | sea | 6 | 60 | 1 (5 dived) | 1 |

Primary weapons spend ammo; machine guns never run out. A unit uses its primary weapon when
it can hit the target and has ammo, otherwise its machine gun. The split by weapon (AW2):

- Mech bazooka, tank / md tank / neotank cannon: vehicles and ships. Their machine guns: foot
  soldiers and copters, and vehicles once the shells are gone.
- B-Copter missiles (6): vehicles and ships. Its machine gun: foot soldiers and copters, and
  vehicles once the missiles are gone.
- Anti-Air vulcan: ground and air. Missiles and Fighter: air only. Bomber: ground and ships.
- Battleship cannon: ground and ships, range 2-6, never after moving, no counter.
- Cruiser anti-sub missiles (9): subs only. Its anti-air guns (machine gun): planes and
  copters.
- Sub torpedoes (6): ships only.
- T-Copter, Lander, APC: unarmed.

## Damage

    damage = floor(base * attacker_hp / 10 * (100 - stars * defender_hp) / 100)

`attacker_hp` and `defender_hp` are the hit points the board shows, 1..10, from internal
1..100 rounded up. Computed in exact integers (`base * ahp * (100 - stars*dhp) / 1000`), so
there is no floating point fix to copy. The luck term is left out. Air units get no terrain
stars wherever they are; ships get the stars of their tile (reef 1, port 3).

A direct defender that survives strikes back at a direct attacker with its new hit points.
Indirect units (artillery, rockets, missiles, battleship) never move and fire on the same turn
and never counter.

## Movement

Move points per tile (AWBW terrain chart, clear weather); `-` is impassable:

| terrain | foot | boot | treads | tires | air | sea | lander |
|---|---|---|---|---|---|---|---|
| plain | 1 | 1 | 1 | 2 | 1 | - | - |
| wood | 1 | 1 | 2 | 3 | 1 | - | - |
| mountain | 2 | 1 | - | - | 1 | - | - |
| road, bridge | 1 | 1 | 1 | 1 | 1 | - | - |
| river | 2 | 1 | - | - | 1 | - | - |
| sea | - | - | - | - | 1 | 1 | 1 |
| shoal | 1 | 1 | 1 | 1 | 1 | - | 1 |
| reef | - | - | - | - | 1 | 2 | 2 |
| city, base, hq, airport | 1 | 1 | 1 | 1 | 1 | - | - |
| port | 1 | 1 | 1 | 1 | 1 | 1 | 1 |

Friendly units can be passed, enemy units block (no fog, so nothing is hidden).

## Fuel

Every unit has a tank (AWBW units.php) and starts full. Moving spends one fuel per move point
of the cheapest path to the tile it stops on; a unit can move at most as many points as it
has fuel. Air and sea units also burn their daily fuel at the start of each of their side's
turns from day 2 on (a dived sub 5 a day); one left with 0 fuel goes down with its cargo before
anything can resupply it, and a side that loses its last unit that way loses the game.
Ground units burn nothing by the day. Cargo burns nothing and keeps its fuel.

## Turns

A turn: income (1000 per owned property), then the daily fuel burn and crashes, then repair
and resupply, then units act one at a time. Each unit moves (or stays) then waits, attacks,
captures, joins, loads, drops, dives or surfaces. Bases build ground units, airports air units
and ports ships when the map's `(build ...)` lists them; a new unit cannot act that turn.
`end` passes the turn.

Repair: a unit on its own property of the right kind at the start of its turn (city, base or
hq for ground units, airport for air units, port for ships) gets its ammo and fuel back and
heals up to 2 board hp (20 internal), paying a tenth of its cost per board hp gained; if it
cannot afford 2 it takes 1.

Join: a unit may move onto a friendly unit of the same type that shows less than 10 hp. Hit
points add (internal, capped at 100); board hp past 10 are refunded at a tenth of the unit's
cost each; ammo and fuel add up to full. The merged unit has acted and keeps a capture in
progress.

Transports: an APC or T-Copter carries one infantry or mech, a Lander two ground units, a
Cruiser two copters. A unit carrying cargo never boards another transport. A unit loads by
moving onto the transport; a transport that has not acted can still move after a unit loads.
Unloading: the transport moves (or stays), then drops one cargo on a free neighbouring tile
the cargo can stand on; the dropped unit has acted. A transport that still holds cargo after a
drop may drop the rest from where it stands as further actions that turn (AW's drop menu
lets you drop both), and nothing else; nothing can board it in between. A Lander loads and
drops only on a shoal or a port (ground units cannot reach it anywhere else anyway). Cargo is
lost with its transport. At the start of its side's turn an APC refills the ammo and fuel of
its side's units next to it, and a Cruiser those of the copters aboard.

Sub: a sub moves (or stays) and dives or surfaces as its action (`a1 b1 dive`,
`a1 b1 surface`); attacking or waiting keeps its state. A dived sub can only be attacked by
cruisers and subs, still attacks itself, and burns 5 fuel a day. Without fog that is all
diving does.

Capture: properties have 20 capture points; capturing removes the capturer's board hp.
Leaving the tile or dying resets the count.

## End of the game

- Capturing the enemy hq wins.
- Losing your last unit loses (rout), in battle or to fuel.
- At the end of player 1's turn on the day limit, the player with more properties wins;
  equal is a draw. This is AWBW's days-limit rule.

## Unconfirmed

Guesses and choices not read off a page:

- No daily fuel is burnt on day 1 (units start the game with a full tank and have not spent a
  night out yet).
- The daily burn and the crash come before repair and resupply, so an air unit that reaches 0
  on its own airport still goes down.
- Cargo burns no daily fuel; only a Cruiser resupplies its cargo (warswiki Cruiser page), APCs
  and Landers do not.
- A Lander may unload from any port; warswiki says "shoals or friendly ports".
- A sub may dive anywhere it can stand, a port included.
- A transport may drop its second cargo where it stands after the first, as its own action;
  it may not move between the drops.
- AWBW's own quirk that loading and unloading never use up the transport's action (guide.php,
  "unique game mechanics") is not modelled: a transport that dropped cargo has acted.
- Nested cargo (a loaded APC boarding a Lander) is not allowed.

Not yet in: fog, Dual Strike units, weather, COs.
