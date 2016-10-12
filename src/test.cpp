#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include "circuit.h"
#include <sstream>
#include <iostream>
#include <sys/stat.h>

using namespace std;
using namespace boost;

void PressEnterToContinue()
  {
  std::cout << "Press ENTER to continue... " << flush;
  std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
  }

int main (int argc, char** argv) {
	string circuit_name = argv[1];
	circuit_name = circuit_name.substr(0, circuit_name.length()-6);

	Circuit c;
        c.read_bench(argv[1]);
        c.remove_input_buffers();
        exit(1);
        c.camouflage_random(0.05, true);
        map<string, bool> input;
        vector<map<string, bool> > input_sequence;
        input["LINE1"] = true;
      input["LINE2"] = true;
 input["G2"] = false;
 input_sequence.push_back(input);
 input["G0"] = false;
// for (int i = 0; i < 15; i++) input_sequence.push_back(input);
input_sequence.push_back(input);
input_sequence.push_back(input);
input_sequence.push_back(input);
// input_sequence.push_back(input);
        input["LINE1"] = false;
      input["LINE2"] = false;
 input["G2"] = false;
 input_sequence.push_back(input);
 input["G0"] = false;
// for (int i = 0; i < 15; i++) input_sequence.push_back(input);
input_sequence.push_back(input);
input_sequence.push_back(input);
input_sequence.push_back(input);
input_sequence.push_back(input);
// input_sequence.push_back(input);
vector<map<string, bool> > output_sequence = 
c.simulate_input_sequence(input_sequence[0], input_sequence);

for (int i = 0; i < output_sequence.size(); i++) {
cout << output_sequence[i]["STATO_REG_2_"] << endl;
cout << output_sequence[i]["STATO_REG_1_"] << endl;
cout << output_sequence[i]["STATO_REG_0_"] << endl;
cout << output_sequence[i]["OUTP_REG"] << endl;
cout << output_sequence[i]["OVERFLW_REG"] << endl;
cout << endl;
// cout << output_sequence[i]["G133"] << endl;
// cout << output_sequence[i]["G66"] << endl;
// cout << output_sequence[i]["G67"] << endl;
}

        	return 0;
}
