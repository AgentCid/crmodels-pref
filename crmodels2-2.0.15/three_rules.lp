r1: a +-.
r2: b +-.
r3: c +-.

%:- not a, not b, not c.
:- not b, not c.
prefer(r1, r2).
%prefer(r2, r3).
%prefer(r1, r3).
