(game
  (map
    (name "hq5")
    (day-limit 6)
    (funds 0 0)
    (income 1000)
    (tiles
      "h.w.."
      ".=..."
      "..c.."
      "...=."
      "..w.h")
    (owner 0 a1)
    (owner 1 e5)
    (unit 0 inf b1)
    (unit 0 tank a2)
    (unit 1 inf d5)
    (unit 1 tank e4)
  )
  (moves
    "a2 e3 x e4"
    "b1 c3 cap"
    "end"
    "e4 e2 x e3"
    "d5 d3 x c3"
    "end"
    "e3 d2 x e2"
    "c3 c3 x d3"
    "end"
    "d3 d3 x c3"
    "end"
    "c3 c3 x d3")
  (evals 1000000 1000000 1000000 -1000000 -1000000 -1000000 1000000 1000000 1000000 -1000000 -1000000 1000000))
