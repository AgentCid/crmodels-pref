/*
 * make_hr
 *
 * by Marcello Balduccini (marcello.balduccini@gmail.com)
 *
 * Copyright Eastman Kodak Company 2009-2012  All Rights Reserved
 * ----------------------------------------------------------------
 *
 * Pre-processor that takes a CR-Prolog program in input and generates
 * a hard reduct, suitable for computing the answer sets of the
 * CR-Prolog program.
 *
 * As usual, an attempt is made to support both lparse syntax and dlv syntax.
 *
 * The parser also processes #sig signature specifications from dlv_rsig.
 *
 * Run with:
 *
 *   hr file1 [file2 ...]
 *
 * The program will output the target program to the console. To store the
 * target program in a file, use redirection. For example, to process files
 * "main-file.lp" and "extra-file.lp" and store the result in file
 * "hr.lp", use:
 *
 *   hr main-file.lp extra-file.lp > hr.lp
 *
 *-------------------------------------
 *   
 *
 *   History
 *  ~~~~~~~~~
 *   09/18/09 - [2.0.0] First version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <string>
#include <vector>

#include "crmodels2.h"
#include "keywords.h"

#include <aspparser/parser.h>

/* debug flags */
//#define DEBUG_TREE 1

using namespace std;

bool non_exclusive;
bool non_transitive;
bool ip_transitive;
bool pi_transitive;


/* it will be better to put it in the aspparser library (parser.c) */
struct rule *create_rule(int type,int head_size,int body_size)
{	struct rule *r;

	r=(struct rule *)calloc(1,sizeof(struct rule));
	if (head_size>0)
		r->head=(struct item **)calloc(head_size,sizeof(struct item *));
	r->head_size=head_size;

	if (body_size>0)
		r->body=(struct item **)calloc(body_size,sizeof(struct item *));
	r->body_size=body_size;
	r->type=type;

	return(r);
}

/* it will be better to put it in the aspparser library (parser.c) */
struct item *dup_item(struct item *i1)
{	struct item *i2;

	i2=makeitem(i1->arity);
	i2->relation=strdup(i1->relation);
	i2->is_variable=i1->is_variable;
	i2->is_term=i1->is_term;
	i2->is_infix=i1->is_infix;
	i2->is_parenthesis=i1->is_parenthesis;
	i2->arglist_end=i1->arglist_end;
	i2->skip_parentheses=i1->skip_parentheses;
	i2->default_negation=i1->default_negation;
	i2->strong_negation=i1->strong_negation;
	
	for(int i=0;i<i1->arity;i++)
		i2->args[i]=dup_item(i1->args[i]);

	return(i2);
}


struct node *add_system_directive(struct node *program,const char *dir)
{	struct rule *r;

	r=create_rule(RULE_SYSTEM_DIRECTIVE,0,0);
	r->system_directive=strdup(dir);

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

	return(program);
}

/* 4. CRtoNormalRules()
	
     * Create fact cr__crname(<n>) where <n> is the name
       of the cr-rule.
	       
     * Create rule cr__bodytrue(<n>) :- <body of the rule>.
	     
     * Add cr__applcr(<n>) to the body of the cr-rule.
	     
     * Replace +- connective by :- .
	     
     * Remove the name of the rule.
 */
