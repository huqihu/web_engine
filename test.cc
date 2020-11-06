#include "include/myhead.h"
int main()
{
    ifstream ifs("tem.txtxtx");
    //double test = 0.000000003232312;
    string line;

    getline(ifs, line);
    istringstream iss(line);
    string tem_1;
    string tem_2;
    iss >> tem_1;
    tem_2 = iss.str();
    cout << "tem_1 = " << tem_1 << endl;
    cout << "tem_2 = " << tem_2 << endl;
}