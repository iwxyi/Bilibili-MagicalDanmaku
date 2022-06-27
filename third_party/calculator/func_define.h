#ifndef FUNC_DEFINE_H
#define FUNC_DEFINE_H

#ifndef PI
#define PI 3.1415926535898
#endif

#ifndef SINT
#define SINT 1000000000000000//如果返回值为这个数，说明字符窜接受有误，也就是说参数的传递有误，
#endif
//可以直接用的函数这里就不用直接写了
//tan也是可以直接用的函数，因为输入不能是原PI

#ifndef MAIN
#define MAIN

#define CALC_MAX_LEN 1000
#define CALC_FUNC_NUM 25
const char calc_func_array[CALC_FUNC_NUM][20]={"SIN","COS","TAN","ARCSIN","ARCCOS","ARCTAN","SINH","COSH","TANH","LOG10","LN","EXP","FACT","SQRT","CUBEROOT","LOG","POW","MOD","YROOT","AVG","SUM","VAR","VARP","STDEV","STDEVP"};//函数名字
typedef struct CalcFun{
    int canshu_number;//表示从字符窜中传递的参数的个数
    int std_number;//表示函数的标准参数的个数
    int bracket;//表示这个函数中括号中的个数
    int count;//记录当前函数内标记走的位置
    long double array[CALC_FUNC_NUM];//接受参数数组
    char temp[CALC_MAX_LEN];
}fun;
fun function_array[CALC_FUNC_NUM];
#endif


//function_dispose(int number,long double count[],int len)

long double SIN(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        if((int)(count[0]/PI)==count[0]/PI)
            return sin(0.0);
        else
        {
            return sin(count[0]*PI/180);
        }
    }
    return SINT;

}

long double COS(int number,long double count[],int len)
{
    if(function_array[number].std_number==len  )
    {
        if((int)(count[0]/PI/2)==count[0]/PI/2 && (int)(count[0]/PI)%2 != 0)
            return 0.0;
        return cos(count[0]*PI/180);
    }
    return SINT;
}

long double TAN(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        //if((int)fmod(count[0],PI)==0&&abs(fmod(count[0]/PI,2))==1)
        ///return SINT;
        //else
        return tan(count[0]*PI/180);
    }
    return SINT;
}

long double ARCSIN(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        if(count[0]>1||count[0]<-1)
            return SINT;
        else
        {
            return asin(count[0]);
        }

    }
    return SINT;
}
//反余弦
long double ARCCOS(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        if(count[0]>1||count[0]<-1)
            return SINT;
        else
            return acos(count[0]);
    }
    return SINT;
}

long double ARCTAN(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        return atan(count[0]);
    }
    return SINT;
}

long double SINH(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        return sinh(count[0]);
    }
    return SINT;
}

long double COSH(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        return cosh(count[0]);
    }

    return SINT;
}

long double TANH(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        return tanh(count[0]);
    }
    return SINT;
}

long double LOG10(int number,long double count[],int len)
{
    if(function_array[number].std_number==len&&count[0]>0)
    {
        return log10(count[0]);
    }
    return SINT;
}
long double LN(int number,long double count[],int len)
{
    if(function_array[number].std_number==len&&count[0]>0)
    {
        return log(count[0]);
    }
    return SINT;
}

long double EXP(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        return exp(count[0]);
    }
    return SINT;
}


//阶乘
long double FACT(int number,long double count[],int len)
{
    long double result=count[0];
    long long rel=1.0;
    if(function_array[number].std_number==len&&count[0]>=0)
    {
        if(floor(count[0])!=count[0])
            return SINT;
        else
        {
            if(count[0]==0)
                return 1;
            while(result!=1.0)
            {
                rel*=result;
                result=result-1.0;
            }
            //cout<<"the result is "<<rel<<endl;
            return rel;
        }
    }
    return SINT;
}

long double SQRT(int number,long double count[],int len)
{
    if(function_array[number].std_number==len&&count[0]>=0)
    {
        return sqrt(count[0]);
    }
    return SINT;
}

long double CUBEROOT(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        if(count[0]<0) return  (-1)*pow(abs(count[0]),(long double)1.0/3);
        else return  pow(count[0],(long double)1.0/3);
    }
    return SINT;
}


//下面是两个函数的
long double LOG(int number,long double count[],int len)
{
    if(function_array[number].std_number==len&&count[1]!=1&&count[0]>0&&count[1]>0)
    {
        return log10(count[0])/log10(count[1]);
    }
    return SINT;
}
//求x的npower次幂（这里的幂指数全是整数）也即乘方运算
long double POW(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        return pow(count[0],count[1]);
    }
    return SINT;
}

long double MOD(int number,long double count[],int len)
{
    if(function_array[number].std_number==len && count[1]!=0)
    {
        return fmod(count[0],count[1]);
    }
    return SINT;
}

//求x的y次方根，y为绝对值大于1的整数
long double YROOT(int number,long double count[],int len)
{
    if(function_array[number].std_number==len)
    {
        if(fmod(count[1],2)==0)
        {
            if(count[0]<0)
                return SINT;
            else
                return pow(count[0],1.0/count[1]);
        }
        else
        {
            if(count[0]<0) return  (-1)*pow(abs(count[0]),(long double)1.0/3);
            else return  pow(count[0],(long double)1.0/3);
        }

    }
    return SINT;
}

long double SUM(int number,long double count[],int len)
{
    long double rel=0.0;
    while(len!=0)
    {
        rel+=count[len-1];
        len--;
    }

    return rel;
}

long double AVG(int number,long double count[],int len)
{
    long double rel=0.0;
    int l=len;
    while(len!=0)
    {
        rel+=count[len-1];
        len--;
    }

    return rel/l;
}

//calculator the follow function

long double VAR(int number,long double count[],int len)
{
    long double avg;
    long double var=0;
    avg=AVG(number,count,len);
    for(int i=0;i<len;i++)
    {
        var+=(count[i]-avg)*(count[i]-avg);
    }
    //cout<<"var 的结果为"<<var/(len-1)<<endl;
    return var/(len-1);
}




//集合的总体方差
long double VARP(int number,long double count[],int len)
{
    long double avg;
    long double var=0;
    avg=AVG(number,count,len);
    for(int i=0;i<len;i++)
    {
        var+=(count[i]-avg)*(count[i]-avg);
    }

    //cout<<"var 的结果为"<<var/len<<endl;
    return var/len;
}

//集合的估算标准差
long double STDEV(int number,long double count[],int len)
{
    long double avg;
    long double var=0;
    avg=AVG(number,count,len);
    for(int i=0;i<len;i++)
    {
        var+=(count[i]-avg)*(count[i]-avg);
    }

    return sqrt(var/(len-1));
}

//集合的总体标准差
long double STDEVP(int number,long double count[],int len)
{
    long double avg;
    long double var=0;
    avg=AVG(number,count,len);
    for(int i=0;i<len;i++)
    {
        var+=(count[i]-avg)*(count[i]-avg);
    }

    return sqrt(var/len);
}

#endif