void crrules_to_regrules(struct node *program,struct node *new_rules)
{	struct node *n;

	for(n=program->next;n!=NULL;n=n->next)
	{	struct rule *r;

		r=n->data;

		if (r->type==RULE_CR_RULE)
		{	struct item *atom;
			struct rule *r2;

			/* forbid unnamed cr-rules, as there is no good way to assign a name automatically */
			if (r->name==NULL)
			{	printf("Error: unnamed cr-rules are not allowed. Offending cr-rule is:\n");
				output_rule(r);
				printf("***Aborting.\n");
				exit(1);
			}

			/* Create fact cr__crname(<n>) where <n> is the name of the cr-rule. */
			atom=makeitem(1);
			atom->relation=strdup(CRNAME);
			atom->args[0]=dup_item(r->name);

			r2=create_rule(RULE_REGULAR,1,0);
			r2->head[0]=atom;

			new_rules->next=(struct node *)calloc(1,sizeof(struct node));
			new_rules=new_rules->next;
			new_rules->data=r2;

			/* Create rule cr__bodytrue(<n>) :- <body of the rule>. */
			atom=makeitem(1);
			atom->relation=strdup(BODYTRUE);
			atom->args[0]=dup_item(r->name);
			r2=create_rule(RULE_REGULAR,1,r->body_size);
			r2->head[0]=atom;
			for(int i=0;i<r->body_size;i++)
				r2->body[i]=dup_item(r->body[i]);

			new_rules->next=(struct node *)calloc(1,sizeof(struct node));
			new_rules=new_rules->next;
			new_rules->data=r2;

			/* Add cr__applcr(<n>) to the body of the cr-rule. */
			
			atom=makeitem(1);
			atom->relation=strdup(APPLCR);
			atom->args[0]=dup_item(r->name);
			struct item **body;
			body=(struct item **)calloc(r->body_size+1,sizeof(item *));
			for(int i=0;i<r->body_size;i++)
				body[i]=r->body[i];
			body[r->body_size]=atom;
			free(r->body);
			r->body=body;
			r->body_size++;
			
			/* Replace +- connective by :- . */
			r->type=RULE_REGULAR;
			
			/* Remove the name of the rule. */
			r->name=NULL;
			
		}
	}
}

/* 1. CreateChoiceRule1()
      Add choice rule:

    { cr__applcr(R_internal) : cr__crname(R_internal) } :- not cr__stop.

      NEW [marcy 092209]
        as a workaround to gringo's inability to reproduce lparse's
	"-d all" behavior, we add a choice rule for cr__stop:

    { cr__stop }.

    #show cr__stop.
 */
void add_choice_rule(struct node *program)
{	struct rule *r;
	struct item *a1,*a2,*a3;

	/* get to the last node (rule) in the program */
	while(program->next!=NULL) program=program->next;

	/*    { cr__applcr(R_internal) : cr__crname(R_internal) } :- not cr__stop.
	 */
	a1=maketerm(0);
	a1->relation=strdup("");

	a2=makeitem(2);
	a2->relation=strdup("{ "APPLCR"("INTERNAL") : "CRNAME"("INTERNAL") }");
	a2->is_infix=1;
	a2->args[0]=a1;
	a2->args[1]=dup_item(a1);

	a3=makeitem(0);
	a3->relation=strdup(STOP);
	a3->default_negation=1;

	r=create_rule(RULE_REGULAR,1,1);
	r->head[0]=a2;
	r->body[0]=a3;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

	/*    { cr__stop }.
	 */
	a1=maketerm(0);
	a1->relation=strdup("");

	a2=makeitem(2);
	a2->relation=strdup("{ "STOP" }");
	a2->is_infix=1;
	a2->args[0]=a1;
	a2->args[1]=dup_item(a1);

	r=create_rule(RULE_REGULAR,1,0);
	r->head[0]=a2;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

	/*    #show cr__stop.
	 */
	program=add_system_directive(program,"#show "STOP".");
//	r=create_rule(RULE_SYSTEM_DIRECTIVE,0,0);
//	r->system_directive=strdup("#show "STOP".");
//	program->next=(struct node *)calloc(1,sizeof(struct node));
//	program=program->next;
//	program->data=r;

}

