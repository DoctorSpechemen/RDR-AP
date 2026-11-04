# Building rdr1.apworld

An `.apworld` is a zip of the `rdr1` folder. From the `apworld` folder:

    zip -r rdr1.apworld rdr1 -x "*/__pycache__/*"

(or right-click the `rdr1` folder > Send to > Compressed folder, then rename `rdr1.zip` to `rdr1.apworld`).
The zip must contain the `rdr1/` folder itself at its top level.

Tests: copy `rdr1` into an Archipelago source checkout's `worlds/` folder and run
`python -m pytest worlds/rdr1/test`.
