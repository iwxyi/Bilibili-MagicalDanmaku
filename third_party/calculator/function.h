
#ifndef FUNC_DEFINE
#define FUNC_DEFINE
#include "func_define.h"
#endif

long double function_dispose(int number,long double count[],int len)//这里将要和你的函数进行交接，这里用一个数组将数据先从栈中取出来，然后传入函数总，之后再将函数的执行结果存入到栈中，
{

    switch(number)
    {
    case 1:return SIN(number-1,count,len);break;
    case 2:return COS(number-1,count,len);break;
    case 3:return TAN(number-1,count,len);break;
    case 4:return ARCSIN(number-1,count,len);break;
    case 5:return ARCCOS(number-1,count,len);break;
    case 6:return ARCTAN(number-1,count,len);break;
    case 7:return SINH(number-1,count,len);break;
    case 8:return COSH(number-1,count,len);break;
    case 9:return TANH(number-1,count,len);break;
    case 10:return LOG10(number-1,count,len);break;
    case 11:return LN(number-1,count,len);break;
    case 12:return EXP(number-1,count,len);break;
    case 13:return FACT(number-1,count,len);break;
    case 14:return SQRT(number-1,count,len);break;
    case 15:return CUBEROOT(number-1,count,len);break;
    case 16:return LOG(number-1,count,len);break;
    case 17:return POW(number-1,count,len);break;
    case 18:return MOD(number-1,count,len);break;
    case 19:return YROOT(number-1,count,len);break;
    case 20:return AVG(number-1,count,len);break;
    case 21:return SUM(number-1,count,len);break;
    case 22:return VAR(number-1,count,len);break;
    case 23:return VARP(number-1,count,len);break;
    case 24:return STDEV(number-1,count,len);break;
    case 25:return STDEVP(number-1,count,len);break;
    }
    //return 1;
}