/* 2. CreateIs_preferredRules()
      Add rules:

    #show cr__crname(_).
    #show cr__is_preferred(_,_).
    #show cr__appl(_).
    #show cr__applcr(_).
    #show cr__applx(_).
    #show cr__bodytrue(_).

    cr__is_preferred(R_internal1,R_internal2) :-
        cr__crname(R_internal1),
	cr__crname(R_internal2),
        prefer(R_internal1,R_internal2).

    cr__appl(R_internal) :- cr__crname(R_internal), cr__applcr(R_internal).
    cr__appl(R_internal) :- cr__crname(R_internal), cr__applx(R_internal).

    :- cr__is_preferred(R_internal,R_internal),cr__crname(R_internal).

    cr__is_preferred(R_internal1,R_internal2) :-
        prefer(R_internal1,R_internal3),
	cr__is_preferred(R_internal3,R_internal2),
	cr__crname(R_internal1),
	cr__crname(R_internal2),
	cr__crname(R_internal3).

	<if exclusive>
    :- cr__appl(R_internal1),cr__appl(R_internal2),
       cr__is_preferred(R_internal1,R_internal2),
       cr__crname(R_internal1),
       cr__crname(R_internal2).

    :- not cr__bodytrue(R_internal),cr__applcr(R_internal),
       cr__crname(R_internal).
 */
void add_ispreferred_rules(struct node *program)
{	struct rule *r;
	struct item *cr_name,*cr_name1,*cr_name2,*cr_name3;
	struct item *a;
	struct item *v,*v1,*v2,*v3;

	/* get to the last node (rule) in the program */
	while(program->next!=NULL) program=program->next;

	/* variables used in all these rules */
	v=maketerm(0);
	v->relation=strdup(INTERNAL);
	v->is_variable=1;
	v1=maketerm(0);
	v1->relation=strdup(INTERNAL1);
	v1->is_variable=1;
	v2=maketerm(0);
	v2->relation=strdup(INTERNAL2);
	v2->is_variable=1;
	v3=maketerm(0);
	v3->relation=strdup(INTERNAL3);
	v3->is_variable=1;

	/* cr__crname(_) atoms */
	cr_name=makeitem(1);
	cr_name->relation=strdup(CRNAME);
	cr_name->args[0]=dup_item(v);
	cr_name1=makeitem(1);
	cr_name1->relation=strdup(CRNAME);
	cr_name1->args[0]=dup_item(v1);
	cr_name2=makeitem(1);
	cr_name2->relation=strdup(CRNAME);
	cr_name2->args[0]=dup_item(v2);
	cr_name3=makeitem(1);
	cr_name3->relation=strdup(CRNAME);
	cr_name3->args[0]=dup_item(v3);

/*    #show cr__crname(_).
      #show cr__is_preferred(_,_).
      #show cr__appl(_).
      #show cr__applcr(_).
      #show cr__applx(_).
      #show cr__bodytrue(_).
 */
	//r=create_rule(RULE_SYSTEM_DIRECTIVE,0,0);
	//r->system_directive=strdup("#show "CRNAME"(_), "IS_PREF"(_,_), "APPL"(_), "APPLCR"(_), "APPLX"(_), "BODYTRUE"(_).");
	//program->next=(struct node *)calloc(1,sizeof(struct node));
	//program=program->next;
	//program->data=r;
	program=add_system_directive(program,"#show "CRNAME"(_).");
	program=add_system_directive(program,"#show "IS_PREF"(_,_).");
	program=add_system_directive(program,"#show "APPL"(_).");
	program=add_system_directive(program,"#show "APPLCR"(_).");
	program=add_system_directive(program,"#show "APPLX"(_).");
	program=add_system_directive(program,"#show "BODYTRUE"(_).");

/*    cr__is_preferred(R_internal1,R_internal2) :-
          cr__crname(R_internal1),
	  cr__crname(R_internal2),
          prefer(R_internal1,R_internal2).
 */
	r=create_rule(RULE_REGULAR,1,3);
	a=makeitem(2);
	a->relation=strdup(IS_PREF);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->head[0]=a;
	r->body[0]=dup_item(cr_name1);
	r->body[1]=dup_item(cr_name2);
	a=makeitem(2);
	a->relation=strdup(PREFER);	
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->body[2]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

/*    cr__appl(R_internal) :- cr__crname(R_internal), cr__applcr(R_internal).
 */
	r=create_rule(RULE_REGULAR,1,2);
	a=makeitem(1);
	a->relation=strdup(APPL);
	a->args[0]=dup_item(v);
	r->head[0]=a;
	r->body[0]=dup_item(cr_name);
	a=makeitem(1);
	a->relation=strdup(APPLCR);
	a->args[0]=dup_item(v);
	r->body[1]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

/*    cr__appl(R_internal) :- cr__crname(R_internal), cr__applx(R_internal).
 */
	r=create_rule(RULE_REGULAR,1,2);
	a=makeitem(1);
	a->relation=strdup(APPL);
	a->args[0]=dup_item(v);
	r->head[0]=a;
	r->body[0]=dup_item(cr_name);
	a=makeitem(1);
	a->relation=strdup(APPLX);
	a->args[0]=dup_item(v);
	r->body[1]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

/* <if no special properties (exclusivity, modified transitivity)>
 *   :- cr__is_preferred(R_internal,R_internal),cr__crname(R_internal).
 */
	if(!non_exclusive&&!non_transitive&&!ip_transitive&&!pi_transitive){
		r=create_rule(RULE_REGULAR,0,2);
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v);
		a->args[1]=dup_item(v);
		r->body[0]=a;
		r->body[1]=dup_item(cr_name);

		program->next=(struct node *)calloc(1,sizeof(struct node));
		program=program->next;
		program->data=r;
	}

