sick(lettuce).
sick(okra).
:- not cause(okra).
:- not cause(lettuce).
cause(lettuce) :- rabbits.
cause(okra) :- ants.

r1: rabbits +-.
r2: ants +-.

prefer(r1, r2).