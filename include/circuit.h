#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <string>
#include <map>
#include <vector>

using namespace std;

struct node {
	string type;
	bool camouflaged, output;
	vector<string> fanins;
};

enum GateType { G_NOR, G_NAND };

class Circuit {
	private:
		map<string,node> node_table;
                int gate_count;
                bool evaluate(string gate_name);
                friend class Circuit_simulate_input_sequence;
                friend class SMVAttack;
                map<string, bool> initial_state;
	public:
		Circuit();
		string generate_smv_model();
		int read_bench(char* bench_file_name);
                void remove_input_buffers();
                int camouflage_random(float percentage, bool xor_or_xnor);
                int decamouflage();
		string generate_smv_sequential_constraint(int number, map<string,bool> initial_state, vector<map<string,bool> > input_sequence, vector<map<string,bool> > output_sequence);
                vector<map<string,bool> > simulate_input_sequence(map<string,bool> initial_state, vector<map<string,bool> > input_sequence);
		~Circuit();
};

struct example {
    map<string,bool> initial_state;
    vector<map<string,bool> > input_sequence;
    vector<map<string,bool> > output_sequence;
};

/*class SMVAttack {
    Circuit c;
    vector<example> examples;
    map<string, GateType> completion();
    string model;
    void build_model();
    
    public:
        SMVAttack(Circuit &circuit);
        map<string, GateType> decamouflage(Circuit &c);
}*/
#endif