/*	<if no other transitivity is defined>
 * cr__is_preferred(R_internal1,R_internal2) :-
          prefer(R_internal1,R_internal3),
	  cr__is_preferred(R_internal3,R_internal2),
	  cr__crname(R_internal1),
	  cr__crname(R_internal2),
	  cr__crname(R_internal3).
 */
	if(!non_transitive&&!ip_transitive&&!pi_transitive){
		r=create_rule(RULE_REGULAR,1,5);
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v2);
		r->head[0]=a;
		a=makeitem(2);
		a->relation=strdup(PREFER);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v3);
		r->body[0]=a;
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v3);
		a->args[1]=dup_item(v2);
		r->body[1]=a;
		r->body[2]=dup_item(cr_name1);
		r->body[3]=dup_item(cr_name2);
		r->body[4]=dup_item(cr_name3);

		program->next=(struct node *)calloc(1,sizeof(struct node));
		program=program->next;
		program->data=r;
	}

/*	<if ip-transitivity is defined>
 * cr__is_preferred(R_internal1,R_internal2) :-
		  not prefer(R_internal1,R_internal3),
		  not prefer(R_internal3,R_internal1),
		  cr__is_preferred(R_internal3,R_internal2),
	  cr__crname(R_internal1),
	  cr__crname(R_internal2),
	  cr__crname(R_internal3),
	  R_internal1 != R_internal2,
	  R_internal2 != R_internal3,
	  R_internal1 != R_internal3.
 */
	if(ip_transitive){
		r=create_rule(RULE_REGULAR,1,9);
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v2);
		r->head[0]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_PREFER);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v3);
		r->body[0]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_PREFER);
		a->args[0]=dup_item(v3);
		a->args[1]=dup_item(v1);
		r->body[1]=a;
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v3);
		a->args[1]=dup_item(v2);
		r->body[2]=a;

		r->body[3]=dup_item(cr_name1);
		r->body[4]=dup_item(cr_name2);
		r->body[5]=dup_item(cr_name3);

		a=makeitem(2);
		a->relation=strdup(NOT_EQ);
		a->is_infix=1;
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v2);
		r->body[6]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_EQ);
		a->is_infix=1;
		a->args[0]=dup_item(v2);
		a->args[1]=dup_item(v3);
		r->body[7]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_EQ);
		a->is_infix=1;
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v3);
		r->body[8]=a;

		program->next=(struct node *)calloc(1,sizeof(struct node));
		program=program->next;
		program->data=r;
	}

