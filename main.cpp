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

void param_unconstrain(stan::model::model_base& model,
                       std::istream& in, std::ostream& out,
                       std::ostream& err,
                       std::stringstream& cmd) {
  std::vector<std::string> names;
  static constexpr bool include_transformed_parameters = false;
  static constexpr bool include_generated_quantities = false;
  model.constrained_param_names(names, include_transformed_parameters,
                                include_generated_quantities);
  auto N = names.size();
  std::vector<double> params(N);
  std::vector<int> dummy_params_i;
  for (int n = 0; n < params.size(); ++n)
    cmd >> params[n];
  std::vector<double> params_unc;
  //  model.transform_inits_impl(params, dummy_params_i, params_unc,
  // std::cerr);
  for (int i = 0; i < params_unc.size(); ++i) {
    if (i > 0) out << ',';
    out << params_unc[i];
  }
}

template <typename RNG>
void param_constrain(stan::model::model_base& model,
                     std::istream& in, std::ostream& out,
                     std::ostream& err, RNG& base_rng,
                     std::stringstream& cmd) {
  std::vector<std::string> names;
  bool include_transformed_parameters;
  cmd >> include_transformed_parameters;
  bool include_generated_quantities;
  cmd >> include_generated_quantities;
  bool incl_gqs = false;
  model.unconstrained_param_names(names, include_generated_quantities,
                                  include_generated_quantities);
  auto N = names.size();
  Eigen::VectorXd params_unc(N);
  for (int n = 0; n < params_unc.size(); ++n)
    cmd >> params_unc(n);
  Eigen::VectorXd params;
  std::vector<int> dummy_params_i;
  model.write_array(base_rng, params_unc, params,
                    include_transformed_parameters,
                    include_generated_quantities, &err);

  for (int i = 0; i < params_unc.size(); ++i) {
    if (i > 0) out << ',';
    out << params[i];
  }
}


bool repl_instruction(stan::model::model_base& model,
                      std::istream& in, std::ostream& out,
                      std::ostream& err,
                      boost::ecuyer1988& base_rng) {
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
  } else if (instruction == "param_unconstrain") {
    param_unconstrain(model, in, out, err, cmd);
  } else if (instruction == "param_constrain") {
    param_constrain(model, in, out, err, base_rng, cmd);
  } else {
    out << "Unknown instruction.";
  }
  out << std::endl;
  return true;
}

struct repl {
  boost::ecuyer1988 base_rng_;
  stan::model::model_base& model_;
  std::istream& in_;
  std::ostream& out_;
  std::ostream& err_;

  template <typename T>
  repl(T&& data_context, uint seed,
       std::istream& in, std::ostream& out, std::ostream& err)
      : base_rng_(seed),
        model_(new_model(data_context, seed, &err)),
        in_(in), out_(out), err_(err) {
    base_rng_.discard(1000000000000L);
  }

  ~repl() { delete &model_; }

  bool loop() {
    while (read_eval_print());
    return 0;  // 0 return code for normal termination
  }

  bool read_eval_print() {
    std::string line;
    std::getline(in_, line);
    std::stringstream cmd(line);
    std::string instruction;
    cmd >> instruction;
    if (instruction == "quit") {
      quit(model_, in_, out_, err_);
      return false;
    }

    if (instruction == "name") {
      name(model_, in_, out_, err_);
    } else if (instruction == "param_names"
               || instruction == "param_unc_names") {
      param_names(model_, in_, out_, err_, cmd, instruction == "param_names");
    } else if (instruction == "param_num"
               || instruction == "param_unc_num") {
      param_num(model_, in_, out_, err_, cmd, instruction == "param_num");
    } else if (instruction == "param_unconstrain") {
      param_unconstrain(model_, in_, out_, err_, cmd);
    } else if (instruction == "param_constrain") {
      param_constrain(model_, in_, out_, err_, base_rng_, cmd);
    } else {
      out_ << "Unknown instruction.";
    }
    out_ << std::endl;
    return true;
  }
};

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

  if (app.count("--data")) {   // JSON data file exists
    std::ifstream in(data_file_path);
    if (!in.good()) {
      std::cout << "Cannot read input file: " << data_file_path << "."
                << std::endl;
      return -1;
    }
    cmdstan::json::json_data json_data_context(in);
    in.close();
    repl r(json_data_context, seed,
           std::cin, std::cout, std::cerr);
    return r.loop();
  } else {
    stan::io::empty_var_context empty_data_context;
    repl r(empty_data_context, seed,
           std::cin, std::cout, std::cerr);
    return r.loop();
  }
}
