#include "linelist.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tempmodel.h"
#include "readfile.h"
#include "parsingfuncs.h"
#include "capture.h"
#include "actions.h"

void get_dimensions(line_list **hd) { //improve error messages
	unsigned int i;
	char *str;
	line_list *nxt, *head = *hd;
	while (head != NULL) {
		i = 0;
		str = get_word(head->str, &i);
		if (str != NULL) {
			if (!strcmp(str, "dimensions")) {
				if (sscanf((head->str)+i, "%i", &i)) {
					if (set_dimensions(i)) {
						if (head->prev == NULL) {
							*hd = nxt = head->next;
						}
						else {
							(head->prev)->next = head->next;
							nxt = head->next;
							(nxt)->prev = head->prev;
						}
						free((char*) head->str);
						free(head);
						head = nxt;
						if (head == NULL)
							break;
					}
				}
			}
			free(str);
		}
		head = head->next;
	}
}

unsigned int pts_sp(line_list *head, temp_point **tree, temp_spring **sp) { //returns the no of points, 0 is error
	unsigned int i, no_nodes;
	char *str, *str2;
	vector v;
	temp_spring *next_spring;
	float mass, damping;
	line_list *device_parameters;
	no_nodes = 0;
	do {
		if (isspace(head->str[1])) {
			i = 2;
			switch (head->str[0]) {
				case 'p': //points
					str = get_word(head->str, &i);
					if (str == NULL) {
						printf("line %i: point unamed\n", head->lineno);
						return 0;
					}
					v = read_vector(head->str, &i);
					if (v == NULL) {
						printf("invalid vector declaration for point %s\n", str);
						free(str);
						return 0;
					}
					if (sscanf((head->str)+i, "%f %f", &mass, &damping) != 2) {
						printf("invalid value for mass or damping for point %s\n", str);
						free(str);
						free(v);
						return 0;
					}
					if (!add_point(tree, str, mass, damping, v)) {
						printf("multiple definitions of point %s\n", str);
						free(str);
						free(v);
						return 0;
					}
					free(str); //could just use them in the temp structure instead of copying as is done now
					free(v);
					no_nodes++;
					break;
				case 's': //springs
					str = get_word(head->str, &i);
					if (str == NULL) {
						printf("spring startpoint undefined\n");
						free(str);
						return 0;
					}
					str2 = get_word(head->str, &i);
					if (str2 == NULL) {
						printf("spring endpoint undefined\n");
						free(str);
						free(str2);
						return 0;
					}
					if (sscanf((head->str)+i, "%f %f", &mass, &damping) != 2) { //reusing the floats so that "mass" is the restlength and "damping" is the spring constant k
						printf("invalid value for mass or damping for spring between %s and %s\n", str, str2);
						free(str);
						free(str2);
						return 0;
					}
					next_spring = add_spring(*tree, *sp, str, str2, mass, damping);
					if (next_spring == NULL) {
						printf("one or both of the points %s and %s doesn't exist at the point of the spring definition\n", str, str2);
						free(str);
						free(str2);
						return 0;
					}
					free(str);
					free(str2);
					*sp = next_spring;
					break;
				case 'c': //capture devices
					str = get_word(head->str, &i); //device type
					str2 = get_word(head->str, &i); //output port name
					device_parameters = NULL;
					extract_curly_braces(&head, &device_parameters, &i);
					register_capture(str, str2, device_parameters);
					break;
				case 'a': //actions
					str = get_word(head->str, &i); //the action to perform
					device_parameters = NULL;
					extract_curly_braces(&head, &device_parameters, &i);
					register_action(str, device_parameters);
			}
		}
		head = head->next;
	} while (head != NULL);
	return no_nodes;
}