/*	<if pi-transitivity is defined>
 * cr__is_preferred(R_internal1,R_internal2) :-
		  not prefer(R_internal2,R_internal3),
		  not prefer(R_internal3,R_internal2),
		  cr__is_preferred(R_internal1,R_internal3),
	  cr__crname(R_internal1),
	  cr__crname(R_internal2),
	  cr__crname(R_internal3),
	  R_internal1 != R_internal2,
	  R_internal2 != R_internal3,
	  R_internal1 != R_internal3.
 */
	if(pi_transitive){
		r=create_rule(RULE_REGULAR,1,9);
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v2);
		r->head[0]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_PREFER);
		a->args[0]=dup_item(v2);
		a->args[1]=dup_item(v3);
		r->body[0]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_PREFER);
		a->args[0]=dup_item(v3);
		a->args[1]=dup_item(v2);
		r->body[1]=a;
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v3);
		r->body[2]=a;

		r->body[3]=dup_item(cr_name1);
		r->body[4]=dup_item(cr_name2);
		r->body[5]=dup_item(cr_name3);

		a=makeitem(2);
		a->relation=strdup(NOT_EQ);
		a->is_infix=1;
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v2);
		r->body[6]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_EQ);
		a->is_infix=1;
		a->args[0]=dup_item(v2);
		a->args[1]=dup_item(v3);
		r->body[7]=a;
		a=makeitem(2);
		a->relation=strdup(NOT_EQ);
		a->is_infix=1;
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v3);
		r->body[8]=a;

		program->next=(struct node *)calloc(1,sizeof(struct node));
		program=program->next;
		program->data=r;
	}

/* <if exclusive>
 * :- cr__appl(R_internal1),cr__appl(R_internal2),
         cr__is_preferred(R_internal1,R_internal2),
         cr__crname(R_internal1),
         cr__crname(R_internal2).
 */
	if(!non_exclusive){
		r=create_rule(RULE_REGULAR,0,5);
		a=makeitem(1);
		a->relation=strdup(APPL);
		a->args[0]=dup_item(v1);
		r->body[0]=a;
		a=makeitem(1);
		a->relation=strdup(APPL);
		a->args[0]=dup_item(v2);
		r->body[1]=a;
		a=makeitem(2);
		a->relation=strdup(IS_PREF);
		a->args[0]=dup_item(v1);
		a->args[1]=dup_item(v2);
		r->body[2]=a;
		r->body[3]=dup_item(cr_name1);
		r->body[4]=dup_item(cr_name2);

		program->next=(struct node *)calloc(1,sizeof(struct node));
		program=program->next;
		program->data=r;
	}

/* :- not cr__bodytrue(R_internal),cr__applcr(R_internal),
         cr__crname(R_internal).
*/
		r=create_rule(RULE_REGULAR,0,3);
		a=makeitem(1);
		a->relation=strdup(BODYTRUE);
		a->args[0]=dup_item(v);
		a->default_negation=1;
		r->body[0]=a;
		a=makeitem(1);
		a->relation=strdup(APPLCR);
		a->args[0]=dup_item(v);
		r->body[1]=a;
		r->body[2]=dup_item(cr_name);

		program->next=(struct node *)calloc(1,sizeof(struct node));
		program=program->next;
		program->data=r;
}

/* 3. CreateIs_preferred2Rules()
      Add:
	      
      #show cr__is_preferred2(__,_).
	      
      cr__prefer2_relation(R_internal1) :- 
          cr__is_preferred2(R_internal1,R_internal2).
	      
      cr__prefer2_relation(R_internal2) :- 
          cr__is_preferred2(R_internal1,R_internal2).

      cr__is_preferred2(R_internal1,R_internal2) :-
          prefer2(R_internal1,R_internal2).

      cr__is_preferred2(R_internal1,R_internal3) :-
          cr__prefer2_relation(R_internal3),
	  prefer2(R_internal1,R_internal2),
	  cr__is_preferred2(R_internal2,R_internal3).
 */
