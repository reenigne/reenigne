#include "alfe/main.h"

#ifndef INCLUDED_JULIAN_H
#define INCLUDED_JULIAN_H

/* This function returns TRUE if year is a leap year, FALSE otherwise */
bool isleapyear(int year)
{
    return (((year % 400) == 0) || (((year % 4) == 0) && ((year % 100) != 0)));
} /* isleapyear */

/* This function determines if the number of days in month month. Fleap is TRUE
   if the year is a leap year. */
int daysinmonth(int month, bool fleap)
{
    if (month == 2)
        if (fleap)
            return 29;
        else
            return 28;
    else
        if (((month & 8) >> 3) != (month & 1))
            return 31;
        else
            return 30;
} /* daysinmonth */

/* This function returns the Julian equivalent of the date (day, month, year)
*/
long int greg2jul(int day, int month, int year)
{
    long int j;
    int dim;
    long int numleaps;
    bool fleap = isleapyear(year);
    j = day;
    for (dim = 1; dim < month; dim++)
        j += daysinmonth(dim, false);
    if (fleap && month > 2)
        j++;
    year--;
    numleaps = (year / 4) - (year / 100) + (year / 400);
    j += ((long)year * 365 + numleaps);
    return j;
} /* greg2jul */

#endif // INCLUDED_JULIAN_H
