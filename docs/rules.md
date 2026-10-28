# Rules

shoal plays the Advance Wars 2 rules as Advance Wars By Web (AWBW) uses them, with no luck
roll, no COs and no fog. Numbers come from awbw.amarriner.com (units.php, damage.php,
terrain.php), read 2026-10-28.

## Units

Infantry, Mech, Recon, Tank, Md.Tank, Neotank, Artillery, Rockets, Anti-Air, Missiles, APC
are in the tables (`src/rules/units.cpp`). Missiles only hit air, so on ground maps they are
targets. APC has no transport rules yet (no load/unload, no resupply).

Primary weapons spend ammo; machine guns never run out. A unit uses its primary weapon when
it can hit the target and has ammo, otherwise its machine gun.

Unconfirmed: the AWBW chart lists one value per pair. The machine gun values used once a
tank, md tank or neotank runs out of shells against vehicles (e.g. tank MG vs tank = 6) are
the AW2 values from memory, not read off a page.

## Damage

    damage = floor(base * attacker_hp / 10 * (100 - stars * defender_hp) / 100)

`attacker_hp` and `defender_hp` are the hit points the board shows, 1..10, from internal
1..100 rounded up. Computed in exact integers (`base * ahp * (100 - stars*dhp) / 1000`), so
there is no floating point fix to copy. The luck term is left out.

A direct defender that survives strikes back at a direct attacker with its new hit points.
Indirect units (artillery, rockets, missiles) never move and fire on the same turn and never
counter.

## Turns

A turn: income (1000 per owned property), then repair, then units act one at a time. Each
unit moves (or stays) then waits, attacks, or captures. Bases build when the map has
`(build ...)`; a new unit cannot act that turn. `end` passes the turn.

Repair: a unit on its own city, base or hq at the start of its turn gets its ammo back and
heals up to 2 board hp (20 internal), paying a tenth of its cost per board hp gained; if it
cannot afford 2 it takes 1.

Capture: properties have 20 capture points; capturing removes the capturer's board hp.
Leaving the tile or dying resets the count.

## End of the game

- Capturing the enemy hq wins.
- Losing your last unit loses (rout).
- At the end of player 1's turn on the day limit, the player with more properties wins;
  equal is a draw. This is AWBW's days-limit rule.

Not yet in: joining units, APC load/unload and supply, fuel, air and sea units, fog.