void add_ispreferred2_rules(struct node *program)
{	struct rule *r;
	struct item *a;
	struct item *v1,*v2,*v3;

	/* get to the last node (rule) in the program */
	while(program->next!=NULL) program=program->next;

	/* variables used in all these rules */
	v1=maketerm(0);
	v1->relation=strdup(INTERNAL1);
	v1->is_variable=1;
	v2=maketerm(0);
	v2->relation=strdup(INTERNAL2);
	v2->is_variable=1;
	v3=maketerm(0);
	v3->relation=strdup(INTERNAL3);
	v3->is_variable=1;

/*      #show cr__is_preferred2(_,_).
 */
	program=add_system_directive(program,"#show "IS_PREF2"(_,_).");
//	r=create_rule(RULE_SYSTEM_DIRECTIVE,0,0);
//	r->system_directive=strdup("#show "IS_PREF2"(_,_).");
//	program->next=(struct node *)calloc(1,sizeof(struct node));
//	program=program->next;
//	program->data=r;

/*      cr__prefer2_relation(R_internal1) :- 
            cr__is_preferred2(R_internal1,R_internal2).
*/
	r=create_rule(RULE_REGULAR,1,1);
	a=makeitem(1);
	a->relation=strdup(PREFER2_REL);
	a->args[0]=dup_item(v1);
	r->head[0]=a;
	a=makeitem(2);
	a->relation=strdup(IS_PREF2);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->body[0]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;


/*      cr__prefer2_relation(R_internal2) :- 
            cr__is_preferred2(R_internal1,R_internal2).
 */
 	r=create_rule(RULE_REGULAR,1,1);
	a=makeitem(1);
	a->relation=strdup(PREFER2_REL);
	a->args[0]=dup_item(v2);
	r->head[0]=a;
	a=makeitem(2);
	a->relation=strdup(IS_PREF2);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->body[0]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

/*      cr__is_preferred2(R_internal1,R_internal2) :-
            prefer2(R_internal1,R_internal2).
*/
 	r=create_rule(RULE_REGULAR,1,1);
	a=makeitem(2);
	a->relation=strdup(IS_PREF2);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->head[0]=a;
	a=makeitem(2);
	a->relation=strdup(PREFER2);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->body[0]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;

/*      cr__is_preferred2(R_internal1,R_internal3) :-
            cr__prefer2_relation(R_internal3),
	    prefer2(R_internal1,R_internal2),
	    cr__is_preferred2(R_internal2,R_internal3).
*/
 	r=create_rule(RULE_REGULAR,1,3);
	a=makeitem(2);
	a->relation=strdup(IS_PREF2);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v3);
	r->head[0]=a;
	a=makeitem(1);
	a->relation=strdup(PREFER2_REL);
	a->args[0]=dup_item(v3);
	r->body[0]=a;
	a=makeitem(2);
	a->relation=strdup(PREFER2);
	a->args[0]=dup_item(v1);
	a->args[1]=dup_item(v2);
	r->body[1]=a;
	a=makeitem(2);
	a->relation=strdup(IS_PREF2);
	a->args[0]=dup_item(v2);
	a->args[1]=dup_item(v3);
	r->body[2]=a;

	program->next=(struct node *)calloc(1,sizeof(struct node));
	program=program->next;
	program->data=r;
}

