(game
  (map
    (name "city3")
    (day-limit 6)
    (funds 0 0)
    (income 1000)
    (tiles
      "..."
      ".c."
      "...")
    (unit 0 inf a1)
    (unit 1 inf c3)
  )
  (moves
    "a1 c2 x c3"
    "end"
    "c3 b2 x c2"
    "end"
    "c2 b1 x b2"
    "end"
    "b2 b2 cap"
    "end"
    "b1 b1 x b2")
  (evals 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000))
