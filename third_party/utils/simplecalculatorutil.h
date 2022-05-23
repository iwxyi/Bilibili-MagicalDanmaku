#ifndef SIMPLECALCULATORUTIL_H
#define SIMPLECALCULATORUTIL_H

#include <iostream>
#include <stack>
#include <math.h>
#include <QString>

using namespace std;

/**
 * 源码：https://blog.csdn.net/jimu41/article/details/116479830
 */

class SimpleCalculatorUtil
{
public:
    double calculate(char* str)
    {
        SimpleCalculatorUtil sc;

        stack<char> Ope;
        stack<double> Num;
        char ope_;
        double num_;

        for (size_t i = 0; i < strlen(str); i++)
        {
            char ch = str[i];
            if (ch >= '0' && ch <= '9')
            {
                num_ = ch - '0';
                Num.push(num_);
            }
            else
            {
                ope_ = ch;
                if (ope_ == '=') {
                    while (!Ope.empty()) calculate(Ope, Num); //如果符号栈不空，就一直计算
                    return Num.top();  //如果是等号且符号栈顶为空，就返回数字栈顶元素
                }
                else if (ope_ == '!') factorial(Num);  //如果是！就阶乘
                else if (ope_ == '(' ||Ope.empty()) Ope.push(ope_);    //如果符号是左括号或符号栈为空直接压入
                else if (ope_ == ')') {  //如果是右括号
                    while (Ope.top() != '(') calculate(Ope, Num); //一直计算完括号里的
                    Ope.pop();  //左括号出栈
                }
                //else if (priority(Ope.top()) >= priority(ope_)) {  //如果栈顶符号的优先级大于等于当前
                else if (!Ope.empty() && priority(Ope.top()) >= priority(ope_)) {
                    calculate(Ope, Num);   //计算结果压入数字栈,取出当前栈顶
                    Ope.push(ope_);  //压入当前符号
                }
                else Ope.push(ope_);  //否则就压入符号栈
            }
        }
    }

private:
    void calculate(stack<char>& Ope, stack<double>& Num) {
        double a, b;

        if (Ope.top() == '-') {
            a = Num.top(); Num.pop();
            Ope.pop();   //取出负号
            //Num.push(-a);  //压入负值
            if (!Num.empty()) {
                if(Ope.empty() || Ope.top() != '(')
                    Ope.push('+');  //如果前面还有数字，就压入+，即变成加负值
            }
            Num.push(-a);  //压入负值
        }
        else {
            a = Num.top(); Num.pop();
            b = Num.top(); Num.pop();

            if (Ope.top() == '+') Num.push(b + a);
            else if (Ope.top() == '*') Num.push(b * a);
            else if (Ope.top() == '/') Num.push(b / a);
            else if (Ope.top() == '^') Num.push(pow(b, a));

            Ope.pop();
        }
    }

    void factorial(stack<double>& Num) {
        int a = static_cast<int>(Num.top());
        int result=1;

        for (int i = 1; i <= a; i++) result *= i;

        Num.push(static_cast<double>(result));
    }

    int priority(char ope_) {
        if (ope_ == '(') return 0;
        else if (ope_ == '+' || ope_ == '-') return 1;
        else if (ope_ == '*' || ope_ == '/') return 2;
        else if (ope_ == '^') return 3;
    }
};

#endif // SIMPLECALCULATORUTIL_H
