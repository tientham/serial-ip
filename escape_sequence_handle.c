/*
 * escape_sequence_handle.c
 *	This is to handle with "escape sequence" based on ASC II.
 *  Created on: Aug 5, 2015
 *      Author: tientham
 */

#include "serial_ip.h"


/*
	Location: escape_sequence_handle.c
	Describe an octal escape sequence (for instance: \0134 => \\) and return the value of the
	expression.  It also returns the length of the escape sequence (max of 4, including the '0').
	Initially, s points to the '0' in the sequence.
*/
int retrieve_octal_escape_sequence(char *s, int *len)
{
	int value = 0;
	int flag = 1;

	for (*len = 1, ++s; *len < 4 && flag; ++(*len), ++s)
	{
		if(*s >= '0' && *s <= '7')
			value = (value * 8) + (*s- '0');
		else
			flag = 0;
	}
	if (*len == 1)						/* was an invalid octal escape sequence*/
		return((int) '0');				/* treat it as "\0" */
	return(value);
}



/*
	Location: escape_sequence_handle.c
	This is based on "ANSC II printable table" for escape sequence.
*/


int escape_sequence(int c)
{
	switch (c) {
	case 'a':							/* alert (bell) */
		return(7);
		/* break; */
	case 'b':							/* backspace */
		return(8);
		/* break; */
	case 'f':							/* form feed */
		return(12);
		/* break; */
	case 'n':							/* line feed */
		return(10);
		/* break; */
	case 'r':							/* carriage return */
		return(13);
		/* break; */
	case 't':							/* tab */
		return(9);
		/* break; */
	case 'v':							/* vertical tab */
		return(11);
		/* break; */
	default:							/* anything else */
		break;
	}
	return(c);
}




/*
	Location: escape_sequence_handle.c
	copy our printf-style arguments to string s and null terminate it.
	it is the caller's responsibility to insure that s points to a
	buffer that is sufficiently large to hold the result.

	returns the number of characters in string s or a negative value if
	an error was encountered.
*/
int esprintf(char *s,char *efmt, ...)
{
	va_list args;
	char *fmt;							/* format str w/o char escapes */
	int ret;							/* returned result code */

	va_start(args,efmt);				/* begin processing varargs */
	fmt = calloc(1,strlen(efmt)+1);		/* alloc memory for other fmt str */
	if (fmt != NULL) {
		escape_sequence_copy(fmt, efmt);/* copy str, interpret char escapes */
		ret = vsprintf(s,fmt,args);
		free(fmt);						/* release alloc memory */
	} else {							/* memory alloc failed */
		ret = -1;
	}
	va_end(args);						/* done processing varargs */
	return(ret);
}

/*
 	Location: escape_sequence_handle.c
	s2 is a input string which contain parameter value type "string".
	This parameter is interpreted as "PRINFSTRING" which may contain esacape sequence.
	Thus, this function is to copy string s2 to s1 and interpret escape characters
	along the way.  Null terminated s1 at the end.

	returns a ptr to s1.
*/
char *escape_sequence_copy(char *s1,char *s2)
{
	int escape_len;							/* length of octal or hex escape */

	while (*s2 != '\0') {
		if (*s2 == '\\') {				/* backslash introduces escape */
			s2++;						/* Point to escape keyword (x - hex; o - octal */
			if (*s2 == '\0') break;		/* if it's null, we're done */
			switch (*s2) {
			case '0':					/* octal escape */
				*s1++ = retrieve_octal_escape_sequence(s2, &escape_len);
				s2 += escape_len;
				break;
			case 'x':					/* hex escape */
				*s1++ = retrieve_hex_escape_sequence(s2, &escape_len);
				s2 += escape_len;
				break;
			default:					/* single char escape */
				*s1++ = escape_sequence(*s2);	/* get value of escape char */
				s2++;
				break;
			}
		} else {						/* regular char */
			*s1++ = *s2++;				/* copy char and increment both ptrs */
		}
	}
	*s1 = '\0';							/* null terminate destination str */
	return(s1);
}

/*
	Location: escape_sequence_handle.c
	This function is to retrieve escape sequence from a string.
	By convention: hex escape sequence is: "\xhh".
	Initially, s points to the 'x' in the sequence.
*/
int retrieve_hex_escape_sequence(char *s, int *len)
{
	int value = 0;

	for (*len = 1, ++s; *len < 3 && isxdigit(*s); ++(*len), ++s)
	{
		if(*s >= 'A' && *s <= 'Z')
			*s = *s + 'a' - 'A';
		value = (value * 16) + (*s - '0');
	}
	if (*len == 1)						/* was an invalid hex escape */
		return((int) 'x');				/* treat it as "\x" */
	return(value);
}
