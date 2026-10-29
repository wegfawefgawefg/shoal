(game
  (map
    (name "city5")
    (day-limit 6)
    (funds 0 0)
    (income 1000)
    (tiles
      "....c"
      "....."
      "..c.."
      "....."
      "c....")
    (unit 0 inf a1)
    (unit 1 inf e5)
  )
  (moves
    "a1 c2"
    "end"
    "e5 e4"
    "end"
    "c2 e3 x e4"
    "end"
    "e4 d3 x e3"
    "end"
    "e3 d2 x d3")
  (evals 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000))
