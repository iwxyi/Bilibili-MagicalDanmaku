
#ifndef nefu_ddos
#define nefu_ddos

#ifndef SINT
#define SINT 9223372036854775783 //如果返回值为这个数，说明字符窜接受有误，也就是说参数的传递有误，
#endif

#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<cstring>
#include<math.h>
#include <stack>

using namespace std;

#endif // nefu_ddos


#ifndef MEM
#define MEM
#include <memory.h>
#endif

char comper(char a,char b)
{
    if(a=='#'&&b=='#')
    {
        return 'r';
    }
    if(a!='#'&&b=='#')
    {
        return '>';
    }

    if(a=='#'||a=='(')
    {
        if(a=='('&&b==')')
            return '=';
        else if(a=='#'&&b=='#')
        {
            return 'r';
        }
        else
            return ('<');
    }

    if(a=='+'||a=='-')
    {
        if(b=='+'||b=='-'||b=='#'||b==')')
            return('>');
        else
            return('<');
    }
    if(a=='*'||a=='/')
    {
        if(b=='+'||b=='-'||b==')'||b=='#'||b=='*'||b=='/'||b=='%')
            return('>');
        else
            return('<');
    }
    if(a=='^')
    {
        if(b=='(')
            return ('<');
        else
            return('>');
    }
    if(a=='%')
    {
        if(b=='(')
            return ('<');
        else
            return ('>');
    }
}

long double calculate(long double m,char c,long double n)
{
    // cout<<"m="<<m<<' '<<"n="<<n<<endl;
    switch(c)
    {
    case '+':return(m+n);break;
    case '-':return(m-n);break;
    case '*':return(m*n);break;
    case '/':return(m/n);break;
    case '^':return(pow(m,n));break;
        // case '%':return fmod(m,n);break;
    case '%':
        if( (int)m == m && (int)n == n )
            return (int)m%(int)n;
        else return SINT;
        break;
    }
}

long double string_temp_dispose(char s[],int l)
{
    int g=0,lenth=0,help=0,l_length;//
    stack<char>q1;//存放运算符
    //q1.push('#');
    stack<long double>q2;//存放数字
    long double m,n,k;
    char str[100];
    memset(str,'&',100);
    char c;

    q1.push('#');
    g=l;
    for(int i=0;i<g;)//这就是为什么要把i++放在数字的循环里面
    {

        if((s[i]>='0'&&s[i]<='9')||s[i]=='.'||(s[i]=='-'&& i==0)||( s[i]=='-' && (!isdigit(s[i-1])&&s[i-1]!=')') ))
        {
            str[help++]=s[i++];
            lenth=1;
        }

        else {

            if(lenth!=0)
            {

                k=atof(str);
                //  cout<<"该数字为："<<k<<endl;
                q2.push(k);
                lenth=0;
                help=0;
                l_length=0;
                memset(str,'&',100);

            }

            else
            {
                switch(comper(q1.top(),s[i]))
                {

                case '<':q1.push(s[i]);i++;break;
                case '>':
                    if (q1.size() && q2.size() > 1)
                    {
                        n = q2.top(); q2.pop();
                        m = q2.top(); q2.pop();
                        c = q1.top(); q1.pop();
                    }
                    else
                    {
                        n = m = c = 0;
                    }

                    q2.push(calculate(m,c,n));

                    break;
                case '=':q1.pop();i++;break;
                case 'r':return q2.size() ? q2.top() : 0;i++;break;

                }

            }

        }

    }//end for
}

/*
int main()
{
        int w;
        char s[2000];


        cin>>w;//输入的字符序列的个数
        while(w--)
        {
                scanf("%s",s);

                cout<<"输入序列为："<<s<<endl;
                int len=strlen(s);
                s[len]='#';
                len++;
                printf("%.2f\n",string_temp_dispose(s,len));

        }
}

*/
