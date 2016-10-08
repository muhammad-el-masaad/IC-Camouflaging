#include <circuit.h>
#include <vector>
#include <map>
#include <stdlib.h>
#include <time.h>
#include <sstream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

Circuit::Circuit() {
    gate_count = 0;
}

Circuit::~Circuit() {
    
}

int Circuit::read_bench(char* circuit_file_name) {
    ifstream circuit_file;
    circuit_file.open(circuit_file_name);
    string line;
    vector<string> outputs;
    while (getline(circuit_file, line)) {
        if (line[0] == '#') continue;
        if (line == "") continue;
        if (line.substr(0,5) == "INPUT") { 
	    string input_name = line.substr(6, line.length()-7); // model_file << "IVAR " + input_name + " : boolean;\n"; 
	    node n;
	    n.camouflaged = false;
            n.output = false;
            n.type = "INPUT";
	    node_table.insert(pair<string, node>(input_name, n));
	    continue;
	}
        if (line.substr(0,6) == "OUTPUT") { 
	    string output_name = line.substr(7, line.length()-8); 
	    node n;
	    n.camouflaged = false;
            n.output = true;
            // node_table.insert(pair<string, node>(output_name, n));
            node_table[output_name].camouflaged = false;
            node_table[output_name].output = true;
	    continue; 
	}
        vector<string> fields;
        split(fields, line, is_any_of( "(),= " ), token_compress_on);

        if (fields[1] == "DFF") {
	    node n; n.camouflaged = false; n.type = "STATE"; n.output = false;
	    n.fanins.push_back(fields[2]); 
	    // node_table.insert(pair<string, node>(fields[0], n));
            node_table[fields[0]].camouflaged = false;
            node_table[fields[0]].type = "STATE";
            node_table[fields[0]].fanins.push_back(fields[2]);
            continue;
        }
        node n;
        node_table[fields[0]].type = fields[1]; 
	node_table[fields[0]].camouflaged = false;
        // node_table[fields[0]].output = false;
        for (int i = 2; i < fields.size() - 1; i++) {
            node_table[fields[0]].fanins.push_back(fields[i]);
        }
        gate_count++;
    }

    for (map<string,node>::iterator it = node_table.begin(); it != node_table.end(); ++it) {
        if (it->second.type == "STATE") {
            string type = node_table[it->second.fanins[0]].type;
            if (type == "NAND" || type == "OR") initial_state[it->first] = false;
            if (type == "NOR" || type == "AND" || type == "NOT") initial_state[it->first] = false;
        }
    }
    return 0;
}

// percentage is percentage of gates in the netlist to camouflage
// xnor_or_xor is used to specify whether to camouflage XNOR or XOR gates

int Circuit::camouflage_random(float percentage, bool xnor_or_xor) {     
    vector<string> camouflaging_candidates;
    for (std::map<string, node>::iterator it=node_table.begin(); it!=node_table.end(); ++it)
	if (it->second.type == "NAND" || it->second.type == "NOR" || it->second.type == "XOR") camouflaging_candidates.push_back(it->first);
    srand(time(NULL));
    for (int i = 0; i < percentage * gate_count; i++) {
        int index = rand() % camouflaging_candidates.size();
        node_table[camouflaging_candidates[index]].camouflaged = true;
        camouflaging_candidates.erase(camouflaging_candidates.begin() + index);
    }
    return 0;
}

int Circuit::decamouflage() { // set camouflaging status of all gates in the netlist to false
    for (std::map<string, node>::iterator it=node_table.begin(); it!=node_table.end(); ++it)
        it->second.camouflaged = false;
    return 0;
}

