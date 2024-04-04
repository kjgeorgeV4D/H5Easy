#include <iostream>
#include <vector>
#include <string>

#include "H5Easy.h"

using namespace std;

int main()
{

   // Load Fenics data
   LoadH5 ldata;
   ldata.setFileName("/home/kjgeorge/run/heat_example/kappa_000000.h5");
   ldata.setVarName("/Function/f/0");
   vector<double>gfdta = ldata.getData();
   for ( vector<double>::iterator it = gfdta.begin(); it != gfdta.end(); ++it )
      cout << *it << endl;

   return 0;
}

