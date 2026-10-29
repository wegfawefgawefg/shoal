(game
  (map
    (name "duel3")
    (day-limit 6)
    (funds 0 0)
    (income 1000)
    (tiles
      "..."
      "..."
      "...")
    (unit 0 inf a1)
    (unit 1 inf c3)
  )
  (moves
    "a1 c2 x c3"
    "end"
    "c3 b1"
    "end"
    "c2 a1 x b1"
    "end"
    "b1 c1"
    "end"
    "a1 b1 x c1")
  (evals 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000))
