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
    "end"
    "c2 c3 cap"
    "end"
    "e5 d3 x c3"
    "end"
    "c3 c3 x d3"
    "end"
    "d3 e1 cap"
    "end"
    "c3 d1 x e1"
    "end"
    "e1 e1 cap"
    "end"
    "d1 d1 x e1")
  (evals 1000000 1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000 1000000 -1000000 -1000000 1000000))
