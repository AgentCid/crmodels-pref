t(1..5).

#domain t(X).

b(3). b(4).

r(X): a(X) +- b(X).

p :- not q.
q :- not p.

some_a :- a(X).

:- not some_a.

prefer(r(X),r(X+1)) :- X < 5.
