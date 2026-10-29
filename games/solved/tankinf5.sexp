(game
  (map
    (name "tankinf5")
    (day-limit 6)
    (funds 0 0)
    (income 1000)
    (tiles
      "....."
      "....."
      "....."
      "....."
      ".....")
    (unit 0 tank a1)
    (unit 1 inf e5)
    (unit 1 inf d5)
  )
  (moves
    "a1 d4 x d5"
    "end"
    "e5 d3 x d4"
    "d5 c4 x d4"
    "end"
    "d4 d2 x d3")
  (evals 1000000 1000000 -1000000 -1000000 -1000000 1000000))