string Circuit::generate_smv_model() {
    stringstream model;
    vector<string> input_list, camouflaged_gate_list, output_list;
    model << "MODULE main\n";
    for (map<string,node>::iterator it=node_table.begin(); it != node_table.end(); ++it) {
        node n = it->second;
        if (n.type == "INPUT") {
            model << "DEFINE " + it->first + "_1 := " << it->first << ";\nDEFINE " + it->first + "_2 := " << it->first << ";\n";    
            input_list.push_back(it->first);
            continue;
        }
        if (n.output) output_list.push_back(it->first);
if (n.type == "STATE") {
            model << "VAR " + it->first + "_1 : boolean;\n";
	    model << "VAR " + it->first + "_2 : boolean;\n";

	    model << "INIT (" + it->first + "_1 = " << (initial_state[it->first]? "TRUE": "FALSE") << ") & (" + it->first + "_2 = " << (initial_state[it->first]? "TRUE": "FALSE") << ");\n";

	    model << "TRANS next(" + it->first + "_1) = " + n.fanins[0] + "_1;\n";
	    model << "TRANS next(" + it->first + "_2) = " + n.fanins[0] + "_2;\n";
	    continue;
	}
	
        
        //model << "VAR " << it->first << "_1 : boolean;\n";
	//model << "VAR " << it->first << "_2 : boolean;\n";
 
        
        if (n.camouflaged) { 
            camouflaged_gate_list.push_back(it->first);
	    model << "FROZENVAR " + it->first + "_id_1 : {nand, nor};\n";
            model << "FROZENVAR " + it->first + "_id_2 : {nand, nor};\n";
	    model << "DEFINE " + it->first + "_1 := (\n";
	    model << "\tcase\n";
	    model << "\t\t" + it->first + "_id_1 = nand : !(" + n.fanins[0] + "_1";
	    for (int j = 1; j < n.fanins.size(); j++) model << " & " + n.fanins[j] + "_1";
	    model << ");\n";
	    model << "\t\t" + it->first + "_id_1 = nor : !(" + n.fanins[0] + "_1";
	    for (int j = 1; j < n.fanins.size(); j++) model << " | " + n.fanins[j] + "_1";
	    model << ");\n" ;
	    model << "esac);\n";
	    model << "DEFINE " + it->first + "_2 := (\n";
	    model << "\tcase\n";
	    model << "\t\t" + it->first + "_id_2 = nand : !(" + n.fanins[0] + "_2";
	    for (int j = 1; j < n.fanins.size(); j++) model << " & " + n.fanins[j] + "_2";
	    model << ");\n";
	    model << "\t\t" + it->first + "_id_2 = nor : !(" + n.fanins[0] + "_2";
	    for (int j = 1; j < n.fanins.size(); j++) model << " | " + n.fanins[j] + "_2";
	    model << ");\n" ;
            model << "esac);\n";
	    continue; 			 
	}
		// switch(gates[i].type) {
	if (n.type == "NOT") {
            model << "DEFINE " + it->first + "_1 := !" + n.fanins[0] + "_1;\n";
	    model << "DEFINE " + it->first + "_2 := !" + n.fanins[0] + "_2;\n";
	}
	if (n.type == "AND") {
	    model << "DEFINE " + it->first + "_1 := (" + n.fanins[0] + "_1";
	    for (int j = 1; j < n.fanins.size(); j++) model << " & " + n.fanins[j] + "_1";
	    model << ");\n";
	    model << "DEFINE " + it->first + "_2 := (" + n.fanins[0] + "_2";
	    for (int j = 1; j < n.fanins.size(); j++) model << " & " + n.fanins[j] + "_2";
	    model << ");\n";
        }
        if (n.type == "NAND") {
		        model << "DEFINE " + it->first + "_1 := !(" + n.fanins[0] + "_1";
			for (int j = 1; j < n.fanins.size(); j++) model << " & " + n.fanins[j] + "_1";
			model << ");\n";
			model << "DEFINE " + it->first + "_2 := !(" + n.fanins[0] + "_2";
			for (int j = 1; j < n.fanins.size(); j++) model << " & " + n.fanins[j] + "_2";
			model << ");\n";
		    }
		    if (n.type == "OR") {
		        model << "DEFINE " + it->first + "_1 := (" + n.fanins[0] + "_1";
		        for (int j = 1; j < n.fanins.size(); j++) model << " | " + n.fanins[j] + "_1";
			model << ");\n";
			model << "DEFINE " + it->first + "_2 := (" + n.fanins[0] + "_2";
			for (int j = 1; j < n.fanins.size(); j++) model << " | " + n.fanins[j] + "_2";
			model << ");\n";
		    }
		    if (n.type == "NOR") {
			model << "DEFINE " + it->first + "_1 := !(" + n.fanins[0] + "_1";
			for (int j = 1; j < n.fanins.size(); j++) model << " | " + n.fanins[j] + "_1";
			model << ");\n";
			model << "DEFINE " + it->first + "_2 := !(" + n.fanins[0] + "_2";
			for (int j = 1; j < n.fanins.size(); j++) model << " | " + n.fanins[j] + "_2";
			model << ");\n";
		    }
    }

    // constraint to ensure the two completions differ in at least one camouflaged gate
    model << "INIT";
    for (int i = 0; i < camouflaged_gate_list.size(); i++) model << (i == 0? " ": " | ") << "(" + camouflaged_gate_list[i] + "_id_1 != " + camouflaged_gate_list[i] + "_id_2)";
    model << ";\n";

    // constraint to ensure the input sequences to the two completions are the same
    for (int i = 0; i < input_list.size(); i++) model << "VAR " << input_list[i] << " : boolean;\n";

    // ensure two completions are sequentially equivalent	    
    model << "LTLSPEC G";
    for (int i = 0; i < output_list.size(); i++) model << (i == 0? " (": " & ") << "(" << output_list[i] << "_1 = " << output_list[i] << "_2)";
    model << ");\n";
    model << "CTLSPEC AG";
    for (int i = 0; i < output_list.size(); i++) model << (i == 0? " (": " & ") << "(" << output_list[i] << "_1 = " << output_list[i] << "_2)";
    model << ");\n";

    return model.str();
}

