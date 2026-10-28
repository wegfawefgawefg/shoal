# Formats

## Map

    (map
      (name "hq5")
      (day-limit 10)         ; scored at the end of player 1's turn on this day
      (funds 0 0)            ; starting funds
      (income 1000)          ; per property per turn
      (build inf mech tank)  ; optional: bases build these
      (tiles
        "h.w.."              ; one string per row, top first
        ".=...")
      (owner 0 a1)           ; owned properties; the rest are neutral
      (owner 1 e5)
      (unit 0 inf b1)        ; owner, type, tile, optional internal hp 1..100
      (unit 1 tank e4 70))

Terrain glyphs: `.` plain, `w` wood, `^` mountain, `=` road, `#` bridge, `~` river, `s` sea,
`_` shoal, `r` reef, `c` city, `b` base, `h` hq, `a` airport, `p` port.

Unit atoms: `inf mech recon tank md neo art rkt aa msl apc`.

## Game

    (game
      (map ...)
      (moves "b1 b2" "a2 c2 x c3" "end" ...)
      (evals 12 -40 ...))    ; optional engine score after each move

Move text: `from to` (wait), `from to x target`, `from to cap`, `tile build type`, `end`.
Loading a game replays it and rejects illegal moves.
