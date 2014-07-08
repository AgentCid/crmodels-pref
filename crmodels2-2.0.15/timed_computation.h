#ifndef TIMED_COMPUTATION_H
#define TIMED_COMPUTATION_H

extern char *get_total_cputime_string(void);
extern int timed_system(char *cmd);
extern int timed_mkatoms_system(char *cmd,char *outfile);
extern void timed_mkatoms_system_err(char *cmd,char *outfile);
extern void set_cputime_limit(int cputime_l);
extern void set_error_on_timeout(bool error_on_timeout_l);
extern const char *solver_cputime_opt(bool cputime_aware_solver);

#endif