class Circuit_simulate_input_sequence {
    Circuit* self;
    vector<map<string,bool> > retval;
    vector<map<string,bool> > output; 
    map<string,bool> evaluated, values;
     
    bool evaluate(string gate_name) {
        if (evaluated[gate_name]) return values[gate_name];
        else {
            node n = self->node_table[gate_name];
            for (int i = 0; i < n.fanins.size(); i++) evaluate(n.fanins[i]);
            if (n.type == "NOT") values[gate_name] = !evaluate(n.fanins[0]);
            if (n.type == "AND") {
                values[gate_name] = true;
                for (int j = 0; j < n.fanins.size(); j++) 
                    if (!evaluate(n.fanins[j])) { 
                        values[gate_name] = false;
                        break;
                    }
            }
            if (n.type == "NAND") {
                values[gate_name] = false;
                for (int j = 0; j < n.fanins.size(); j++) 
                    if (!evaluate(n.fanins[j])) { 
                        values[gate_name] = true;
                        break;
                    }
            } 
            if (n.type == "OR") {
                values[gate_name] = false;
                for (int j = 0; j < n.fanins.size(); j++) 
                    if (evaluate(n.fanins[j])) { 
                        values[gate_name] = true;
                        break;
                    }
            } 
            if (n.type == "NOR") {
                values[gate_name] = true;
                for (int j = 0; j < n.fanins.size(); j++) 
                    if (evaluate(n.fanins[j])) { 
                        values[gate_name] = false;
                        break;
                    }
            }
        }
        evaluated[gate_name] = true;
        return values[gate_name];
    }
    public:
        Circuit_simulate_input_sequence(Circuit* c, map<string,bool> initial_state, vector<map<string,bool> > input_sequence) {
            self = c;
            for (int cycle = 0; cycle < input_sequence.size(); cycle++) { // loop through sequential cycles
                for (map<string,node>::iterator it = c->node_table.begin(); it != c->node_table.end(); ++it) evaluated[it->first] = false;

                for (map<string,node>::iterator it = c->node_table.begin(); it != c->node_table.end(); ++it) {
                    evaluated[it->first] = false;
                    if (it->second.type == "INPUT") { 
                        evaluated[it->first] = true; 
                        values[it->first] = input_sequence[cycle][it->first];
                    }
                    else if (it->second.type == "STATE") {
                        evaluated[it->first] = true;
                        if (cycle == 0) values[it->first] = c->initial_state[it->first]; // initial_state[it->first];
                        else values[it->first] = values[it->second.fanins[0]];
                    }
                }
                for (map<string,node>::iterator it = c->node_table.begin(); it != c->node_table.end(); ++it) {
                    evaluate(it->first);
                }
                // for (map<string,bool>::iterator it = values.begin(); it != values.end(); ++it) cout << it->first << " = " << it->second << endl;
                retval.push_back(values);
            }
        }
        operator vector<map<string,bool> >() { return retval; }
};

