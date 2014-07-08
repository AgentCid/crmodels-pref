#ifndef CRMODELS_CRMODELS2_H
#define CRMODELS_CRMODELS2_H

#define CRMODELS_VERSION "2.0.15"

#define DEFAULT_GROUNDER "gringo"
/* We must use our front-end, clasp-fe, instead of clasp
 * until clasp starts using standard exit status values.
 */
//#define DEFAULT_SOLVER "clasp"
#define DEFAULT_SOLVER "clasp-fe"

#endif /* CRMODELS_CRMODELS2_H */
