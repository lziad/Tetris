#include <iostream>
#include <string>
using namespace std;

int main()
{
	string s;
	string r;
	while (getline(cin, s))
	{
		r += s;
	}
	cout << r;
}