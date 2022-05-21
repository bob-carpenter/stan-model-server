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

void quit(stan::model::model_base& model,
          std::istream& in, std::ostream& out, std::ostream& err) {
  out << "REPL ended." << std::endl;
}

void name(stan::model::model_base& model,
          std::istream& in, std::ostream& out, std::ostream& err) {
    out << model.model_name();
}

void param_names(stan::model::model_base& model,
                 std::istream& in, std::ostream& out, std::ostream&
                 err,
                 std::stringstream& cmd,
                 bool constrained) {
  bool include_transformed_parameters;
  cmd >> include_transformed_parameters;
  bool include_generated_quantities;
  cmd >> include_generated_quantities;
  std::vector<std::string> names;
  if (constrained)
    model.constrained_param_names(names,
                                  include_transformed_parameters,
                                  include_generated_quantities);
  else
    model.unconstrained_param_names(names,
                                    include_transformed_parameters,
                                    include_generated_quantities);
  for (size_t i = 0; i < names.size(); ++i) {
    if (i > 0) out << ",";
    out << names[i];
  }
}

void param_num(stan::model::model_base& model,
               std::istream& in, std::ostream& out, std::ostream& err,
               std::stringstream& cmd,
               bool constrained) {
  bool include_transformed_parameters;
  cmd >> include_transformed_parameters;
  bool include_generated_quantities;
  cmd >> include_generated_quantities;
  std::vector<std::string> names;
  if (constrained)
    model.constrained_param_names(names,
                                      include_transformed_parameters,
                                      include_generated_quantities);
  else
    model.unconstrained_param_names(names,
                                    include_transformed_parameters,
                                    include_generated_quantities);
  out << names.size();
}



bool repl_instruction(stan::model::model_base& model,
          std::istream& in, std::ostream& out, std::ostream& err) {
  std::string line;
  std::getline(in, line);
  std::stringstream cmd(line);
  std::string instruction;
  cmd >> instruction;
  if (instruction == "quit") {
    quit(model, in, out, err);
    return false;
  }

  if (instruction == "name") {
    name(model, in, out, err);
  } else if (instruction == "param_names"
             || instruction == "param_unc_names") {
    param_names(model, in, out, err, cmd, instruction == "param_names");
  } else if (instruction == "param_num"
             || instruction == "param_unc_num") {
    param_num(model, in, out, err, cmd, instruction == "param_num");
  } else {
    out << "Unknown instruction.";
  }
  out << std::endl;
  return true;
}

template <typename T>
void repl(T&& data_context, uint seed,
          std::istream& in, std::ostream& out, std::ostream& err) {
  stan::model::model_base* model = &new_model(data_context, seed, &std::cerr);
  while (repl_instruction(*model, in, out, err));
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
    repl(json_data_context, seed,
         std::cin, std::cout, std::cerr);
  } else {
    stan::io::empty_var_context empty_data_context;
    repl(empty_data_context, seed,
         std::cin, std::cout, std::cerr);
  }
}
