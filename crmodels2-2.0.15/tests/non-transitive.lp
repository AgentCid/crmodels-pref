friend(barnaby).
friend(reginald).
friend(wellington).

r1: guest(barnaby) +-.
r2: guest(reginald) +-.
r3: guest(wellington) +-.
prefer(r1, r2).
prefer(r2, r3).

:- not guest(A), not guest(B), friend(A), friend(B), A != B.