void make_hr(vector<string> flist,string ofile,string efile)
{	int n_files;
	char **files;
	struct node *program,*n;

	FILE *fpo,*fpe;

	if (ofile=="-")
		fpo=stdout;
	else
	{	fpo=fopen(ofile.c_str(),"w");
		if (fpo==NULL)
		{	// fix here!!!
			printf("ERROR\n");
			exit(1);
		}
	}

	if (efile=="-")
		fpe=stderr;
	else
	{	fpe=fopen(efile.c_str(),"w");
		if (fpe==NULL)
		{	// fix here!!!
			printf("ERROR\n");
			exit(1);
		}
	}

	if (flist.size()==0)
	{	n_files=1;
		files=NULL;
	}
	else
	{	files=(char**)calloc(flist.size(),sizeof(char *));
		for(int i=0;i<(int)flist.size();i++)
			files[i]=strdup(flist[i].c_str());
		n_files=flist.size();
	}

	program=parse_program(n_files,files);


#ifdef DEBUG_TREE
	debug_show_program(program);
#endif

	/* 1. CreateChoiceRule1()
	      Add choice rule:

	    { cr__applcr(R_internal) : cr__crname(R_internal) } :- not cr__stop.

	      NEW [marcy 092209]
	        as a workaround to gringo's inability to reproduce lparse's
		"-d all" behavior, we add a choice rule for cr__stop:

	    { cr__stop }.
	    
	    #show cr__stop.
	 */
	add_choice_rule(program);

	/* 2. CreateIs_preferredRules()
	      Add rules:

	    #show cr__crname(_).
	    #show cr__is_preferred(_,_).
	    #show cr__appl(_).
	    #show cr__applcr(_).
	    #show cr__applx(_).
	    #show cr__bodytrue(_).

	    cr__is_preferred(R_internal1,R_internal2) :-
	        cr__crname(R_internal1),
		cr__crname(R_internal2),
	        prefer(R_internal1,R_internal2).

	    cr__appl(R_internal) :- cr__crname(R_internal), cr__applcr(R_internal).
	    cr__appl(R_internal) :- cr__crname(R_internal), cr__applx(R_internal).

		<if no special properties (exclusivity, non transitivity)>
	    :- cr__is_preferred(R_internal,R_internal),cr__crname(R_internal).

		<if no other transitivity is defined>
	    cr__is_preferred(R_internal1,R_internal2) :-
	        prefer(R_internal1,R_internal3),
		cr__is_preferred(R_internal3,R_internal2),
		cr__crname(R_internal1),
		cr__crname(R_internal2),
		cr__crname(R_internal3).

		<if ip-transitivity is defined>
 	 	cr__is_preferred(R_internal1,R_internal2) :-
  	      cr__is_preferred(R_internal3,R_internal2),
		  not prefer(R_internal1,R_internal3),
		  not prefer(R_internal3,R_internal1),
	    cr__crname(R_internal1),
	    cr__crname(R_internal2),
	    cr__crname(R_internal3),
	    R_internal1 != R_internal2,
	    R_internal2 != R_internal3,
	    R_internal1 != R_internal3.

	    <if pi-transitivity is defined>
	    cr__is_preferred(R_internal1,R_internal2) :-
	 	  cr__is_preferred(R_internal1,R_internal3),
		  not prefer(R_internal2,R_internal3),
		  not prefer(R_internal3,R_internal2),
		cr__crname(R_internal1),
		cr__crname(R_internal2),
		cr__crname(R_internal3),
		R_internal1 != R_internal2,
		R_internal2 != R_internal3,
		R_internal1 != R_internal3.

		<if exclusive>
	    :- cr__appl(R_internal1),cr__appl(R_internal2),
	       cr__is_preferred(R_internal1,R_internal2),
	       cr__crname(R_internal1),
	       cr__crname(R_internal2).

	    :- not cr__bodytrue(R_internal),cr__applcr(R_internal),
	       cr__crname(R_internal).
	 */
	add_ispreferred_rules(program);

	/* 3. CreateIs_preferred2Rules()
	      Add:
	      
	      #show cr__is_preferred2(_,_).
	      
	      cr__prefer2_relation(R_internal1) :- 
	          cr__is_preferred2(R_internal1,R_internal2).
	      
	      cr__prefer2_relation(R_internal2) :- 
	          cr__is_preferred2(R_internal1,R_internal2).

	      cr__is_preferred2(R_internal1,R_internal2) :-
	          prefer2(R_internal1,R_internal2).
		  
	      cr__is_preferred2(R_internal1,R_internal3) :-
	          cr__prefer2_relation(R_internal3),
		  prefer2(R_internal1,R_internal2),
		  cr__is_preferred2(R_internal2,R_internal3).
	 */
fprintf(stderr,"ispreferred2 relations DISABLED.\n");
	//add_ispreferred2_rules(program);

	/* 4. CRtoNormalRules()
	
	     * Create fact cr__crname(<n>) where <n> is the name
	       of the cr-rule.
	       
	     * Create rule cr__bodytrue(<n>) :- <body of the rule>.
	     
	     * Add cr__applcr(<n>) to the body of the cr-rule.
	     
	     * Replace +- connective by :- .
	     
	     * Remove the name of the rule.
	 */

	struct node *new_rules;

	new_rules=(struct node *)calloc(1,sizeof(struct node));	/* head of the list */
	crrules_to_regrules(program,new_rules);

	for(n=program;n->next!=NULL;n=n->next);
	n->next=new_rules->next;	/* skip new_rules' head and add the new rules to the program */


	/* when done, add domains */
	add_domains_to_program(program);

	output_program_to_file(program,fpo);

	if (fpo!=stdout) fclose(fpo);
	if (fpe!=stderr) fclose(fpe);
}

