/*	SC	A Spreadsheet Calculator
 *		Routines for piping data to and from an external macro program
 *
 *		Chuck Martin <nrocinu@myrealbox.com>
 *		Original Version Created:  June, 2000
 *
 *		$Revision: 7.16 $
 */

#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "compat.h"
#include "sc.h"

void
getnum(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    struct ent	*p;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    p = *pp;
	    if (p) {
		if (p->cellerror)
		    snprintf(line, sizeof line, "%s", (*pp)->cellerror == CELLERROR ?
			    "ERROR" : "INVALID");
		else if (p->flags & IS_VALID)
		    snprintf(line, sizeof line, "%.15g", p->v);
	    }
	    if (c < cn)
		strlcat(line, "\t", sizeof line);
	    else
		strlcat(line, "\n", sizeof line);
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
fgetnum(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    struct ent	*p;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    p = *pp;
	    if (p) {
		if (p->cellerror)
		    snprintf(line, sizeof line, "%s", p->cellerror == CELLERROR ?
			    "ERROR" : "INVALID");
		else if (p->flags & IS_VALID) {
		    if (p->format) {
			if (*(p->format) == ctl('d')) {
			    time_t i = (time_t) (p->v);
			    strftime(line, sizeof(line), (p->format)+1,
				localtime(&i));
			} else
			    format(p->format, precision[c], p->v, line,
				    sizeof(line));
		    } else
			engformat(realfmt[c], fwidth[c], precision[c],
				p->v, line, sizeof(line));
		}
	    }
	    if (c < cn)
		strlcat(line, "\t", sizeof line);
	    else
		strlcat(line, "\n", sizeof line);
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getstring(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    if (*pp && (*pp)->label)
		snprintf(line, sizeof line, "%s", (*pp)->label);
	    if (c < cn)
		strlcat(line, "\t", sizeof line);
	    else
		strlcat(line, "\n", sizeof line);
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getexp(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    struct ent	*p;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    p = *pp;
	    if (p && p->expr) {
		linelim = 0;
		decompile(p->expr, 0);	/* set line to expr */
		line[linelim] = '\0';
		if (*line == '?')
		    *line = '\0';
	    }
	    if (c < cn)
		strlcat(line, "\t", sizeof line);
	    else
		strlcat(line, "\n", sizeof line);
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getformat(int col, int fd)
{
    snprintf(line, sizeof line, "%d %d %d\n", fwidth[col], precision[col], realfmt[col]);
    write(fd, line, strlen(line));
    linelim = -1;
}

void
getfmt(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    if (*pp && (*pp)->format)
		snprintf(line, sizeof line, "%s", (*pp)->format);
	    if (c < cn)
		strlcat(line, "\t", sizeof line);
	    else
		strlcat(line, "\n", sizeof line);
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getframe(int fd)
{
    struct frange *fr;
    size_t l;

    *line = '\0';
    if ((fr = find_frange(currow, curcol))) {
	snprintf(line, sizeof line, "%s", r_name(fr->or_left->row, fr->or_left->col,
		fr->or_right->row, fr->or_right->col));
	strlcat(line, " ", sizeof line);
	l = strlen(line);
	snprintf(line + l, sizeof(line) - l, "%s", r_name(fr->ir_left->row,
		fr->ir_left->col, fr->ir_right->row, fr->ir_right->col));
    }
    strlcat(line, "\n", sizeof line);
    write(fd, line, strlen(line));
    linelim = -1;
}

void
getrange(char *name, int fd)
{
    struct range *r;
    char *p;

    *line = '\0';
    if (!find_range(name, strlen(name), (struct ent *)0, (struct ent *)0, &r)) {
	snprintf(line, sizeof line, "%s%s%s%d",
		r->r_left.vf & FIX_COL ? "$" : "",
		coltoa(r->r_left.vp->col),
		r->r_left.vf & FIX_ROW ? "$" : "",
		r->r_left.vp->row);
	if (r->r_is_range) {
	    p = line;
	    while (*p)
		p++;
	    snprintf(p, sizeof(line) - (p - line), ":%s%s%s%d",
		    r->r_right.vf & FIX_COL ? "$" : "",
		    coltoa(r->r_right.vp->col),
		    r->r_right.vf & FIX_ROW ? "$" : "",
		    r->r_right.vp->row);
	}
	/************************************************/
	/*                                              */
	/* if(r->r_is_range)                            */
	/*         sprintf(line,"%d:%d:%d:%d",          */
	/*                         r->r_left.vp->col,   */
	/*                         r->r_left.vp->row,   */
	/*                         r->r_right.vp->col,  */
	/*                         r->r_right.vp->row); */
	/* else                                         */
	/*         sprintf(line,"%d:%d",                */
	/*                         r->r_left.vp->col,   */
	/*                         r->r_left.vp->row);  */
	/*                                              */
	/************************************************/
    }
    strlcat(line, "\n", sizeof line);
    write(fd, line, strlen(line));
    linelim = -1;
}

void
doeval(struct enode *e, char *fmt, int row, int col, int fd)
{
    double v;

    gmyrow = row;
    gmycol = col;

    v = eval(e);
    if (fmt) {
	if (*fmt == ctl('d')) {
	    time_t tv = v;
	    strftime(line, FBUFLEN, fmt + 1, localtime(&tv));
	} else
	    format(fmt, precision[col], v, line, FBUFLEN);
    } else
	snprintf(line, sizeof line, "%.15g", v);
    strlcat(line, "\n", sizeof line);
    write(fd, line, strlen(line));
    linelim = -1;

    efree(e);
    if (fmt) scxfree(fmt);
}

void
doseval(struct enode *e, int row, int col, int fd)
{
    char *s;

    gmyrow = row;
    gmycol = col;

    s = seval(e);
    if (s)
	write(fd, s, strlen(s));
    write(fd, "\n", 1);
    linelim = -1;

    efree(e);
    if (s)
	scxfree(s);
}


void
doquery(char *s, char *data, int fd)
{
    goraw();
    query(s, data);
    deraw(0);
    if (linelim >= 0) {
	write(fd, line, strlen(line));
	write(fd, "\n", 1);
    }

    line[0] = '\0';
    linelim = -1;
    CLEAR_LINE;
    update(0);

    if (s) scxfree(s);
}

void
dogetkey(void) {
    int c, len;

    goraw();
    c = nmgetch();
    deraw(0);

    if (c < 256) {
	snprintf(line, sizeof line, "%c", c);
	len = 1;
    } else if (c >= KEY_MIN && c <= KEY_MAX) {
	int i, j;
	line[0] = '\0';
	snprintf(line + 1, sizeof(line) - 1, "%s\n", keyname(c));
	for (i = 1, j = 5; line[j-1]; ) {
	    if (line[j] == '(' || line[j] == ')')
		j++;
	    else
		line[i++] = line[j++];
	}
	len = strlen(line + 1) + 1;
    } else {
	line[0] = '0';
	snprintf(line + 1, sizeof(line) - 1, "UNKNOWN KEY");
	len = strlen(line + 1) + 1;
    }


    write(macrofd, line, len);
}

void
dostat(int fd)
{
    *line = '\0';
    if (modflg)			snprintf(line, sizeof line, "m");
    if (isatty(STDIN_FILENO))	strlcat(line, "i", sizeof line);
    if (isatty(STDOUT_FILENO))	strlcat(line, "o", sizeof line);
    strlcat(line, "\n", sizeof line);
    write(fd, line, strlen(line));
    linelim = -1;
}