vector<map<string,bool> > Circuit::simulate_input_sequence(map<string,bool> initial_state, vector<map<string, bool> > input_sequence) { 
    return Circuit_simulate_input_sequence(this, initial_state, input_sequence); 
}

//vector<string, GateType> decamouflage() {
//    build_model();
//    
//    while (counter_example_exists())
//        update_model();
//    }
//    return completion();
//}

//vector<string, GateType> SMVAttack::completion() {
//}

//void SMVAttack::build_model() {
//}

//bool SMVAttack::counter_example_exists() {
//}

// Given sequential behavior, generate smv constraint to make sure the two completions comply with sequential behaviour
string Circuit::generate_smv_sequential_constraint(int constraint_number, map<string, bool> initial_state, vector<map<string,bool> > input_sequence, vector<map<string,bool> > output_sequence) {
    stringstream constraint;
    // constraint << "DEFINE \n";

    // Define intermediate circuit values
    for (int cycle = 0; cycle < input_sequence.size(); cycle++) {
        ostringstream temp;
        temp << constraint_number << "_" << cycle;
        string number = temp.str();
        for (map<string,node>::iterator it = node_table.begin(); it != node_table.end(); ++it) {
            node n = it->second; string node_name = it->first;
            // constraint << "VAR " << node_name << "_" << number << "_1 : boolean;\n";
            // constraint << "VAR " << node_name << "_" << number << "_2 : boolean;\n";

            if (n.type == "INPUT") {
                constraint << "DEFINE " << node_name << "_" << number << "_1" << " := " << (input_sequence[cycle][node_name]? "TRUE": "FALSE") << ";\n";
                constraint << "DEFINE " << node_name << "_" << number << "_2" << " := " << (input_sequence[cycle][node_name]? "TRUE": "FALSE") << ";\n";
            }
            if (n.type == "STATE") {
                if (cycle == 0) {
                    constraint << "DEFINE " << node_name << "_" << constraint_number << "_0_1 := " << (this->initial_state[it->first]? "TRUE": "FALSE") << ";\n"; // << (initial_state[node_name]? "TRUE": "FALSE") << ";\n";
                    constraint << "DEFINE " << node_name << "_" << constraint_number << "_0_2 := " << (this->initial_state[it->first]? "TRUE": "FALSE") << ";\n"; // << (initial_state[node_name]? "TRUE": "FALSE") << ";\n";
                }
                if (cycle != input_sequence.size() - 1) {
                    constraint << "DEFINE " << node_name << "_" << constraint_number << "_" << cycle + 1 << "_1" << " := " << n.fanins[0] << "_" << number << "_1;\n";
                    constraint << "DEFINE " << node_name << "_" << constraint_number << "_" << cycle + 1 << "_2" << " := " << n.fanins[0] << "_" << number << "_2;\n";
                }
            }
            if (n.output) {
                constraint << "INIT " << it->first << "_" << number << "_1 = " << (output_sequence[cycle][it->first]? "TRUE": "FALSE") << ";\n";
                constraint << "INIT " << it->first << "_" << number << "_2 = " << (output_sequence[cycle][it->first]? "TRUE": "FALSE") << ";\n";
            }
            if (n.camouflaged) { 
                // camouflaged_gate_list->push_back(it->first);
	        // model << "FROZENVAR " + it->first + "_id_1 : {nand, nor};\n";
            //model << "FROZENVAR " + it->first + "_id_2 : {nand, nor};\n";
	        constraint << "DEFINE " << node_name << "_" << number << "_1" << " := (\n";
	        constraint << "\tcase\n";
	        constraint << "\t\t" << node_name << "_id_1 = nand : !(" << n.fanins[0] << "_" << number << "_1";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " & " << n.fanins[j] << "_" << number << "_1";
	        constraint << ");\n";
	        constraint << "\t\t" << node_name <<  "_id_1 = nor : !(" << n.fanins[0] << "_" << number << "_1";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " | " << n.fanins[j] << "_" << number << "_1";
	        constraint << ");\n" ;
	        constraint << "esac);\n";
	        constraint << "DEFINE " << it->first << "_" << number << "_2 := (\n";
	        constraint << "\tcase\n";
	        constraint << "\t\t" << it->first << "_id_2 = nand : !(" << n.fanins[0] << "_" << number << "_2";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " & " << n.fanins[j] << "_" << number << "_2";
	        constraint << ");\n";
	        constraint << "\t\t" << it->first << "_id_2 = nor : !(" << n.fanins[0] << "_" << number << "_2";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " | " << n.fanins[j] << "_" << number << "_2";
	        constraint << ");\n" ;
                constraint << "esac);\n";
	        continue; 			 
	    }
		// switch(gates[i].type) {
	    if (n.type == "NOT") {
                constraint << "DEFINE " << it->first << "_" << number << "_1 := !" << n.fanins[0] << "_" << number << "_1;\n";
	        constraint << "DEFINE " << it->first << "_" << number << "_2 := !" << n.fanins[0] << "_" << number << "_2;\n";
	    }
	    if (n.type == "AND") {
	        constraint << "DEFINE " << it->first << "_" << number << "_1 := (" << n.fanins[0] << "_" << number << "_1";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " & " << n.fanins[j] << "_" << number << "_1";
	        constraint << ");\n";
	        constraint << "DEFINE " << it->first << "_" << number << "_2 := (" << n.fanins[0] << "_" << number << "_2";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " & " << n.fanins[j] << "_" << number << "_2";
	        constraint << ");\n";
            }
            if (n.type == "NAND") {
	        constraint << "DEFINE " << it->first << "_" << number << "_1 := !(" << n.fanins[0] << "_" << number << "_1";
	        for (int j = 1; j < n.fanins.size(); j++) constraint << " & " << n.fanins[j] << "_" << number << "_1";
	        constraint << ");\n";
		constraint << "DEFINE " << it->first << "_" << number << "_2 := !(" << n.fanins[0] << "_" << number << "_2";
		for (int j = 1; j < n.fanins.size(); j++) constraint << " & " << n.fanins[j] << "_" << number << "_2";
		constraint << ");\n";
	    }
	    if (n.type == "OR") {
	        constraint << "DEFINE " << it->first << "_" << number << "_1 := (" << n.fanins[0] << "_" << number << "_1";
		for (int j = 1; j < n.fanins.size(); j++) constraint << " | " << n.fanins[j] << "_" << number << "_1";
		constraint << ");\n";
		constraint << "DEFINE " << it->first << "_" << number << "_2 := (" << n.fanins[0] << "_" << number << "_2";
		for (int j = 1; j < n.fanins.size(); j++) constraint << " | " << n.fanins[j] << "_" << number << "_2";
		constraint << ");\n";
	    }
            if (n.type == "NOR") {
		constraint << "DEFINE " << it->first << "_" << number << "_1 := !(" << n.fanins[0] << "_" << number << "_1";
		for (int j = 1; j < n.fanins.size(); j++) constraint << " | " << n.fanins[j] << "_" << number << "_1";
		constraint << ");\n";
		constraint << "DEFINE " << it->first << "_" << number << "_2 := !(" << n.fanins[0] << "_" << number << "_2";
		for (int j = 1; j < n.fanins.size(); j++) constraint << " | " << n.fanins[j] << "_" << number <<  "_2";
		constraint << ");\n";
	    }
            
	}
    }
    return constraint.str();
}
