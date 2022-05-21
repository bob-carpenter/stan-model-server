#include <cmdstan/io/json/json_data.hpp>
#include <stan/io/empty_var_context.hpp>
#include <stan/model/model_base.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <CLI11/CLI11.hpp>

stan::model::model_base& new_model(stan::io::var_context &data_context,
                                   unsigned int seed, std::ostream *msg_stream);

using shared_context_ptr = std::shared_ptr<stan::io::var_context>;

bool repl_instruction(stan::model::model_base& model,
                      std::istream& in,
                      std::string& instruction) {
  std::getline(in, instruction);
  if (instruction == "quit") {
    std::cout << "REPL ended." << std::endl;
    return false;
  } else if (instruction == "name") {
    std::cout << model.model_name() << std::endl;
    return true;
  } else if (instruction == "param_num") {
      std::vector<std::string> names;
      model.constrained_param_names(names, true, true);
      std::cout << names.size() << std::endl;
      return true;
  } else if (instruction == "param_num_unc") {
      std::vector<std::string> names;
      model.unconstrained_param_names(names, false, false);
      std::cout << names.size() << std::endl;
      return true;
  } else {
    std::cout << "Unknown instruction." << std::endl;
    return true;
  }

}

template <typename T>
void repl(T&& data_context, uint seed) {
  stan::model::model_base* model = &new_model(data_context, seed, &std::cerr);
  std::string instruction;
  while (repl_instruction(*model, std::cin, instruction));
  delete model;
}

int main(int argc, const char* argv[]) {
  // remove synch on std I/O
  std::ios_base::sync_with_stdio(false);
  // don't flush std::out before read std::cin
  std::cin.tie(NULL);

  CLI::App app{"Stan Command Line Interface"};

  std::string data_file_path;
  app.add_option("-d, --data", data_file_path,
                 "File containing data in JSON", true)
      -> check(CLI::ExistingFile);

  unsigned int seed = 1234;
  app.add_option("-s, --seed", seed,
                 "Random seed", true)
      -> check(CLI::PositiveNumber);

  try {
    CLI11_PARSE(app, argc, argv);
  } catch (const CLI::Error& e) {
    return app.exit(e);
  }

  stan::model::model_base* model_ptr;
  if (app.count("--data")) {   // JSON data file exists
    std::ifstream in(data_file_path);
    if (!in.good()) {
      std::cout << "Cannot read input file: " << data_file_path << "."
                << std::endl;
      return -1;
    }
    cmdstan::json::json_data json_data_context(in);
    in.close();
    repl(json_data_context, seed);
  } else {
    stan::io::empty_var_context empty_data_context;
    repl(empty_data_context, seed);
  }
}
