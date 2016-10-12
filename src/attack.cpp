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
	ofstream model_file;
	string model_file_name = circuit_name + ".smv";
	model_file.open(model_file_name.c_str());

	Circuit c;
        c.read_bench(argv[1]);
        c.camouflage_random(0.05, true);
        string model = c.generate_smv_model();
        model_file << model;
	model_file.close();
// exit(0); 
        int discriminating_example_count = 0;
        int bmc_length = 10, bmc_length_increment = 10;
        cout << "BMC length: " << bmc_length << endl;
while (true) {
            
                        // cout << "Iteration...\n" << flush;
            // cout << "Example No. " << discriminating_example_count << endl << flush;
            // PressEnterToContinue();
       
	stringstream shell_command;
        ofstream commands_file;
        commands_file.open("commands");
        if (atoi(argv[2]) == 0) {
        commands_file << "read_model -i " << model_file_name << endl << "go_bmc" << endl << "check_ltlspec_bmc_onepb -k " << bmc_length << "" << endl << "show_traces -o output" << endl << "quit" << endl;
        }
        else {
            commands_file << "read_model -i " << model_file_name << endl << "go" << endl << "check_ctlspec" << endl << "show_traces -o output" << endl << "quit" << endl;
        }
        commands_file.close();
         system("rm -rf output");
         cout << "Checking for counterexamples..." << endl;

shell_command << "NuSMV-2.6.0-Linux/bin/NuSMV -source commands > /dev/null 2>&1";
               if (system(shell_command.str().c_str())) { cout << "Fatal error: exiting..." << endl; exit(EXIT_FAILURE); };
	
        struct stat buf;
	        if (stat("output", &buf) != 0) { // file doesn't exist
            // system("rm -rf output"); 
            if (atoi(argv[2]) == 1) { cout << "No counterexample found: circuit decamouflaged." << endl; break; }
            cout << "No counterexample found: checking reachability..." << endl; // PressEnterToContinue();
            commands_file.open("commands");
            commands_file << "read_model -i " << model_file_name << endl << "go_bmc" << endl << "check_ltlspec_bmc_onepb -p \"FALSE\" -l X -k " << bmc_length << endl << "show_traces -o output" << endl << "quit" << endl;
            commands_file.close();

            // shell_command << "NuSMV-2.6.0-Linux/bin/NuSMV -source commands";
            // system("rm -rf output");
            if (system(shell_command.str().c_str())) { cout << "Fatal error: exiting..." << endl; exit(EXIT_FAILURE); };
	
	    // ifstream example_file;
            // example_file.open("output");        
        
            // string line;

            if (stat("output", &buf) != 0) { cout << "All states reached: circuit decamouflaged." << endl; break; }
            else { cout << "States remaining to explore: Increasing BMC length" << endl; bmc_length+=bmc_length_increment; cout << "BMC length: " << bmc_length << endl; }
        }
        else {
            cout << "Counterexample found: updating model..." << endl;
ifstream example_file;
        example_file.open("output");        
        
            string line;


            getline(example_file, line); getline(example_file, line); getline(example_file, line);
            discriminating_example_count++;
            //getline(example_file, line);
            map<string,bool> initial_state;
            //while (line != "  -> State: 1.2 <-") {
            //    vector<string> fields;
            //    split(fields, line, is_any_of(" ="), token_compress_on);
            //    initial_state[fields[0].substr(0, fields[0].length()-2)] = (fields[1] == "TRUE"? true: false);
            //    getline(example_file, line);
            //}
        
            vector<map<string,bool> > input_sequence;
            int example_length = 0;
            map<string,bool> input;
            while (true) {
                //map<string,bool> input;
                bool found = false;
                if (!getline(example_file, line)) break;
                while (line.substr(0, 14) != "  -> State: 1.") {
                    if (line == "  -- Loop starts here") { getline(example_file, line); continue; }
                    vector<string> fields;
                    split(fields, line, is_any_of("\t ="), token_compress_on);
                    // cout << "here";
                    // cout << fields[1] << endl << fields[2] << endl << flush;
                    input[fields[1].substr(0, fields[1].length()-2)] = (fields[2] == "TRUE"? true: false);
                    if (!getline(example_file, line)) { found = true; break; }
                }
            
                input_sequence.push_back(input);
                example_length++;
                if (found) break;
                //found = false;
                //while (getline(example_file, line))
                //    if (line.substr(0, 14) == "  -> Input: 1.") { found = true; break; }
                //if (!found) break;
            }
            // cout << "Example length: " << example_length << endl << flush;
            model_file.open(model_file_name.c_str(), ofstream::app);
            vector<map<string,bool> > output_sequence;
            
            output_sequence = c.simulate_input_sequence(initial_state, input_sequence);
            model_file << c.generate_smv_sequential_constraint(discriminating_example_count, initial_state, input_sequence, output_sequence);
            model_file.close();
            example_file.close();
// PressEnterToContinue();
        }       
        } 
        
            // bool found = false;
            // for (int i = 0; i < 21; i++) if (!getline(example_file, line)) { found = true; break; }
            // if (found) break;
         

	
	return 0;
}
