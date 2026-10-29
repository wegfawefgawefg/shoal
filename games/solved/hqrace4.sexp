(game
  (map
    (name "hqrace4")
    (day-limit 8)
    (funds 0 0)
    (income 1000)
    (tiles
      "h..."
      "...."
      "...."
      "...h")
    (owner 0 a1)
    (owner 1 d4)
    (unit 0 inf b1)
    (unit 1 inf c4)
  )
  (moves
    "b1 c3 x c4"
    "end"
    "c4 d4"
    "end"
    "c3 d3 x d4"
    "end"
    "d4 b3"
    "end"
    "d3 b2 x b3"
    "end"
    "b3 a1 cap"
    "end"
    "b2 b1 x a1")
  (evals 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000))