void show_usage(void){
	printf("Usage:  \n");
	printf("       hr [<options>] <input> \n");
	printf("		 Options:\n");
	printf("       	   -ne: use non-exclusive preferences for the following input\n");
	printf("              at most one of the following transitivity options can be used:\n");
	printf("       	   		-nt: use non-transitive preferences for the following input\n");
	printf("       	   		-ip: use IP-transitive preferences for the following input\n");
	printf("       	   		-pi: use PI-transitive preferences for the following input\n");
	printf("		 Inputs:");
	printf("           -- :  processes input from console (CTRL+D to terminate input on Unix)\n");
	printf("           <file1> [<file2> [...]] :  processes input files file1,file2,...\n");
	printf("       hr -h\n");
	printf("           prints this help\n\n");
}

int main(int argc,char *argv[])
{	vector<string> files;
	int i;

	fprintf(stderr,"hr/crmodels version "CRMODELS_VERSION"...\n");
	fprintf(stderr,"parser version %s...\n\n",parser_version());

	if (argc==1)
	{	show_usage();
		exit(1);
	}

	if (strcmp(argv[1],"-h")==0)
	{	show_usage();
		exit(0);
	}

	non_exclusive=false;
	non_transitive=false;
	ip_transitive=false;
	pi_transitive=false;
	for(i=1;i<argc && argv[i][0]=='-' && strcmp(argv[i],"--")!=0;i++){
		if (strcmp(argv[i],"-ne")==0){
				non_exclusive=true;
			}
		else
		if (strcmp(argv[i],"-nt")==0){
			if(ip_transitive||pi_transitive){
				fprintf(stderr, "ERROR: -nt, -ip, and -pi are exclusive options\n");
				show_usage();
				exit(1);
			}
			else
				non_transitive=true;
			}
		else
		if (strcmp(argv[i],"-ip")==0){
			if(non_transitive||pi_transitive){
				fprintf(stderr, "ERROR: -nt, -ip, and -pi are exclusive options\n");
				show_usage();
				exit(1);
			}
			else
				ip_transitive=true;
			}
		else
		if (strcmp(argv[i],"-pi")==0){
			if(non_transitive|ip_transitive){
				fprintf(stderr, "ERROR: -nt, -ip, and -pi are exclusive options\n");
				show_usage();
				exit(1);
			}
			else
				pi_transitive=true;
			}
	}


	if (strcmp(argv[i],"--")!=0)
	{	for(int j=i;j<argc;j++)
			files.push_back(argv[j]);
	}

	make_hr(files,"-","-");

	exit(0);
}
