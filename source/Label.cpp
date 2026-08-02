#include "Types.h"
//#include <math.h> // need for ftoa
//#include <stdlib.h>  //for itoa and ltoa

void label(HWND, int, char *, int);
void ulabel( HWND hwnd, int button, char *, unsigned int );
void numlabel( HWND , int , int );

void textLabel (HWND hwnd, int button, char*);  // display text only
void realLabel (HWND hwnd, int button, Real num);  // display Real only
//----------------------CODE--------------------

void textLabel ( HWND hwnd, int button, char* title)
{
    SetWindowTextA ( GetDlgItem(hwnd, button), (LPSTR)title);
}
#define THREE_DIGITS
#ifdef THREE_DIGITS
void realLabel (HWND hwnd, int button, Real num)
{
    char numString[21];
    char sign =0;
    char integerString[10];
    char fractionString[10];
    long integer;
    long fraction;
    Real temp;

    if (num < 0.0)
    {
        sign = '-';
        num *= -1;
    }

    integer = num; // integer part of num
    temp = num - integer; // temp = fraction part of num
    temp *= 1000;
    fraction = temp; // fraction part of num
    temp -= fraction;
    if (temp > 0.5)  // round up
        fraction++;
    if (fraction > 999) // round up > .999, add to integer
    {
        fraction -= 1000;
        integer ++;
    }
    if (fraction < 0)
        fraction *= -1;  // don't show the fraction part as negative

    wsprintfA(integerString, "%ld", integer);
    wsprintfA(fractionString, "%ld", fraction);

    if (!sign)
    {
        if (fraction < 10 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.00%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else if (fraction < 100 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.0%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else
            wsprintfA((LPSTR)numString, "%s.%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
    }
    else
    {
        if (fraction < 10 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.00%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else if (fraction < 100 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.0%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else
            wsprintfA((LPSTR)numString, "-%s.%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
    }
    SetWindowTextA(GetDlgItem(hwnd, button), (LPSTR)numString);
}
#else //not THREE_DIGITS, let's do 6
void realLabel (HWND hwnd, int button, Real num)
{
    char numString[21];
    char sign =0;
    char integerString[10];
    char fractionString[10];
    long integer;
    long fraction;
    Real temp;

    if (num < 0.0)
    {
        sign = '-';
        num *= -1;
    }

    integer = num; // integer part of num
    temp = num - integer; // temp = fraction part of num
    temp *= 1000000;
    fraction = temp; // fraction part of num
    temp -= fraction;
    if (temp > 0.5)  // round up
        fraction++;
    if (fraction > 999999) // round up > .999999, add to integer
    {
        fraction -= 1000000;
        integer ++;
    }
    if (fraction < 0)
        fraction *= -1;  // don't show the fraction part as negative

    wsprintfA(integerString, "%ld", integer);
    wsprintfA(fractionString, "%ld", fraction);

    if (!sign)
    {
        if (fraction < 10 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.00000%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        if (fraction < 100 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.0000%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        if (fraction < 1000 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.000%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        if (fraction < 10000 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.00%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else if (fraction < 100000 && fraction != 0)
            wsprintfA((LPSTR)numString, "%s.0%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else
            wsprintfA((LPSTR)numString, "%s.%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
    }
    else
    {
        if (fraction < 10 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.00000%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        if (fraction < 100 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.0000%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        if (fraction < 1000 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.000%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        if (fraction < 10000 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.00%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else if (fraction < 100000 && fraction != 0)
            wsprintfA((LPSTR)numString, "-%s.0%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
        else
            wsprintfA((LPSTR)numString, "-%s.%s", (LPSTR)integerString,
                        (LPSTR)fractionString);
    }
    SetWindowTextA(GetDlgItem(hwnd, button), (LPSTR)numString);
}
#endif //THREE_DIGITS

void label( HWND hwnd, int button, char title[10], int d )
{
    char labeltext[20], numberstring[10];

    wsprintfA (numberstring, "%d", d);
    wsprintfA( (LPSTR)labeltext, "%s = %s",
        (LPSTR)title, (LPSTR)numberstring );
    SetWindowTextA( GetDlgItem( hwnd, button ), (LPSTR)labeltext );
}

void ulabel( HWND hwnd, int button, char title[10], unsigned int d )
{
    char labeltext[20], numberstring[10];
    long ld;

    ld = d;
    wsprintfA (numberstring, "%ld", ld);
    wsprintfA( (LPSTR)labeltext, "%s = %s",
        (LPSTR)title, (LPSTR)numberstring );
    SetWindowTextA( GetDlgItem( hwnd, button ), (LPSTR)labeltext );
}

void numlabel( HWND hwnd, int button, int d )
{
    char numberstring[10];

    wsprintfA(numberstring, "%d", d);
    SetWindowTextA( GetDlgItem( hwnd, button ), (LPSTR)numberstring );
}


